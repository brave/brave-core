---
name: brave/assert-bound-instead-of-initializer-on-bind
description:
  onBindViewHolder() is never marked @Initializer; methods needing the fields it
  sets call an assertBound() helper annotated @EnsuresNonNull.
rubric: correctness
severity: should_fix
---

**Rule.** NEVER annotate `onBindViewHolder()` (or another view binder) with
`@Initializer`. Add a private `assertBound()` helper annotated
`@EnsuresNonNull({"mField1", "mField2"})` that asserts each field is non-null,
and call it at the start of the methods that use those fields.

**Why.** `@Initializer` tells NullAway the method runs right after construction,
so it treats the fields as set from then on. A binder runs later, possibly
never, so the annotation hides real null dereferences; `assertBound()` checks
the fields where they are used.

## Examples

Bad:

```java
@Initializer
@Override
public void onBindViewHolder(ViewHolder holder, int position) {
    mField1 = holder.field1;
    mField2 = holder.field2;
}
```

Good:

```java
@EnsuresNonNull({"mField1", "mField2"})
private void assertBound() {
    assert mField1 != null;
    assert mField2 != null;
}

private void updateView() {
    assertBound();
    mField1.setText("hello");
}
```
