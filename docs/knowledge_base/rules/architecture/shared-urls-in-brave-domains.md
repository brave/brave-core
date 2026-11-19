---
name: brave/shared-urls-in-brave-domains
description:
  'A URL constant or URL builder shared by several components without a single
  owner goes in brave_domains:urls, not duplicated per component nor added to
  brave_constants.h.'
rubric: design
severity: should_fix
---

**Rule.** A URL constant or URL-building function used by several components,
none of which clearly owns it, goes in the `brave_domains:urls` target
(`brave_domains/urls.h`). Do not copy it into each component, and do not add it
to the legacy `components/constants/brave_constants.h`. Build environment-aware
URLs with `GetServicesDomain()` from `brave_domains/service_domains.h`.

**Why.** Duplicated URLs drift apart, and one hard-coded copy misses the staging
or development environment the others honour.

## Examples

Bad:

```cpp
// components/brave_rewards/common/constants.h
inline constexpr char kGate3Url[] = "https://gate3.brave.com";
// components/brave_wallet/common/constants.h
inline constexpr char kGate3Url[] = "https://gate3.brave.com";
```

Good:

```cpp
// brave_domains/urls.h
GURL GetGate3URL();  // environment-aware, uses GetServicesDomain()
```
