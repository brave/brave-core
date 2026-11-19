---
name: brave/brave-prefix-only-for-chromium-subclasses
description:
  'Class names get the Brave prefix only when they override or subclass a
  Chromium class; Brave-only classes use the feature name.'
rubric: readability
severity: should_fix
---

**Rule.** Add the `Brave` prefix to a class name only when the class overrides
or subclasses a Chromium class
(`BraveOmniboxClientImpl : public ChromeOmniboxClient`). A class that exists
only in Brave is named after its feature, without the prefix:
`WebcompatReporterService`, not `BraveWebcompatReporterService`.

**Why.** The prefix tells a reader that the class stands in for, or extends,
upstream code of the same name. On Brave-only code it says nothing, and suggests
a Chromium counterpart that does not exist.

## Examples

Bad:

```cpp
// A new, Brave-only class.
class BraveWebcompatReporterService { ... };
```

Good:

```cpp
class WebcompatReporterService { ... };

class BraveOmniboxClientImpl : public ChromeOmniboxClient { ... };
```
