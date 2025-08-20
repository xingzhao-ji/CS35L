#!/usr/bin/env bash
set -eu
bytes=1000
out=$(mktemp)
./randall -i mrand48_r -o 1 "$bytes" >"$out"
[[ $? -eq 0 && $(wc -c <"$out") -eq $bytes ]]
rm "$out"
