---
name: brave/prefer-single-ownership-over-ref-counting
description:
  'New reference-counted classes are avoided in favor of single ownership; when
  refcounting is necessary the class is single-sequence with WeakPtr-bound
  callbacks.'
rubric: design
severity: should_fix
---

**Rule.** Do not make a class reference-counted (`base::RefCounted`,
`base::RefCountedThreadSafe`, `scoped_refptr`) when it can be redesigned to have
a single owner holding it in a `std::unique_ptr`. When refcounting does seem
necessary, restrict the class to a single thread or sequence, proxy calls to
that sequence with `PostTask()`, and bind callbacks with `base::BindOnce()` and
a `WeakPtr` so they are cancelled on destruction.

**Why.** With reference counting, no single place decides when the object is
destroyed or on which thread, which makes ownership and destruction order hard
to reason about, especially with multiple threads.

## Examples

Bad:

```cpp
class Downloader : public base::RefCountedThreadSafe<Downloader> {};
scoped_refptr<Downloader> downloader_ = base::MakeRefCounted<Downloader>();
```

Good:

```cpp
class Downloader {};
std::unique_ptr<Downloader> downloader_ = std::make_unique<Downloader>();
```
