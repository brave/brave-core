---
name: brave/scope-features-to-one-ownership-primitive
description:
  'Each feature is owned by exactly one of Tab (TabFeatures), Browser Window
  (BrowserWindowFeatures), Profile (keyed service) or Global (GlobalFeatures),
  the narrowest that fits its data and lifetime.'
rubric: design
severity: blocking
---

**Rule.** A new feature MUST be owned by exactly one ownership primitive, the
narrowest that fits its data and lifetime:

- Tab: `TabFeatures` (`BraveTabFeatures`), for find-in-page or page actions;
- Browser Window: `BrowserWindowFeatures`, for the omnibox or vertical tabs;
- Profile: a keyed service built by its keyed service factory, for Rewards or
  Wallet;
- Global: `GlobalFeatures`, only for process-wide features spanning profiles.
  These are rare.

Flag a feature with no clear owner, one split across several primitives, or one
scoped wider than its data needs.

**Why.** The owner fixes the feature's lifetime and what it may depend on, as
Chromium's browser design principles set out; a feature scoped too widely holds
state for objects it outlives, or that other instances share.

## Examples

Bad:

```cpp
// Per-tab state kept in a profile-wide service.
class FooService : public KeyedService {
  std::map<tabs::TabHandle, FooState> state_by_tab_;
};
```

Good:

```cpp
// Owned by BraveTabFeatures, one per tab.
class FooTabFeature {
  FooState state_;
};
```
