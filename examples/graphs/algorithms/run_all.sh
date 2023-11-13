#!/bin/bash
#make -j
mkdir -p outputs

ROUNDS=10
GRAPHPATH=../../../../../graphs
OUTPATH=./outputs

function run {
  echo $1
  echo sizes
  numactl -i all ./BFS-CPAM -s -rounds 1 -src $2 $1 | grep "size in bytes"

  #echo "BFS"
  #numactl -i all ./BFS-CPAM -s -rounds $ROUNDS -src $2 $1 | grep "time per iter"

  echo "PR"
  numactl -i all ./PR-CPAM -s -rounds $ROUNDS -src $2 $1 | grep "time per iter"

  echo "BC"
  numactl -i all ./BC-CPAM -s -rounds $ROUNDS -src $2 $1 | grep "time per iter"
  echo "CC"
  numactl -i all ./CC-CPAM -s -rounds $ROUNDS -src $2 $1 | grep "time per iter"

}

run  $GRAPHPATH/lj.adj    0 2>&1 >> $OUTPATH/lj.out
run  $GRAPHPATH/co.adj  1000  2>&1 >> $OUTPATH/co.out
run  $GRAPHPATH/er.adj                 0      2>&1 >> $OUTPATH/er.out
run  $GRAPHPATH/tw.adj            12      2>&1 >> $OUTPATH/tw.out
run  $GRAPHPATH/fs.adj            100000     2>&1 >> $OUTPATH/fs.out
