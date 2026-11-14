---
name: brave/ios-feature-flags-gate-registration
description:
  'A feature flag check decides whether a handler or observer is registered,
  rather than running inside the handler on every event.'
rubric: perf
severity: should_fix
---

**Rule.** Check a feature flag where the feature's handler, observer or script
is registered, and register it only when the feature is enabled. Do not register
it unconditionally and check the flag at the top of each callback.

**Why.** A handler registered for a disabled feature still runs on every event
just to return early; gating the registration costs nothing when the feature is
off.

## Examples

Bad:

```swift
func handle(event: Event) {
  guard FeatureList.kMyFeature.enabled else { return }
  // handle event...
}
```

Good:

```swift
if FeatureList.kMyFeature.enabled {
  eventBus.register(handler: MyFeatureHandler())
}
```
