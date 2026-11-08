---
name: brave/component-for-mojo-service-targets
description:
  'A Mojo service implementation target in BUILD.gn is a component(), not a
  static_library().'
rubric: design
severity: should_fix
---

**Rule.** Declare the GN target holding a Mojo service implementation with
`component()`. Do not declare it with `static_library()`.

**Why.** Components support dynamic linking in component builds and are the
correct target type for service implementations, which are loaded by the
service's host rather than linked into each user.

## Examples

Bad:

```gn
static_library("foo_service") {
  sources = [ "foo_service_impl.cc" ]
  deps = [ "//brave/components/services/foo/public/mojom" ]
}
```

Good:

```gn
component("foo_service") {
  sources = [ "foo_service_impl.cc" ]
  deps = [ "//brave/components/services/foo/public/mojom" ]
}
```
