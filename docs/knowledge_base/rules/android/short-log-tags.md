---
name: brave/short-log-tags
description:
  An Android Log TAG is a short, descriptive string, not a repeat of the full
  class name.
rubric: readability
severity: nit
---

**Rule.** A `TAG` field for `Log` calls SHOULD be a short, descriptive string
naming the feature or component (`"BraveVPN"`), not the full class name repeated
when that adds no value.

**Why.** A concise tag is easier to filter logcat by and to read; the full class
name is long and adds nothing the message does not already say.

## Examples

Bad:

```java
private static final String TAG = "BraveVpnProfileController";
```

Good:

```java
private static final String TAG = "BraveVPN";
```
