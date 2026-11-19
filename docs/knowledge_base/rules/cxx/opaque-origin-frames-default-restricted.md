---
name: brave/opaque-origin-frames-default-restricted
description:
  "A frame with an opaque origin defaults to restricted; a decision it needs
  uses the request's initiator_origin or a verified non-opaque ancestor."
rubric: security
severity: blocking
---

**Rule.** When the origin a decision is based on is opaque (`file://`, `data:`
and sandboxed frames), default to the restricted behaviour. If a decision is
still needed:

- for a network request, use the request's `initiator_origin`;
- otherwise, where it applies, walk up to the nearest ancestor frame with a
  non-opaque origin and use its origin, after verifying that this is really the
  origin the decision should rest on.

Remember that `about:blank` and `about:srcdoc` frames embedded in an opaque
context are opaque themselves.

**Why.** An opaque origin can read as a `null` origin, leaving nothing to key a
policy on, while the same frame may also carry a tuple origin. Granting it the
capabilities of whatever origin happens to be at hand opens a bypass.

## Examples

Bad:

```cpp
const url::Origin& origin = render_frame_host->GetLastCommittedOrigin();
// An opaque origin gets the default-allow branch.
if (!IsBlockedForOrigin(origin)) { AllowCapability(); }
```

Good:

```cpp
const url::Origin& origin = render_frame_host->GetLastCommittedOrigin();
if (origin.opaque()) {
  return;  // Restricted by default.
}
if (!IsBlockedForOrigin(origin)) {
  AllowCapability();
}
```
