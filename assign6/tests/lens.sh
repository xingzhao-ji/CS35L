#!/usr/bin/env sh
set -eu
[ "$(./randall 4096 | wc -c)" -eq 4096 ]
