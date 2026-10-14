---
name: obsolete/test-filters
rubric: obsolete
severity: obsolete
---

**Rule.** Every test filter entry names a test that exists at the new tag, in
the form gtest matches.

**Why.** A negative filter matches the full name exactly, so upstream turning a
`TEST_F` into a `TEST_P` silently re-enables a filtered test (`All/…/0`), and
the reverse makes the `All/…` entry match nothing. A rename leaves the entry
stale, and a class split copies tests into a new suite the filters don't name.

**How to check.** The candidates are
`vpython3 tools/cr/codehealth/scripts/obsolete.py test-filters`, which indexes
the `TEST*` and `INSTANTIATE_*` macros at both tags (stripping `MAYBE_`,
`DISABLED_` and `PRE_`), and lists entries that are gone in the range (with the
suites that now hold the name), that became `TEST_P` or `TEST_F`, whose test
lost its `DISABLED_` prefix upstream (a "flaky upstream" filter to drop), or
whose test was copied into a new suite. For an entry gone in the range,
`git log -S<name> $A..$B` finds the commit; read it, because a renamed test
needs its filter retargeted, not deleted. Entries matching nothing at either tag
predate the range: they're counted, and `--debt` lists them.
`test/filters/generated/` is fixed by rerunning
`tools/chromium_tests_analysis/update-upstream-flake-filters.py` rather than by
hand. `tools/cr/prune_test_filters.py` does the existence check with built
binaries, but it deletes renamed entries.

## Examples

- M158, 8117.1: flag clean-ups turned
  `ForceSigninProfilePickerCreationFlowBrowserTest` into `_F`, and folded
  `CanOfferSigninWithSigninToSyncFeatureTest` into `CanOfferSigninTest`.
- M157: renamed `TransportSecurityStateTest` cases.
- M158, 8127.2: `KeyPinningStateChrome.PreloadedProductionStaticPins` and four
  more, copied by a class split.
- M157, 8093.0: `PinnedToolbarActionsContainerTest`, and five tab sharing suites
  that became `All/…/N`.
- The payment handler camera test, in `test/filters/generated/`.
- M159, 8133.1: `GlicApiActorTest`, instantiated with an empty prefix on a split
  `INSTANTIATE_TEST_SUITE_P(` line, took tests out of a `-GlicApiTest*.*`
  filter.
