---
name: brave/abstract-interface-for-browser-access-from-components
description:
  'When components/ code needs browser-layer behaviour, it declares an abstract
  interface in components/ that browser/ implements, instead of reaching into
  browser/.'
rubric: design
severity: should_fix
---

**Rule.** When code in `components/` needs behaviour that lives in the browser
layer, declare an abstract interface (pure virtual methods) in `components/`,
implement it in `browser/`, and hand the component the implementation through
the abstract type. The component MUST NOT include or call the browser-layer
class itself.

**Why.** The interface inverts the dependency: the component depends only on its
own abstraction, the browser layer depends on the component, and the layering
stays downward-only.

## Examples

Bad:

```cpp
// components/omnibox/browser/brave_autocomplete_controller.cc
#include "brave/browser/autocomplete/brave_omnibox_client_impl.h"
```

Good:

```cpp
// In components/ - abstract interface
class BraveOmniboxClient {
 public:
  virtual bool IsAutocompleteEnabled() = 0;
};

// In browser/ - concrete implementation
class BraveOmniboxClientImpl : public BraveOmniboxClient {
  bool IsAutocompleteEnabled() override;
};
```
