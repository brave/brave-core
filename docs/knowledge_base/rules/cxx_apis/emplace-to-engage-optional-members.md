---
name: brave/emplace-to-engage-optional-members
description:
  'Engage a std::optional member with .emplace(...) rather than by assigning a
  freshly constructed value, so the intent to engage it is explicit.'
rubric: readability
severity: nit
---

**Rule.** When a change engages a `std::optional` member by constructing a new
value in it, write `member_.emplace(args...)` instead of `member_ = T(args...)`.

**Why.** `emplace()` reads as "start holding a value" and constructs it in
place; an assignment of a temporary reads like an ordinary update of an existing
value and hides that the member is an optional being engaged.

## Examples

Bad:

```cpp
elapsed_timer_ = base::ElapsedTimer();
```

Good:

```cpp
elapsed_timer_.emplace();
```
