---
name: overlap/plaster-intent
rubric: overlap
severity: regression
---

**Rule.** Each touched plaster still does what its descriptions say, against the
pristine function at the new tag.

**Why.** An anchor that matches only proves the text is there. A plaster that
enumerates methods misses new ones, a sibling path can compute the same value
unpatched, and a value built from Brave constants may no longer be accepted.

**How to check.** For each candidate touching a plastered file, read the
plaster's descriptions against the pristine function at the new tag. For a
plaster that enumerates methods (an alternation with a `count`), diff the
class's method list at both tags. Look for a sibling path that computes the same
value. When a plaster returns a value built from Brave constants, read what the
callee now accepts; grepping Brave for removed flag names finds these.

## Examples

- M157, 8097.1: `GlicEnabling.isEnabledAndConsentForProfile`, missed by a method
  alternation.
- M158: the .onion Origin is opaque in `MaybeSetOriginHeader` but not on the
  CORS preflight.
- M158: `Cup` `CHECK`s for an ML-DSA-44 key, and Brave's CUP key is ECDSA (found
  by grepping Brave for `kPqcCupSigning`).
