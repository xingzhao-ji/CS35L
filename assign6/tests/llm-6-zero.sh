#!/usr/bin/env bash
set -eu
out=$(mktemp)
./randall 0 >"$out"
[[ $? -eq 0 && $(wc -c <"$out") -eq 0 ]]
rm "$out"
