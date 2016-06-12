#!/bin/bash

OUT_INFO=/tmp/am2321-log.txt
OUT_VAL=/tmp/am2321-val.txt
AM2321_BIN=/home/pi/am2321/am2321-read

if [ "$EUID" -ne 0 ]; then
	echo "This need to be run as root (or sudo)"
	echo "This need to be run as root (or sudo)" > $OUT_INFO
	echo "x:0;E;9;FFFF;FFFF;65535;32767" > $OUT_VAL
	exit 1
fi

eval $AM2321_BIN -b 1 -raw -val -info 2>$OUT_INFO 1>$OUT_VAL

echo -e "Values: "
cat $OUT_VAL
echo -e "\n\nLOG output:"
cat $OUT_INFO

