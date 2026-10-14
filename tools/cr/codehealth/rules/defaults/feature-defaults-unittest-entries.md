---
name: defaults/feature-defaults-unittest-entries
rubric: defaults
severity: regression
---

**Rule.** Each `app/feature_defaults_unittest.cc` entry for a feature that
flipped in the range has an override behind it.

**Why.** An entry with no plaster or `chromium_src` override only records
upstream's default: when upstream flips it, the test fails while the feature
ships.

**How to check.** The candidates are flips of features Brave names. For each one
in `app/feature_defaults_unittest.cc`, look for its override in `rewrite/`
(`set_feature_flag_default_state`) and `chromium_src`.

## Examples

- M158, 8117.1: `kAndroidPageInfoAsAppMenuItem` moved page info into the Android
  app menu.
