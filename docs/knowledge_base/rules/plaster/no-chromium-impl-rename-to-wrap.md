---
name: brave/no-chromium-impl-rename-to-wrap
description:
  New code never renames an upstream function to _ChromiumImpl just to wrap it;
  Brave's code is inserted into the upstream body with a plaster instead.
rubric: design
severity: blocking
---

**Rule.** NEVER add a rename of an upstream function to `Foo_ChromiumImpl` (by a
`#define` in a `chromium_src/` override, or by a plaster substitution and the
extra declaration it needs) only to wrap the original with Brave code. Insert
that code into the upstream function body with a plaster AST rewriter,
`preempt_function_impl` at its top or `after_function_impl` at its end, in the
plaster of the file holding the definition.

**Why.** The rename splits one function across three places (the renamed
definition, a second declaration in the header, and the replacement in the
override), and the `#define` rewrites every use of the name. An insertion shows
in the generated patch at the function it concerns, and disturbs no declaration.

## Examples

Bad:

```cpp
// chromium_src/foo/foo.cc
#define DoSomething DoSomething_ChromiumImpl
#include <foo/foo.cc>
#undef DoSomething

void Foo::DoSomething() {
  if (!IsBarEnabled(prefs())) {
    return;
  }
  DoSomething_ChromiumImpl();
}
```

Good:

```yaml
# rewrite/foo/foo.cc.yaml
substitutions:
  - description: 'Leave DoSomething alone when Brave has the feature off.'
    preempt_function_impl:
      function_name: Foo::DoSomething
      code: |-
        if (!IsBarEnabled(prefs())) {
          return;
        }
```
