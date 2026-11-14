---
name: brave/mojom-forward-headers
description:
  'A header that uses mojom types only by pointer, reference or in signatures
  includes *.mojom-forward.h, leaving the full *.mojom.h to the .cc file.'
rubric: perf
severity: should_fix
---

**Rule.** In a header that only names mojom types in declarations (pointers,
references, `mojom::FooPtr` parameters and return types), include the generated
`*.mojom-forward.h`, and include the full `*.mojom.h` in the `.cc` file that
uses the types. Every mojom target generates the forward header.

**Why.** The full bindings header is large and pulls in many transitive
includes; the forward header declares all the file's types without them, which
cuts compile times and dependencies for every includer. This applies
`brave/forward-declare-in-headers` to mojom.

## Examples

Bad:

```cpp
// prefs_registration.h
#include "brave/components/containers/core/mojom/containers.mojom.h"

namespace containers {
void RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry,
    const std::vector<mojom::ContainerPtr>& default_containers);
}  // namespace containers
```

Good:

```cpp
// prefs_registration.h
#include "brave/components/containers/core/mojom/containers.mojom-forward.h"

// prefs_registration.cc
#include "brave/components/containers/core/mojom/containers.mojom.h"
```
