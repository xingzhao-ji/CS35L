#!/usr/bin/env bash
set -eu
# Expect failure only when CPU lacks RDRAND.
./randall -i rdrand 1 >/dev/null 2>err || true
if ./randall -i rdrand 1 >/dev/null 2>&1 ; then      # succeeded ⇒ CPU has RDRAND
  exit 0                                             # treat as pass
fi
grep -q "RDRAND not available" err                  # need that message

