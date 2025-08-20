#!/usr/bin/env bash

set -euo pipefail
if git rev-parse --verify -q HEAD^ >/dev/null; then
    git diff HEAD^ HEAD
fi
