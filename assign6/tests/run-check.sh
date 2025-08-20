#!/usr/bin/env bash
set -eu
bytes=1024                          # 1 KiB sanity check
./randall "$bytes" > tmp.bin
[[ $(wc -c < tmp.bin) == "$bytes" ]] || {
  echo "FAIL: wrong length" >&2; exit 1; }
rm -f tmp.bin
echo "check √"
