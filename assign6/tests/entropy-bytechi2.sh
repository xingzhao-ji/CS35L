#!/usr/bin/env bash
# tests/entropy-bytechi2.sh  –  χ² test on single-byte frequencies
set -eu

BYTES=8192          # sample size
CRIT=1000           # loose threshold that still nails bad RNGs

tmp=$(mktemp)
./randall "$BYTES" >"$tmp" || { rm -f "$tmp"; exit 1; }

od -An -v -t u1 "$tmp" |
awk -v total="$BYTES" -v crit="$CRIT" '
{ for (i = 1; i <= NF; i++) freq[$i]++ }
END {
    expected = total / 256.0           # ← was “exp” — renamed
    chi = 0
    for (i = 0; i < 256; i++) {
        diff = (i in freq ? freq[i] : 0) - expected
        chi += diff * diff / expected
    }
    if (chi > crit) {
        printf "chi² %.0f > %.0f\n", chi, crit
        exit 1
    }
}'
rc=$?; rm -f "$tmp"; exit "$rc"
