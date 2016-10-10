#!/bin/bash

# Terminate this script on any failure
set -e

BATTOR_SERIAL_PATH=$1

BATTOR_DIR=`dirname $0`/../

echo "==== Building BattOr firmware ===="
cd $BATTOR_DIR/fw
make clean
make

echo "==== Building BattOr software ===="
cd $BATTOR_DIR/sw
./bootstrap
./configure
make clean
make

echo "==== Flashing BattOr firmware ===="
cd $BATTOR_DIR/fw
make flash

OUT_DIR="/tmp/fw_qc_test-self_test"
echo "==== Self test (output in $OUT_DIR) ===="
rm -rf $OUT_DIR
mkdir $OUT_DIR
$BATTOR_DIR/sw/battor -t $BATTOR_SERIAL_PATH  > "$OUT_DIR/stdout" 2> "$OUT_DIR/stderr"

OUT_DIR="/tmp/fw_qc_test-bd_loop"
echo "==== Buffer/download loop 30s 10x (output in $OUT_DIR) ===="
rm -rf $OUT_DIR
mkdir $OUT_DIR
for i in `seq 1 10`
do
	echo -n "Test #$i..."
	$BATTOR_DIR/sw/battor -v -b $BATTOR_SERIAL_PATH > "$OUT_DIR/$i""-b_stdout" 2> "$OUT_DIR/$i""-b_stderr"
	
	# Check to see if there were no firmware errors in the last test"
	grep "control: got ack ret:2 type:0 value:0" "$OUT_DIR/$i""-b_stderr" >/dev/null 2>/dev/null
	sleep 30
	$BATTOR_DIR/sw/battor -d $BATTOR_SERIAL_PATH > "$OUT_DIR/$i""-d_stdout"  2> "$OUT_DIR/$i""-d_stderr"
	echo "PASSED"
done

echo "++++++ PASSSED ++++++"
