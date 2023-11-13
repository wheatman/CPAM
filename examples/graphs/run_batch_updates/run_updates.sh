#!/bin/bash

mkdir -p outputs

./run_batch_updates-CPAM-CPAM-Diff -s -rounds 1 ../../../../../graphs/fs.adj 2>&1 >> outputs/batch_inserts.out
