---
name: brave/layout-shorthand-attributes
description:
  Android XML sets paddingHorizontal, paddingVertical, marginHorizontal and
  marginVertical instead of equal start/end or top/bottom pairs.
rubric: readability
severity: nit
---

**Rule.** When a view or style sets the same padding or margin on both start and
end, or on both top and bottom, use the shorthand attribute
(`android:paddingHorizontal`, `android:layout_marginHorizontal`,
`android:paddingVertical`, `android:layout_marginVertical`) instead of the two
separate ones.

**Why.** One attribute instead of two makes the XML shorter and says directly
that both sides are the same.

## Examples

Bad:

```xml
<View
    android:paddingStart="16dp"
    android:paddingEnd="16dp" />
```

Good:

```xml
<View
    android:paddingHorizontal="16dp" />
```
