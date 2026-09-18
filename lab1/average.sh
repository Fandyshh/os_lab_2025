#!/bin/bash

count=$#

if [ "$count" -eq 0 ]; then
    echo "Count: 0"
    echo "Average: 0"
    exit 0
fi

sum=0
for arg in "$@"; do
    sum=$((sum + arg))
done

avg=$(awk -v s="$sum" -v c="$count" 'BEGIN { printf "%.2f", s / c }')

echo "Count: $count"
echo "Average: $avg"


