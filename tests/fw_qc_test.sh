#!/bin/bash

# Terminate this script on any failure
set -e

BATTOR_SERIAL_PATH=$1

BATTOR_DIR=`dirname $0`/../

echo "==== Building BattOr firmware ===="
cd $BATTOR_DIR/fw
make clean
make
cd -

echo "==== Building BattOr software ===="
cd $BATTOR_DIR/sw
./bootstrap
./configure
make clean
make
cd -

echo "==== Cloning catapult ===="
cd $BATTOR_DIR/tests
rm -rf catapult
git clone https://github.com/catapult-project/catapult.git
cd -

echo "==== Flashing BattOr firmware ===="
cd $BATTOR_DIR/fw
make flash
cd -

OUT_DIR="/tmp/fw_qc_test-self_test"
echo "==== Self test (output in $OUT_DIR) ===="
rm -rf $OUT_DIR
mkdir $OUT_DIR
$BATTOR_DIR/sw/battor -t0 $BATTOR_SERIAL_PATH  > "$OUT_DIR/stdout" 2> "$OUT_DIR/stderr"

OUT_DIR="/tmp/fw_qc_test-bd_loop"
echo "==== Buffer/download loop 30s 100x (output in $OUT_DIR) ===="
rm -rf $OUT_DIR
mkdir $OUT_DIR
for i in `seq 1 100`
do
	echo -n "Test #$i..."
	$BATTOR_DIR/sw/battor -v -b $BATTOR_SERIAL_PATH > "$OUT_DIR/$i""-b_stdout" 2> "$OUT_DIR/$i""-b_stderr"
	
	# Check to see if there were no firmware errors in the last test"
	if [ $i -gt 1 ]
	then
		grep "control: got ack ret:2 type:0 value:0" "$OUT_DIR/$i""-b_stderr" >/dev/null 2>/dev/null
	fi
	sleep 30
	$BATTOR_DIR/sw/battor -d $BATTOR_SERIAL_PATH > "$OUT_DIR/$i""-d_stdout"  2> "$OUT_DIR/$i""-d_stderr"
	echo "PASSED"
done

OUT_DIR="/tmp/fw_qc_test-agent_devicetest"
echo "==== battor_agent device test (output in $OUT_DIR) ===="
rm -rf $OUT_DIR
mkdir $OUT_DIR
cd $BATTOR_DIR/tests/catapult
python common/battor/battor/battor_wrapper_devicetest.py > "$OUT_DIR/stdout" 2> "$OUT_DIR/stderr"
cd -

OUT_DIR="/tmp/fw_qc_test-agent_loop"
echo "==== battor_agent loop 30s 100x (output in $OUT_DIR) ===="
cd $BATTOR_DIR/tests/catapult
rm -rf $OUT_DIR
mkdir $OUT_DIR
BATTOR_AGENT=./common/battor/bin/darwin/x86_64/battor_agent
for i in `seq 1 100`
do
	echo -n "Test #$i..."
	awk 'BEGIN {system("sleep 5"); print "StartTracing"; system("sleep 10"); print "RecordClockSyncMarker marker"; system("sleep 20"); print "StopTracing"}' | $BATTOR_AGENT --battor-serial-log="$OUT_DIR/$i""-serial_log" > "$OUT_DIR/$i""-stdout"  2> "$OUT_DIR/$i""-stderr"
	grep "<marker>" "$OUT_DIR/$i""-stdout" >/dev/null 2>/dev/null
	echo "PASSED"
done

echo "++++++ PASSSED ++++++"
