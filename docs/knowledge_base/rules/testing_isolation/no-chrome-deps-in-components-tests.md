---
name: brave/no-chrome-deps-in-components-tests
description:
  Tests under components/ must not include or depend on //chrome; they construct
  what they need directly.
rubric: design
severity: blocking
---

**Rule.** A test under `components/` MUST NOT include a `chrome/` header or add
a `//chrome` dependency to its test target. When it needs an object Chrome
infrastructure normally creates (a `HostContentSettingsMap`, a pref service, a
keyed service), the test constructs it directly instead of fetching it through a
`chrome/` factory.

**Why.** `//components` sits below `//chrome` in the layering, so a component
test that depends on `//chrome` creates an upward dependency, can no longer
build in the component's own test target, and pulls the whole browser layer into
what should be an isolated test.

## Examples

Bad:

```cpp
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"

auto* settings_map = HostContentSettingsMapFactory::GetForProfile(profile);
```

Good:

```cpp
auto settings_map = base::MakeRefCounted<HostContentSettingsMap>(
    &pref_service_, /*is_off_the_record=*/false,
    /*store_last_modified=*/false, /*restore_session=*/false,
    /*should_record_metrics=*/false);
```
