---
name: brave/hide-ui-for-enforced-policies
description:
  'UI for a feature an enforced (mandatory) admin policy disables is hidden
  entirely, not greyed out; for a recommended policy it stays visible.'
rubric: design
severity: should_fix
---

**Rule.** When an admin policy with ENFORCED (mandatory) enforcement disables a
feature, hide the feature's UI entirely; do not just disable or grey out its
controls. When the policy is only RECOMMENDED, keep the UI visible, since the
user can override it. On macOS, policies set with `defaults write` are
RECOMMENDED, not MANDATORY.

**Why.** Greyed-out controls for a feature the user can never use are clutter
and confusion; hiding them under a recommended policy would take away a choice
the user still has.

## Examples

Bad:

```cpp
if (IsFeatureManaged()) {
  button->SetEnabled(false);  // greyed out but visible
}
```

Good:

```cpp
if (IsFeatureManaged() && enforcement == Enforcement::kEnforced) {
  section->SetVisible(false);  // completely hidden
}
```
