---
name: brave/pointer-types-express-ownership
description:
  'Parameter and return pointer types state ownership (T*/T& borrow, unique_ptr
  transfers, scoped_refptr shares); a function must never take ownership of a T*
  parameter.'
rubric: safety
severity: blocking
---

**Rule.** Declare parameter and return types so that they state who owns the
object:

- a `T*` / `T&` parameter: no ownership change; the caller keeps the object
  alive for the call;
- a `std::unique_ptr<T>` parameter: the function takes ownership;
- a `scoped_refptr<T>` parameter: the function may take a ref;
- a `T*` return: only if the caller does _not_ take ownership;
- a `std::unique_ptr<T>` / `scoped_refptr<T>` returned by value: ownership is
  handed off;
- a `const scoped_refptr<T>&` return: the implementation retains ownership.

A function MUST NEVER take ownership of a parameter passed as `T*` (by
`reset()`ting a smart pointer to it, wrapping it, or deleting it): take a
`std::unique_ptr<T>`, into which callers `std::move()` their pointer.

**Why.** Callers should not have to read the implementation to learn who frees
what. A function that takes ownership of a `T*` silently deletes a pointer the
caller still believes it owns, and the transfer is invisible at the call site.

## Examples

Bad:

```cpp
void SetDelegate(Delegate* delegate);  // impl does: delegate_.reset(delegate);
```

Good:

```cpp
void SetDelegate(std::unique_ptr<Delegate> delegate);  // takes ownership
void UseDelegate(Delegate* delegate);                  // borrows only
```
