#!/usr/bin/env python

# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Android system-wide tracing utility.

This is a tool for capturing a trace that includes data from both userland and
the kernel.  It creates an HTML file for visualizing the trace.
"""

import errno, optparse, os, re, select, subprocess, signal, sys, time, zlib

flattened_html_file = 'systrace_trace_viewer.html'

battor_exec = os.path.join('..', 'sw', 'battor')
battor_charge_sh = os.path.join('..', 'scripts', 'charge.sh')
battor_sync_sh = os.path.join('..', 'scripts', 'sync.sh')
battor_vregtrace_sh = os.path.join('..', 'scripts', 'vregtrace.sh')

class OptionParserIgnoreErrors(optparse.OptionParser):
  def error(self, msg):
    pass

  def exit(self):
    pass

  def print_usage(self):
    pass

  def print_help(self):
    pass

  def print_version(self):
    pass

def get_device_sdk_version():
  getprop_args = ['adb', 'shell', 'getprop', 'ro.build.version.sdk']

  parser = OptionParserIgnoreErrors()
  parser.add_option('-e', '--serial', dest='device_serial', type='string')
  options, args = parser.parse_args()
  if options.device_serial is not None:
    getprop_args[1:1] = ['-s', options.device_serial]

  try:
    adb = subprocess.Popen(getprop_args, stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE)
  except OSError:
    print 'Missing adb?'
    sys.exit(1)
  out, err = adb.communicate()
  if adb.returncode != 0:
    print >> sys.stderr, 'Error querying device SDK-version:'
    print >> sys.stderr, err
    sys.exit(1)

  version = int(out)
  return version

def add_adb_serial(command, serial):
  if serial is not None:
    command.insert(1, serial)
    command.insert(1, '-s')

def main():
  device_sdk_version = get_device_sdk_version()
  if device_sdk_version < 18:
    legacy_script = os.path.join(os.path.dirname(sys.argv[0]), 'systrace-legacy.py')
    os.execv(legacy_script, sys.argv)

  usage = "Usage: %prog [options] [category1 [category2 ...]]"
  desc = "Example: %prog -b 32768 -t 15 gfx input view sched freq"
  parser = optparse.OptionParser(usage=usage, description=desc)
  parser.add_option('-o', dest='output_file', help='write HTML to FILE',
                    default='trace.html', metavar='FILE')
  parser.add_option('-t', '--time', dest='trace_time', type='float',
                    help='trace for N seconds', metavar='N', default=5)
  parser.add_option('-b', '--buf-size', dest='trace_buf_size', type='int',
                    help='use a trace buffer size of N KB', metavar='N')
  parser.add_option('-k', '--ktrace', dest='kfuncs', action='store',
                    help='specify a comma-separated list of kernel functions to trace')
  parser.add_option('-l', '--list-categories', dest='list_categories', default=False,
                    action='store_true', help='list the available categories and exit')
  parser.add_option('-a', '--app', dest='app_name', default=None, type='string',
                    action='store', help='enable application-level tracing for comma-separated ' +
                    'list of app cmdlines')
  parser.add_option('--no-fix-threads', dest='fix_threads', default=True,
                    action='store_false', help='don\'t fix missing or truncated thread names')

  parser.add_option('--link-assets', dest='link_assets', default=False,
                    action='store_true', help='link to original CSS or JS resources '
                    'instead of embedding them')
  parser.add_option('--from-file', dest='from_file', action='store',
                    help='read the trace from a file (compressed) rather than running a live trace')
  parser.add_option('--asset-dir', dest='asset_dir', default='trace-viewer',
                    type='string', help='')
  parser.add_option('-e', '--serial', dest='device_serial', type='string',
                    help='adb device serial number')

  options, args = parser.parse_args()

  if options.link_assets or options.asset_dir != 'trace-viewer':
    parser.error('--link-assets and --asset-dir is deprecated.')

  if options.list_categories:
    atrace_args = ['--list_categories']
    expect_trace = False
  elif options.from_file is not None:
    atrace_args = ['cat', options.from_file]
    expect_trace = True
  else:
    atrace_args = ['-z']
    expect_trace = True

    if options.trace_time is not None:
      if options.trace_time <= 0:
        parser.error('the trace time must be a positive number')

    if options.trace_buf_size is not None:
      if options.trace_buf_size > 0:
        atrace_args.extend(['-b', str(options.trace_buf_size)])
      else:
        parser.error('the trace buffer size must be a positive number')

    if options.app_name is not None:
      atrace_args.extend(['-a', options.app_name])

    if options.kfuncs is not None:
      atrace_args.extend(['-k', options.kfuncs])

    atrace_args.extend(args)

  script_dir = os.path.dirname(os.path.abspath(sys.argv[0]))

  with open(os.path.join(script_dir, flattened_html_file), 'r') as f:
    trace_viewer_html = f.read()

  html_filename = options.output_file

  select_rlist = []
  select_xlist = []

  result = None
  data = []

  # Attempt to establish connection with an attached BattOr 
  '''
  TODO(aschulman)
  find battor path and make sure udev is setup in battor install so sudo 
  isn't needed
  '''
  battor = None
  battor_args = [battor_exec, '-r', '10000', '-s']
  try:
    battor_file_write = open('/tmp/battor', 'w')
    battor = subprocess.Popen(battor_args,  stdout=battor_file_write,
                              stderr=subprocess.PIPE)
    select_rlist += [battor.stderr]
    select_xlist += [battor.stderr]
  except:
    print 'Missing battor exec?'
    battor_file_write.close()
    pass 

  # Disable charging
  if (battor is not None):
    charging_args = [battor_charge_sh, '0']
    try:
      charging = subprocess.Popen(charging_args)
      charging.wait()
    except:
      print "charge.sh missing?"
      sys.exit(1)

    # Read the BattOr output and watch for the first numerical data
    # that indicates the start of the power data.
    sys.stdout.write("waiting for BattOr to start... ")
    sys.stdout.flush()
    battor_file_read = open('/tmp/battor', 'r')
    while result is None:
      where = battor_file_read.tell()
      line = battor_file_read.readline()
      if not line:
        time.sleep(0.1)
        battor_file_read.seek(where)
      else:
        if line[0] == "0":
          sys.stdout.write("done\n")
          sys.stdout.flush()
          break

      result = battor.poll()

    battor_file_read.close()

    # Enable voltage regulator tracing
    vregtrace_args = [battor_vregtrace_sh, '1']
    try:
      vregtrace = subprocess.Popen(battor_vregtrace_args,  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE)
      vregtrace.wait()
    except:
      print "vregtrace.sh missing?"
      sys.exit(1)

  # Start atrace
  atrace_start_args = ['adb', 'shell', 'atrace', '--async_start'] + atrace_args
  add_adb_serial(atrace_start_args, options.device_serial)
  adb = subprocess.Popen(atrace_start_args)
  adb.wait()

  # Delay while collecting trace
  time.sleep(options.trace_time)

  # Execute BattOr sync
  if battor is not None:
    sync_args = [battor_sync_sh]
    try:
      sync = subprocess.Popen(sync_args,  stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
    except:
      print "sync.sh missing?"
      sys.exit(1)
    sync.wait()

  # Delay after sync to ensure it is in trace
  time.sleep(2)

  atrace_dump_args = ['adb', 'shell', 'atrace', '--async_dump'] + atrace_args
  if options.fix_threads:
    atrace_dump_args.extend([';', 'ps', '-t'])
  add_adb_serial(atrace_dump_args, options.device_serial)
  adb = subprocess.Popen(atrace_dump_args, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
  
  select_rlist += [adb.stdout, adb.stderr]
  select_xlist += [adb.stdout, adb.stderr]

  # Read the text portion of the output and watch for the 'TRACE:' marker that
  # indicates the start of the trace data.
  while result is None:
    ready = select.select(select_rlist, [], select_xlist)
    if adb.stderr in ready[0]:
      err = os.read(adb.stderr.fileno(), 4096)
      sys.stderr.write(err)
      sys.stderr.flush()
    if adb.stdout in ready[0]:
      out = os.read(adb.stdout.fileno(), 4096)
      parts = out.split('\nTRACE:', 1)

      txt = parts[0].replace('\r', '')
      if len(parts) == 2:
        # The '\nTRACE:' match stole the last newline from the text, so add it
        # back here.
        txt += '\n'
      sys.stdout.write(txt)
      sys.stdout.flush()

      if len(parts) == 2:
        data.append(parts[1])
        sys.stdout.write("downloading trace...")
        sys.stdout.flush()
        break

    result = adb.poll()

  result_sync = None
  outs_after_sync = 0

  # Read and buffer the data portion of the output.
  while True:
    ready = select.select(select_rlist, [], select_xlist)
    keepReading = False
    if adb.stderr in ready[0]:
      err = os.read(adb.stderr.fileno(), 4096)
      if len(err) > 0:
        keepReading = True
        sys.stderr.write(err)
        sys.stderr.flush()
    if adb.stdout in ready[0]:
      out = os.read(adb.stdout.fileno(), 4096)
      if len(out) > 0:
        keepReading = True
        data.append(out)

    if result is not None and not keepReading:
      break

    result = adb.poll()

  # Stop atrace
  null_file = open('/dev/null', 'w')
  atrace_stop_args = ['adb', 'shell', 'atrace', '--async_stop']
  add_adb_serial(atrace_dump_args, options.device_serial)
  adb = subprocess.Popen(atrace_stop_args, stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
  adb.wait()

  # Enable voltage regulator tracing
  if battor is not None:
    vregtrace_args = [battor_vregtrace_sh, '0']
    try:
     vregtrace = subprocess.Popen(battor_vregtrace_args,  stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
     vregtrace.wait()
    except:
     print "vregtrace.sh missing?"
     sys.exit(1)

  # Stop BattOr collection
  if battor is not None:
    battor.send_signal(signal.SIGINT)
    battor_file_write.close()

  # Enable charging
  if battor is not None:
    charging_args = [battor_charge_sh, '1']
    try:
      charging = subprocess.Popen(charging_args,  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE)
      charging.wait()
    except:
      print "charge.sh missing?"
      sys.exit(1)

  if result == 0:
    if expect_trace:
      data = ''.join(data)

      # Collapse CRLFs that are added by adb shell.
      if data.startswith('\r\n'):
        data = data.replace('\r\n', '\n')

      # Skip the initial newline.
      data = data[1:]

      if not data:
        print >> sys.stderr, ('No data was captured.  Output file was not ' +
          'written.')
        sys.exit(1)
      else:
        # Indicate to the user that the data download is complete.
        print " done\n"

      # Extract the thread list dumped by ps.
      threads = {}
      if options.fix_threads:
        parts = data.split('USER     PID   PPID  VSIZE  RSS     WCHAN    PC        NAME', 1)
        if len(parts) == 2:
          data = parts[0]
          for line in parts[1].splitlines():
            cols = line.split(None, 8)
            if len(cols) == 9:
              tid = int(cols[1])
              name = cols[8]
              threads[tid] = name

      # Decompress and preprocess the data.
      out = zlib.decompress(data)
      if options.fix_threads:
        def repl(m):
          tid = int(m.group(2))
          if tid > 0:
            name = threads.get(tid)
            if name is None:
              name = m.group(1)
              if name == '<...>':
                name = '<' + str(tid) + '>'
              threads[tid] = name
            return name + '-' + m.group(2)
          else:
            return m.group(0)
        out = re.sub(r'^\s*(\S+)-(\d+)', repl, out, flags=re.MULTILINE)

      html_prefix = read_asset(script_dir, 'prefix.html')
      html_suffix = read_asset(script_dir, 'suffix.html')

      html_file = open(html_filename, 'w')
      html_file.write(
        html_prefix.replace("{{SYSTRACE_TRACE_VIEWER_HTML}}", trace_viewer_html))

      # Write ADB trace to html
      html_file.write("<!-- BEGIN TRACE -->\n"+
          "  <script class=\"trace-data\" type=\"application/text\">\n")
      html_file.write(out)
      html_file.write("  </script>\n<!-- END TRACE -->\n")

      # Write BattOr trace to html
      if (battor is not None):
        with open('/tmp/battor', 'r') as f:
            battor_out = f.read()
        html_file.write("<!-- BEGIN TRACE -->\n"+
            "  <script class=\"trace-data\" type=\"application/text\">\n")
        html_file.write(battor_out)
        html_file.write("  </script>\n<!-- END TRACE -->\n")

      html_file.write(html_suffix)
      html_file.close()
      print "\n    wrote file://%s\n" % os.path.abspath(options.output_file)

  else: # i.e. result != 0
    print >> sys.stderr, 'adb returned error code %d' % result
    sys.exit(1)

def read_asset(src_dir, filename):
  return open(os.path.join(src_dir, filename)).read()


if __name__ == '__main__':
  main()
