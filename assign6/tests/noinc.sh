#!/usr/bin/env bash
# tests/noinc.sh – fails only for the 0x00..0xFF repeating RNG
set -eu

tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT
./randall 257 >"$tmp"                || exit 1   # any error → suspicious

od -An -v -tu1 "$tmp" |
awk '
{
    for (i = 1; i <= NF; i++) bytes[cnt++] = $i
}
END {
    if (cnt < 257) exit 1            # shouldn’t happen

    for (i = 1; i < 257; i++)
        if (bytes[i] != ((bytes[i-1] + 1) % 256))
            exit 0                  # pattern breaks → RNG is fine

    print "detected +1 pattern"      # perfect +1 sequence found
    exit 1
}'
