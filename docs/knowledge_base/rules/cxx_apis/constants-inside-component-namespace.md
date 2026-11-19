---
name: brave/constants-inside-component-namespace
description:
  "File-local constants go in an anonymous namespace nested inside the
  component's namespace, not in an anonymous namespace at global scope."
rubric: readability
severity: nit
---

**Rule.** In a `.cc` file whose code lives in a component namespace (such as
`brave_stats`), put its file-local constants in an anonymous namespace inside
that namespace, not in an anonymous namespace at file scope before it.

**Why.** Inside the component's namespace, the constants are found by the
component's code without qualification, and cannot collide with or be mistaken
for global names. Keeping the file's declarations in one namespace also keeps
its structure uniform.

## Examples

Bad:

```cpp
namespace {
constexpr int kMaxRetries = 3;
}  // namespace

namespace brave_stats {
// ...
}  // namespace brave_stats
```

Good:

```cpp
namespace brave_stats {
namespace {
constexpr int kMaxRetries = 3;
}  // namespace
// ...
}  // namespace brave_stats
```
