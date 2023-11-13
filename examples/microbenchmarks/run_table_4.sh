#!/bin/bash

mkdir -p outputs

echo "size test"
echo "uncompressed Parallel"
echo "1000000, 10000000, 100000000, 1000000000"
# for N in 1000000 10000000 100000000 1000000000
for N in 100 1000 10000 100000
do
	./testParallel-CPAM-NA -n $N -m 0 -r 10 35   | grep "size in bytes"|tail -n 1 | awk '{print $9}' | sed -z 's/\n/,/g' >> outputs/upac_micro_sizes.out
done
echo ""

echo "compressed Parallel"
echo "1000000, 10000000, 100000000, 1000000000"
# for N in 1000000 10000000 100000000 1000000000
for N in 100 1000 10000 100000
do
	./testParallel-CPAM-NA-Diff -n $N -m 0 -r 10 35   | grep "size in bytes" |tail -n 1| awk '{print $9}' | sed -z 's/\n/,/g' >> outputs/cpac_micro_sizes.out
done
echo ""
