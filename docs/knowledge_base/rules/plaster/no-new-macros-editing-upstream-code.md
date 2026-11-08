---
name: brave/no-new-macros-editing-upstream-code
description:
  'New code must never use a macro to edit upstream code (a chromium_src #define
  renaming or rewriting an upstream identifier); the edit is made with a plaster
  rewrite.'
rubric: design
severity: blocking
---

**Rule.** A change MUST NOT add a macro that edits upstream code: a `#define` in
a `chromium_src/` override (or a patch) that renames, redirects or rewrites an
upstream identifier, call or declaration, typically followed by the upstream
`#include` and an `#undef`. This includes macros that only decorate a
declaration ([brave/use-make-virtual-not-define-virtual]). Make the edit with a
substitution in `rewrite/<upstream path>.yaml`; helper code it calls can still
live in the `chromium_src/` override. Macros already in the tree are not in
scope; new ones are.

**Why.** A `#define` rewrites every occurrence of the identifier in all sources
included after it, invisibly and often unintentionally. A plaster changes only
the occurrence it matches, the change is visible in the generated patch, and
`count` (default `1`) fails loudly when its premise stops holding.

## Examples

Bad:

```cpp
// chromium_src/chrome/browser/ui/web_applications/web_app_launch_process.cc
#define NavigateWebAppUsingParams(nav_params) \
  NavigateWebAppInContainerUsingParams(&profile_.get(), *params_, nav_params)
#include <chrome/browser/ui/web_applications/web_app_launch_process.cc>
```

Good:

```yaml
# rewrite/chrome/browser/ui/web_applications/web_app_launch_process.cc.yaml
substitutions:
  - description: |
      Route command line PWA launches through the containers storage partition.
    regex:
      pattern: 'return NavigateWebAppUsingParams(nav_params);'
      replace:
        'return BraveNavigateWebAppUsingParams(&profile_.get(), *params_,
        nav_params);'
```
