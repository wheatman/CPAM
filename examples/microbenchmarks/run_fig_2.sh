#!/bin/bash

mkdir -p outputs

N=100000000
# N=1000

echo "Map range"
echo ""
echo "PAM"
echo "range_size, time"
#for M in 1 2 4 8 16 32 64 128 256 512
for M in 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912 1073741824 2147483648 4294967296 8589934592 17179869184
do
	echo -n $M ", "
	numactl -i all ./testParallel-PAM-NA -n $N -m $M -r 10 40 | grep Average | awk '{print $3}' >> outputs/pam_parallel_map_range.out
done

echo ""
echo "U-Pac"
echo "range_size, time"
#for M in 1 2 4 8 16 32 64 128 256 512
for M in 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912 1073741824 2147483648 4294967296 8589934592 17179869184
do
	echo -n $M ", "
	numactl -i all ./testParallel-CPAM-NA -n $N -m $M -r 10 40 | grep Average | awk '{print $3}' >> outputs/upac_parallel_map_range.out
done

echo ""
echo "C-Pac"
echo "range_size, time"
#for M in 1 2 4 8 16 32 64 128 256 512
for M in 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912 1073741824 2147483648 4294967296 8589934592 17179869184
do
	echo -n $M ", "
	numactl -i all ./testParallel-CPAM-NA-Diff -n $N -m $M -r 10 40 | grep Average | awk '{print $3}' >> outputs/cpac_parallel_map_range.out
done
