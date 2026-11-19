---
name: brave/addview-over-replacing-upstream-layouts
description:
  Android UI is customized by adding views programmatically with addView()
  rather than by replacing an entire upstream XML layout, unless that would be
  equally invasive and the trade-off is documented.
rubric: change_scope
severity: should_fix
---

**Rule.** To customize an upstream Android screen, add the Brave views
programmatically (`addView()` from Java) rather than overriding the whole
upstream XML layout file with a Brave copy. Accept a full layout replacement
only when adding the views programmatically would require extending final
classes or be equally invasive, and the change documents that trade-off.

**Why.** A replaced layout is a copy of upstream's that every Chromium upgrade
has to reconcile with upstream's changes to it. Views added from code leave the
upstream layout untouched.

## Examples

Bad:

```gn
# android/brave_java_resources.gni: a full copy of the upstream layout.
"java/res/layout/foo_layout.xml",
```

Good:

```java
ViewGroup container = findViewById(R.id.foo_container);
container.addView(mBraveFooButton);
```
