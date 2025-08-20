#!/usr/bin/env bash
set -eu
tmp=$(mktemp)
./randall 2048 >"$tmp"            # 256 words

first=$(od -An -N8 -t u8 "$tmp" | tr -d ' ')
if od -An -t u8 "$tmp" | tr -d ' ' | grep -vq "^${first}$"
then rm -f "$tmp"; exit 0          # at least one word differs => PASS
else echo "all 64-bit words equal"; rm -f "$tmp"; exit 1
fi
