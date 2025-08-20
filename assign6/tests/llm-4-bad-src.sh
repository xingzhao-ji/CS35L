#!/usr/bin/env bash
set -eu
./randall -i bogus 10 >/dev/null 2>err && exit 1
grep -qi "usage" err
