---
name: brave/no-view-types-bound-into-callbacks
description:
  'base::BindOnce/BindRepeating must not bind a std::string_view or naked
  base::span unless its storage is static; bind an owning copy or a
  base::raw_span.'
rubric: safety
severity: blocking
---

**Rule.** NEVER pass a non-owning view such as `std::string_view` or
`base::span<T>` as a bound argument of `base::BindOnce` / `base::BindRepeating`.
Bind an owning copy (`std::string(name)`, `std::vector<T>`) instead, and when
the callback target keeps the data, make it take the owning type by value.
Binding a view is acceptable only when it is a `base::raw_span<T>` (not a naked
`base::span<T>`), or when its storage is static (a span over a static or
`constexpr` array, or a `std::string_view` of a string literal).

**Why.** `base::Bind*` rejects unsafe raw pointers and references at compile
time but not view types. It copies a view by value, which copies only its
pointer and length, so if nothing keeps the backing storage alive until the
callback runs, the view dangles and the callback reads freed memory.

## Examples

Bad:

```cpp
void Schedule(std::string_view name) {
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&MyClass::OnReady, weak_factory_.GetWeakPtr(), name));
}
```

Good:

```cpp
void Schedule(std::string_view name) {
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&MyClass::OnReady, weak_factory_.GetWeakPtr(),
                     std::string(name)));  // owns its bytes
}
```
