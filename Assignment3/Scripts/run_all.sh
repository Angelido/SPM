#!/bin/bash
for i in {1..10}; do
  ./minizpar -r 0 -C 0 -q 1 small_files/
  ./minizpar -r 0 -D 1 -q 1 small_files/
  ./minizpar -r 0 -C 0 -q 1 big_files/
  ./minizpar -r 0 -D 1 -q 1 big_files/
  ./minizpar -r 1 -C 0 -q 1 nested_files/
  ./minizpar -r 1 -D 1 -q 1 nested_files/
done
