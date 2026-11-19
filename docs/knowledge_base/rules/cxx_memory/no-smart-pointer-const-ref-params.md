---
name: brave/no-smart-pointer-const-ref-params
description:
  'Function parameters are never const std::unique_ptr<T>& or const
  scoped_refptr<T>&; take T* or T& when ownership does not change.'
rubric: design
severity: should_fix
---

**Rule.** Do not declare a function parameter as `const std::unique_ptr<T>&` or
`const scoped_refptr<T>&`. When the function does not change ownership, take a
raw pointer (`T*`), or a reference (`T&`) when it must be non-null. The
exception is a lambda passed to an STL algorithm over a container of smart
pointers.

**Why.** Such a parameter forces callers to heap-allocate the object and to hold
it in that particular smart pointer, coupling them to an ownership choice the
function does not use.

## Examples

Bad:

```cpp
void Process(const std::unique_ptr<Foo>& foo);
```

Good:

```cpp
void Process(Foo* foo);
void Process(Foo& foo);  // when non-null is required
```
