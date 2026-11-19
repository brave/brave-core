---
name: brave/no-std-move-on-return
description:
  'return std::move(local) is never written for a local or a temporary; it
  suppresses copy elision. Write return local;.'
rubric: perf
severity: should_fix
---

**Rule.** Do not wrap a local variable or a temporary in `std::move()` in a
`return` statement. Write `return foo;`.

**Why.** `return std::move(foo);` suppresses copy elision, so it produces a move
the compiler would otherwise have elided entirely. Returning the local by name
already moves it when elision is not possible.

## Examples

Bad:

```cpp
std::vector<Item> GetItems() {
  std::vector<Item> items = LoadItems();
  return std::move(items);
}
```

Good:

```cpp
std::vector<Item> GetItems() {
  std::vector<Item> items = LoadItems();
  return items;
}
```
