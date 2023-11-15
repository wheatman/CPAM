#!/bin/bash

mkdir -p outputs

N=100000000
# N=1000

echo "batch insert"
echo ""
echo "uncompressed Parallel uniform"
#echo "100, 10, 1"
#for M in 100 10 1
echo "10000000, 1000000, 100000, 10000, 1000, 100, 10, 1"
for M in 10000000 1000000 100000 10000 1000 100 10 1
do 
numactl -i all stdbuf -i0 -o0 -e0 ./testParallel-CPAM-NA -n $N -m $M -r 10 37 | grep Average  | awk '{print $3}' | sed -z 's/\n/,/g' >> outputs/upac_parallel_batch.out
done 
echo ""

echo "compressed Parallel uniform"
#echo "100, 10, 1"
#for M in 100 10 1
echo "10000000, 1000000, 100000, 10000, 1000, 100, 10, 1"
for M in 10000000 1000000 100000 10000 1000 100 10 1
do 
numactl -i all stdbuf -i0 -o0 -e0 ./testParallel-CPAM-NA-Diff -n $N -m $M -r 10 37 | grep Average  | awk '{print $3}' | sed -z 's/\n/,/g' >> outputs/cpac_parallel_batch.out
done 
echo ""


echo "PAM uniform"
#echo "100, 10, 1"
#for M in 100 10 1
echo "10000000, 1000000, 100000, 10000, 1000, 100, 10, 1"
for M in 10000000 1000000 100000 10000 1000 100 10 1
do 
numactl -i all stdbuf -i0 -o0 -e0 ./testParallel-PAM-NA -n $N -m $M -r 10 37 | grep Average  | awk '{print $3}' | sed -z 's/\n/,/g' >> outputs/pam_parallel_batch.out
done 
echo ""
