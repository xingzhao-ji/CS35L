#!/usr/bin/env sh
# tests/check-all.sh  –  single-file byte-count + randomness test suite
#  • verifies exact SIZE bytes
#  • rejects streams with ≤1 distinct byte
#  • rejects 0x00-0xFF repeating pattern (perfect +1 sequence)
#  • χ² byte-frequency test, df=255, α≈10⁻⁴  (crit = 360)

###############################################################################
# Configuration
###############################################################################
BYTES=8192            # 8 KiB sample (fast; enough for strong χ² power)
CHI_CRIT=360          # χ² critical value (df=255, 99.99 % one-sided)

###############################################################################
# Generate sample
###############################################################################
tmp=$(mktemp); trap 'rm -f "$tmp"' EXIT
./randall "$BYTES" >"$tmp"      || { echo "randall failed"; exit 1; }

###############################################################################
# Quick byte-count smoke test
###############################################################################
[ "$(wc -c <"$tmp")" -eq "$BYTES" ] || { echo "length mismatch"; exit 1; }

###############################################################################
# Combined distinct-byte, +1-pattern, χ² tests  (single awk pass, INT math)
###############################################################################
od -An -v -tu1 "$tmp" |
awk -v n="$BYTES" -v chi_crit="$CHI_CRIT" '
{
  for (i = 1; i <= NF; i++) {
    b = $i + 0
    freq[b]++
    if (!seen[b]++) {                          # distinct-byte counter
      if (++distinct > 1) distinct_enough = 1
    }

    if (++idx == 1) prev = b                   # +1-pattern detector
    else if (idx < 258 && pattern_ok == 0) {
      if (b != ((prev + 1) % 256)) pattern_ok = 1
      prev = b
    }
  }
}
END {
  # ----- fail if all bytes identical -----
  if (!distinct_enough) { print "all bytes identical"; exit 1 }

  # ----- fail on perfect 0x00-0xFF repeat pattern -----
  if (pattern_ok == 0) { print "detected +1 pattern"; exit 1 }

  # ----- χ² frequency test (integer arithmetic) -----
  expected = n / 256                      # integer expected count
  num = 0                                 # numerator Σ(256*obs − n)²
  for (i = 0; i < 256; i++) {
    diff = 256 * freq[i] - n              # diff scaled by 256
    num += diff * diff
  }
  # χ² = num / (256*n); compare without floats
  if (num > chi_crit * 256 * n) {
    printf "χ² %d > %d\n", num, chi_crit * 256 * n
    exit 1
  }

  exit 0                                   # all tests passed
}'
