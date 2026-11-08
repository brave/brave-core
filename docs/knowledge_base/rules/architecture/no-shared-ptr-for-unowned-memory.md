---
name: brave/no-shared-ptr-for-unowned-memory
description:
  'Never use std::shared_ptr or std::make_shared, least of all to take ownership
  of memory another object owns; pass a reference or raw pointer to unowned
  objects.'
rubric: safety
severity: blocking
---

**Rule.** NEVER use `std::shared_ptr` or `std::make_shared`; they are banned in
Chromium code. In particular, NEVER wrap memory another object owns in one: use
a reference or a raw pointer for a resource you do not own. If shared ownership
seems necessary, rethink the design first; where it is genuinely unavoidable,
use `scoped_refptr<T>` with a `base::RefCounted` type.

**Why.** A `shared_ptr` over memory owned elsewhere frees that memory while its
owner still references it, which crashes; and Chromium bans `std::shared_ptr` in
favour of `base::RefCounted`.

## Examples

Bad:

```cpp
void Init(network::ResourceRequest& request) {
  auto shared_request = std::make_shared<network::ResourceRequest>(request);
}
```

Good:

```cpp
void Init(const network::ResourceRequest& request) {
  // Use the request directly, don't take ownership.
}
```
