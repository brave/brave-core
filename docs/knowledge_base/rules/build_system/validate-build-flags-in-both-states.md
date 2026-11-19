---
name: brave/validate-build-flags-in-both-states
description:
  'A change adding a build flag is checked with gn_check, the unit, component
  and browser tests and presubmit, with the flag both enabled and disabled.'
rubric: testing
severity: should_fix
---

**Rule.** When a change adds a build flag (a `declare_args()` arg exposed
through a `buildflag_header`), it is validated with the flag enabled AND with it
disabled: `gn_check`, unit tests, component tests, browser tests and presubmit,
in both states. When the change gives no sign of the disabled state having been
built, ask for it.

**Why.** Issues frequently only appear in the disabled state: deps, includes and
test sources that are only correct when the feature is on fail in the
configuration that CI and most developers do not build by default.

## Examples

Bad:

```gn
buildflag_header("buildflags") {
  header = "buildflags.h"
  flags = [ "ENABLE_BRAVE_FOO=$enable_brave_foo" ]
}
# Only built and tested with enable_brave_foo = true.
```

Good:

```sh
# Both states pass before review.
gn gen out/foo_off --args='enable_brave_foo=false'
gn check out/foo_off
```
