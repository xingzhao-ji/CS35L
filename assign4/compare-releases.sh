#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 TAG1 TAG2" >&2
  exit 1
fi
git diff "$1" "$2"   
