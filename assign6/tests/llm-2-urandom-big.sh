#!/usr/bin/env bash
set -eu
bytes=131072          # 128 KiB
out=$(mktemp)
./randall -i /dev/urandom -o 65536 "$bytes" >"$out"
[[ $? -eq 0 && $(wc -c <"$out") -eq $bytes ]]
rm "$out"
