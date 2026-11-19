---
name: brave/prefer-chromium-src-override-over-patch
description:
  A hand-written patch to upstream code is a last resort; whatever a
  chromium_src override can do (intercepting behaviour, adding a class method)
  must be done with one instead.
rubric: design
severity: should_fix
---

**Rule.** NEVER add a hand-written patch to upstream C++, TypeScript, mojom or
Python for a change a `chromium_src/` override can make: intercepting behaviour,
adding a class method, or defining code next to the upstream source. Keep
patches for what an override cannot reach, such as GN files. To make a method
`virtual`, use the `make_virtual` plaster rewriter
([brave/use-make-virtual-not-define-virtual]). This concerns hand-written
patches only: a patch generated from a `rewrite/<path>.yaml` plaster is reviewed
through its plaster.

**Why.** Patches are harder to maintain, more likely to conflict on a Chromium
rebase, and harder to review than an override, which keeps Brave's code in a
file of its own.

## Examples

Bad:

```diff
// patches/components-permissions-permissions_client.cc.patch
+bool PermissionsClient::BraveCanBypassEmbeddingOriginCheck(
+    ContentSettingsType type,
+    const GURL& requesting_origin,
+    const GURL& embedding_origin) {
+  return CanBypassEmbeddingOriginCheck(requesting_origin, embedding_origin);
+}
```

Good:

```cpp
// chromium_src/components/permissions/permissions_client.cc
#include <components/permissions/permissions_client.cc>

namespace permissions {

bool PermissionsClient::BraveCanBypassEmbeddingOriginCheck(
    ContentSettingsType type,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  return CanBypassEmbeddingOriginCheck(requesting_origin, embedding_origin);
}

}  // namespace permissions
```
