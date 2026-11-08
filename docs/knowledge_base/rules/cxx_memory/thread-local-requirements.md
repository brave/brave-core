---
name: brave/thread-local-requirements
description:
  'A thread_local must be trivially destructible, not exported, ABSL_CONST_INIT
  at namespace or class scope, and not constructed in OOM handlers; otherwise
  use base::ThreadLocalOwnedPointer.'
rubric: correctness
severity: blocking
---

**Rule.** A `thread_local` variable MUST:

- have a type satisfying `std::is_trivially_destructible_v<T>`, which excludes
  `raw_ptr`;
- not be exported through `COMPONENT_EXPORT`;
- be annotated `ABSL_CONST_INIT` when it is at namespace or class scope;
- not be constructed in an OOM handler, since construction may allocate on
  POSIX.

If these requirements cannot be met, use `base::ThreadLocalOwnedPointer`
instead.

**Why.** These are the requirements of Chromium's C++ style guide: non-trivial
destructors of thread-locals caused problems at thread exit, exported
thread-locals miscompile on some platforms, and construction can allocate.

## Examples

Bad:

```cpp
thread_local raw_ptr<Foo> g_current_foo = nullptr;
```

Good:

```cpp
ABSL_CONST_INIT thread_local Foo* g_current_foo = nullptr;
```
