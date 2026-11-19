---
name: brave/ui-text-positive-toggle-labels
description:
  Settings toggles are labelled with the action in the positive, such as "Show
  Home button" rather than "Hide Home button".
rubric: readability
severity: should_fix
---

**Rule.** Label a settings toggle with the positive form of what it does when
on: "Show Home button", not "Hide Home button"; "Allow..." rather than "Don't
block...".

**Why.** A negative label makes "on" mean "off", so users have to reason through
a double negative to know the state.

## Examples

Bad:

```xml
<message name="IDS_SETTINGS_HIDE_HOME_BUTTON" desc="Label of the Home button toggle">
  Hide Home button
</message>
```

Good:

```xml
<message name="IDS_SETTINGS_SHOW_HOME_BUTTON" desc="Label of the Home button toggle">
  Show Home button
</message>
```
