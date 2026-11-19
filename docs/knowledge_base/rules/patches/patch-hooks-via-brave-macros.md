---
name: brave/patch-hooks-via-brave-macros
description:
  A hand-written C++ patch inserts a BRAVE_ macro defined in the chromium_src
  override instead of inline Brave logic, so behaviour changes in brave-core
  without changing the patch.
rubric: design
severity: should_fix
---

**Rule.** A hand-written patch to upstream C++ MUST NOT inline Brave's logic
into the upstream code. It adds a single line holding a `BRAVE_*` macro (and,
where the file does not already pick it up, the include of the header that
defines it); the macro's body lives in the file's `chromium_src/` override.
Where a plaster can make the change instead, a new macro is not added
([brave/no-new-macros-editing-upstream-code]).

**Why.** With a hook macro, what Brave does at that point changes in brave-core
alone and the patch stays as it is. Inline logic in a patch has to be edited,
and can conflict, every time Brave's behaviour changes.

## Examples

Bad:

```diff
+  if (permission == AUTOPLAY) return true;
```

Good:

```diff
+#include "brave/chromium_src/path/to/override.h"
+BRAVE_PERMISSION_CONTROLLER_IMPL_METHOD
```
