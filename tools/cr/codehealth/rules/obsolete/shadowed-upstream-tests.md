---
name: obsolete/shadowed-upstream-tests
rubric: obsolete
severity: regression
---

**Rule.** A `chromium_src` test on an upstream test's path includes the upstream
file, unless it is meant to replace every test in it.

**Why.** The build compiles a `chromium_src` file in place of its upstream
namesake, so a Brave-only test put there silently drops all of upstream's tests
in that file, including those the range adds or extends.

**How to check.** The candidates are
`vpython3 tools/cr/codehealth/scripts/obsolete.py shadowed-tests`, rows
`shadowed` with the range's commits to the upstream test (`--debt` lists those
the range left alone). Move a Brave-only test to a Brave path and its
`test/BUILD.gn` entry, or include the upstream file.

## Examples

- M160, 8184.1:
  `chromium_src/components/omnibox/browser/autocomplete_match_unittest.cc` hides
  `AutocompleteMatchTest`, which ebb1d796c461 extends.
