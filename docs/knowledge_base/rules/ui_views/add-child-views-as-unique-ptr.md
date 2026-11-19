---
name: brave/add-child-views-as-unique-ptr
description:
  Child views are added with the std::unique_ptr overload of
  View::AddChildView(), never by passing a raw new or released pointer.
rubric: safety
severity: should_fix
---

**Rule.** Add a child view by passing ownership as a `std::unique_ptr`:
`AddChildView(std::make_unique<views::Label>(...))` (or `AddChildViewAt()`),
keeping the returned raw pointer if the parent needs to refer to the child. Do
not pass a raw pointer to the `View*` overload, whether from `new` or from
`std::unique_ptr::release()`.

**Why.** The `unique_ptr` overload makes the transfer of ownership to the parent
explicit and checked by the type system; with a raw pointer nothing shows who
owns the view, which invites leaks and double frees.

## Examples

Bad:

```cpp
AddChildView(new views::Label(u"Hello"));
```

Good:

```cpp
auto* label = AddChildView(std::make_unique<views::Label>(u"Hello"));
```
