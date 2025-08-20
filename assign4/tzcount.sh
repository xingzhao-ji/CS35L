#!/usr/bin/env bash

set -euo pipefail

set -euo pipefail
if (( $# == 0 )); then
  set -- HEAD
fi

git log --pretty=format:'%cd' --date=format:'%z' "$@" |
  awk '{c[$1]++} END {for (z in c) printf "%s %d\n", z, c[z]}' |
  sort -n
