#!/bin/bash

make testParallel-CPAM-NA testParallel-CPAM-NA-Diff testParallel-CPAM-NA-Seq testParallel-CPAM-NA-Diff-Seq testParallel-PAM-NA -j


# total scan time, not used 
# echo "scan time"

# echo "uncompressed Parallel"
# echo "1000000, 10000000, 100000000, 1000000000"
# ./testParallel-CPAM-NA -n 1000000 -m 0 -r 10 34   | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA -n 10000000 -m 0 -r 10 34  | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA -n 100000000 -m 0 -r 10 34 | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA -n 1000000000 -m 0 -r 10 34| grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# echo ""

# echo "compressed Parallel"
# echo "1000000, 10000000, 100000000, 1000000000"
# ./testParallel-CPAM-NA-Diff -n 1000000 -m 0 -r 10 34   | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA-Diff -n 10000000 -m 0 -r 10 34  | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA-Diff -n 100000000 -m 0 -r 10 34 | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA-Diff -n 1000000000 -m 0 -r 10 34| grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# echo ""


# echo "uncompressed serial"
# ./testParallel-CPAM-NA-Seq -n 1000000 -m 0 -r 10 34   | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA-Seq -n 10000000 -m 0 -r 10 34  | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA-Seq -n 100000000 -m 0 -r 10 34 | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA-Seq -n 1000000000 -m 0 -r 10 34| grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# echo ""

# echo "compressed serial"
# ./testParallel-CPAM-NA-Diff-Seq -n 1000000 -m 0 -r 10 34   | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA-Diff-Seq -n 10000000 -m 0 -r 10 34  | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA-Diff-Seq -n 100000000 -m 0 -r 10 34 | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# ./testParallel-CPAM-NA-Diff-Seq -n 1000000000 -m 0 -r 10 34| grep Average | awk '{print $3}' | sed -z 's/\n/,/g'
# echo ""

# echo ""

N=100000000

echo "batch insert"
echo ""
echo "uncompressed Parallel uniform"
echo "10000000, 1000000, 100000, 10000, 1000, 100, 10, 1"
for M in 10000000 1000000 100000 10000 1000 100 10 1
do 
stdbuf -i0 -o0 -e0 ./testParallel-CPAM-NA -n $N -m $M -r 10 37 | grep Average  | awk '{print $3}' | sed -z 's/\n/,/g' 
done 
echo ""

echo "uncompressed Parallel zipfian"
echo "10000000, 1000000, 100000, 10000, 1000, 100, 10, 1"
for M in 10000000 1000000 100000 10000 1000 100 10 1
do 
stdbuf -i0 -o0 -e0 ./testParallel-CPAM-NA -n $N -m $M -r 10 41 | grep Average  | awk '{print $3}' | sed -z 's/\n/,/g' 
done 
echo ""

echo "compressed Parallel uniform"
echo "10000000, 1000000, 100000, 10000, 1000, 100, 10, 1"
for M in 10000000 1000000 100000 10000 1000 100 10 1
do 
stdbuf -i0 -o0 -e0 ./testParallel-CPAM-NA-Diff -n $N -m $M -r 10 37 | grep Average  | awk '{print $3}' | sed -z 's/\n/,/g' 
done 
echo ""
echo "compressed Parallel zipfian"
echo "10000000, 1000000, 100000, 10000, 1000, 100, 10, 1"
for M in 10000000 1000000 100000 10000 1000 100 10 1
do 
stdbuf -i0 -o0 -e0 ./testParallel-CPAM-NA-Diff -n $N -m $M -r 10 41 | grep Average  | awk '{print $3}' | sed -z 's/\n/,/g' 
done 
echo ""


echo "PAM uniform"
echo "10000000, 1000000, 100000, 10000, 1000, 100, 10, 1"
for M in 10000000 1000000 100000 10000 1000 100 10 1
do 
stdbuf -i0 -o0 -e0 ./testParallel-PAM-NA -n $N -m $M -r 10 37 | grep Average  | awk '{print $3}' | sed -z 's/\n/,/g' 
done 
echo ""
echo "compressed Parallel zipfian"
echo "10000000, 1000000, 100000, 10000, 1000, 100, 10, 1"
for M in 10000000 1000000 100000 10000 1000 100 10 1
do 
stdbuf -i0 -o0 -e0 ./testParallel-PAM-NA -n $N -m $M -r 10 41 | grep Average  | awk '{print $3}' | sed -z 's/\n/,/g' 
done 
echo ""

# serial batch inserts, not used
# echo "uncompressed Serial"
# echo "10000000, 1000000, 100000, 10000, 1000, 100, 10, 1"
# for M in 10000000 1000000 100000 10000 1000 100 10 1
# do 
# stdbuf -i0 -o0 -e0 ./testParallel-CPAM-NA-Seq -n $N -m $M -r 10 37        | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# done 
# echo ""

# echo "compressed Serial"
# echo "10000000, 1000000, 100000, 10000, 1000, 100, 10, 1"
# for M in 10000000 1000000 100000 10000 1000 100 10 1
# do 
# stdbuf -i0 -o0 -e0 ./testParallel-CPAM-NA-Diff-Seq -n $N -m $M -r 10 37        | grep Average | awk '{print $3}' | sed -z 's/\n/,/g' 
# done 
echo ""

echo ""

echo "size test"
echo "uncompressed Parallel"
echo "1000000, 10000000, 100000000, 1000000000"
for N in 1000000 10000000 100000000 1000000000
do 
./testParallel-CPAM-NA -n $N -m 0 -r 10 35   | grep "size in bytes"|tail -n 1 | awk '{print $9}' | sed -z 's/\n/,/g' 
done
echo ""

echo "compressed Parallel"
echo "1000000, 10000000, 100000000, 1000000000"
for N in 1000000 10000000 100000000 1000000000
do 
./testParallel-CPAM-NA-Diff -n $N -m 0 -r 10 35   | grep "size in bytes" |tail -n 1| awk '{print $9}' | sed -z 's/\n/,/g' 
done
echo ""



N=100000000

echo "Map range"
echo ""
echo "PAM"
echo "range_size, time"
for M in 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912 1073741824 2147483648 4294967296 8589934592 17179869184
do
echo -n $M ", "
numactl -i all ./testParallel-PAM-NA -n $N -m $M -r 10 40 | grep Average | awk '{print $3}'
done

echo ""
echo "U-Pac"
echo "range_size, time"
for M in 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912 1073741824 2147483648 4294967296 8589934592 17179869184
do
echo -n $M ", "
numactl -i all ./testParallel-CPAM-NA -n $N -m $M -r 10 40 | grep Average | awk '{print $3}'
done

echo ""
echo "C-Pac"
echo "range_size, time"
for M in 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912 1073741824 2147483648 4294967296 8589934592 17179869184
do
echo -n $M ", "
numactl -i all ./testParallel-CPAM-NA-Diff -n $N -m $M -r 10 40 | grep Average | awk '{print $3}'
done
