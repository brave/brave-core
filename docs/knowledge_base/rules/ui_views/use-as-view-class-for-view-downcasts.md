---
name: brave/use-as-view-class-for-view-downcasts
description:
  A views::View pointer is downcast with views::AsViewClass<T>() or tested with
  views::IsViewClass<T>(), never with static_cast<T*>().
rubric: safety
severity: should_fix
---

**Rule.** To downcast a `views::View*` to a subclass, use
`views::AsViewClass<T>(view)` and handle the `nullptr` it returns on a type
mismatch, or test the type with `views::IsViewClass<T>(view)`. Do NOT
`static_cast<T*>()` a view pointer. This holds in particular for a Brave
subclass overriding a Chromium view.

**Why.** Chromium builds without RTTI, so `dynamic_cast` is unavailable;
`AsViewClass` and `IsViewClass` walk the metadata chain `METADATA_HEADER`
registers. They fail to compile for a target class missing `METADATA_HEADER`,
and at runtime return `nullptr` on a mismatch, including a partially constructed
view whose own metadata is not installed yet. `static_cast` checks nothing: on a
mismatch dereferencing its result is undefined behaviour, which UBSan does not
catch for a partially constructed object.

## Examples

Bad:

```cpp
auto* my_view = static_cast<MyCustomView*>(some_view);
my_view->DoSomething();
```

Good:

```cpp
auto* my_view = views::AsViewClass<MyCustomView>(some_view);
if (!my_view) {
  return;
}
my_view->DoSomething();
```
