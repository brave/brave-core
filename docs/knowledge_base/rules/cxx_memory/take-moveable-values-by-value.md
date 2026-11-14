---
name: brave/take-moveable-values-by-value
description:
  'A moveable type is passed and returned by value; std::unique_ptr<T> is kept
  for types that are not moveable, such as polymorphic ones.'
rubric: design
severity: should_fix
---

**Rule.** To hand a moveable type to a function, take it by value
(`void Consume(T value)`, called as `Consume(std::move(t))`), and to hand it
back, return a plain `T`. Use `std::unique_ptr<T>` only when the type is not
moveable, typically a polymorphic type held by a base pointer. Do not use it,
either, to express "maybe a value": that is `std::optional<T>`.

**Why.** Much of the `std::unique_ptr<T>` in existing code is historical: before
move semantics it was the only way to hand an object around, and before
`std::optional<T>` it doubled as the optional value. For a moveable type it only
adds a heap allocation and an indirection.

## Examples

Bad:

```cpp
std::unique_ptr<std::vector<Bookmark>> LoadBookmarks();
void Save(std::unique_ptr<base::Value::Dict> dict);
```

Good:

```cpp
std::vector<Bookmark> LoadBookmarks();
void Save(base::Value::Dict dict);
```
