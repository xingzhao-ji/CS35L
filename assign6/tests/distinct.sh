#!/usr/bin/env sh
# Fails if every byte in a 256-byte sample is identical.
set -eu
./randall 256 | od -An -tx1 |
awk '{for(i=1;i<=NF;i++) seen[$i]=1}
     END{exit (length(seen)<2)}'
