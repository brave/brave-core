---
name: brave/no-define-final
description:
  'Never #define final away in a chromium_src override; remove final from the
  upstream declaration with a patch (the drop_final rewriter) or find another
  approach.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER `#define final` (to nothing or anything else), or `#undef` it,
in a `chromium_src/` override. When a Brave subclass needs an upstream class
that is `final`, remove `final` from that one declaration with a patch, written
as a `drop_final` substitution in `rewrite/<path>.yaml`, or find an approach
that does not need the subclass.

**Why.** Redefining a keyword is undefined behaviour under the C++ standard, and
the macro is viral: it strips `final` from every class and method in everything
included after it, which breaks unrelated code that relies on `final` in other
contexts.

## Examples

Bad:

```cpp
#define final
#include "src/chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#undef final
```

Good:

```yaml
# rewrite/chrome/browser/ui/views/side_panel/side_panel_coordinator.h.yaml
substitutions:
  - description: 'Let BraveSidePanelCoordinator subclass SidePanelCoordinator.'
    drop_final:
      class_name: SidePanelCoordinator
```
