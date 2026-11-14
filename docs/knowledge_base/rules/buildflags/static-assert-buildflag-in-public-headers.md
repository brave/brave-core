---
name: brave/static-assert-buildflag-in-public-headers
description:
  'When a component gets a build flag, its public-facing headers static_assert
  that flag, so including them with the feature disabled fails to compile.'
rubric: correctness
severity: should_fix
---

**Rule.** When a change introduces a build flag for a component, or adds a
public-facing header to a component behind one, the header includes the
component's `buildflags.h` and has a `static_assert(BUILDFLAG(...), "...")` on
that flag, with a message saying the header must not be included when the
feature is disabled.

**Why.** The assert catches accidental inclusion when the feature is disabled at
compile time, with a clear message, instead of letting code compiled in that
configuration use a feature that is not there.

## Examples

Bad:

```cpp
// brave/components/brave_wallet/browser/brave_wallet_service.h
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"

class BraveWalletService : public KeyedService {
```

Good:

```cpp
// brave/components/brave_wallet/browser/brave_wallet_service.h
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
static_assert(BUILDFLAG(ENABLE_BRAVE_WALLET),
              "brave_wallet_service.h must not be included when "
              "enable_brave_wallet is false");
```
