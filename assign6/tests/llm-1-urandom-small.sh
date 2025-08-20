#!/usr/bin/env bash
set -eu
bytes=256
out=$(mktemp)
./randall -i /dev/urandom "$bytes" >"$out"
[[ $? -eq 0 && $(wc -c <"$out") -eq $bytes ]]
rm "$out"
