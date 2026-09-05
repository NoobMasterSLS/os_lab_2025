#!/bin/bash

if [ "$#" -eq 0 ]; then
  echo "ошибка: не передано ни одного числа" >&2
  exit 1
fi


args="$*"

result=$(awk -v args="$args" '
BEGIN {
  n = split(args, arr, " ")
  sum = 0
  for (i = 1; i <= n; i++) {
    if (arr[i] !~ /^-?[0-9]+(\.[0-9]+)?$/) {
      print "ошибка: \"" arr[i] "\" не является числом" > "/dev/stderr"
      exit 1
    }
    sum += arr[i]
  }
  avg = sum / n
  printf "%d %f", n, avg
}')

if [ $? -ne 0 ]; then
  exit 1
fi

read -r count average <<< "$result"

echo "количество чисел: $count"
echo "среднее арифметическое: $average"
