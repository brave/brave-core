---
name: brave/store-only-success-state
description:
  'A member that receives a success-or-error result stores only the success
  value; the error is handled or logged where it occurs.'
rubric: design
severity: should_fix
---

**Rule.** Do not keep a `base::expected<T, E>` (or another success-or-error
type) as a member. Handle or log the error where the result arrives, and store
only the success value, in a `std::optional<T>` when it may be absent.

**Why.** A stored error forces every later reader of the member to handle the
error case again, long after the failure, and with no more context than the
first reader had.

## Examples

Bad:

```cpp
base::expected<ChainMetadata, std::string> chain_metadata_;
```

Good:

```cpp
std::optional<ChainMetadata> chain_metadata_;
```
