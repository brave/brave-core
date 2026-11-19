---
name: brave/no-use-after-std-move
description:
  'A variable must never be read or used after it has been std::move()-d, later
  in the same function or in a branch; reinitializing it first is fine.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER read or use a variable after passing it to `std::move()`,
whether later in the same function or after a move inside an `if`/`else` branch.
Watch for strings, vectors, `unique_ptr`s and any other movable type that
appears again after the move. Two exceptions: reinitializing the variable
(assigning to it, or calling `reset()` or `clear()`) before its next use, and a
test of a type's move operations asserting the moved-from state.

**Why.** After a move the source is in a valid but unspecified state, so using
it is a bug for all types. A move inside a branch is no safer: a later refactor
can change the control flow and expose it.

## Examples

Bad:

```cpp
if (auto result = ParseEvent(params, std::move(model_key))) {
  callback.Run(std::move(*result));
}
// ... later in the same function ...
callback.Run(GenerationResultData(std::move(event), model_key));
```

Good:

```cpp
if (auto event = ParseEvent(params)) {
  callback.Run(GenerationResultData(std::move(*event), model_key));
}
```
