---
name: brave/no-new-code-in-l10n
description:
  "Do not add files, classes or new uses of the deprecated components/l10n/; use
  Chromium's localization functionality instead."
rubric: design
severity: should_fix
---

**Rule.** Do not add files, classes or functions to `components/l10n/`, and do
not make new code depend on it (`brave_l10n::`, `brave/components/l10n/...`).
Use Chromium's localization functionality instead. If Chromium has no equivalent
for the use case, the alternatives are discussed before `components/l10n/` is
used.

**Why.** `components/l10n/` is deprecated: new code there or new callers of it
grow what has to be migrated away.

## Examples

Bad:

```cpp
#include "brave/components/l10n/common/locale_util.h"
const std::string country = brave_l10n::GetDefaultISOCountryCodeString();
```

Good:

```cpp
#include "components/country_codes/country_codes.h"
const country_codes::CountryId country = country_codes::GetCurrentCountryID();
```
