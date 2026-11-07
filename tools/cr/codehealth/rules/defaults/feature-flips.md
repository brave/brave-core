---
name: defaults/feature-flips
rubric: defaults
severity: regression
---

**Rule.** Every upstream feature that flips from disabled to enabled in the
range is checked against Brave's UI and features, not only privacy.

**Why.** A "Launch … to 100%" or "Enable … by default" is the highest-yield
signal of the sweep: it turns on UI that duplicates or contradicts Brave's, or
promos that assume a Google account.

**How to check.** The candidates are files whose changed lines remove one
default and add another: `FEATURE_(EN|DIS)ABLED_BY_DEFAULT`, the short form
`BASE_FEATURE(kX, ENABLED)` and `enable_if(…)`. Classify each as new, gone or
flipped from the diff with context, carrying the last `BASE_FEATURE(k…` name
across context lines: a flip inside an `#if` doesn't change the name line
(`git show <hash> -U4 -- <file>`). For each disabled → enabled flip, decide
whether Brave should pin it, using `rewrite/` `set_feature_flag_default_state`
plasters (which swap the macro to `BASE_OVERRIDDEN_FEATURE`, so grep both),
`base/compile_overridden_features.inc`, the strings in
`renderer/brave_content_renderer_client.cc` and
`app/feature_defaults_unittest.cc`. For WebUI promos, read the element's show
condition against a signed-out Brave profile. Android Java safe defaults
(`newMutableFlagWithSafeDefault`) flip together with C++.

New upstream features that phone home or add Google-service UI aren't items;
list them for the privacy review.

## Examples

- The Android tips promo; Ctrl+Tab MRU duplicating Brave's MRU cycling; the
  bookmark bar IPH.
- M158: `BASE_FEATURE(kAim3pEntrypoint, ENABLED)`, the short form.
- M158, 8113.1: `kUnoPhase2FollowUp` shows the history page's Google sync promo,
  and `kReadAnythingOmniboxChip` adds a Reading mode chip next to Speedreader.
- M158, 8127.2: `kContextualTasksSidePanel` became enabled on desktop inside an
  `#if`.
- M159, 8140.0: the new `kIPHTabGroupsNonGroupFocusingFeature`, enabled from the
  start, and Java `newFlag(..., FeatureState.ENABLED_IN_PROD)` for
  `kPlatformAgnosticXGeo`.
