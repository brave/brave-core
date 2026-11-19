---
name: brave/nullable-immediately-before-type
description:
  '@Nullable, a TYPE_USE annotation, is written immediately before the type it
  annotates: after the modifiers, before the inner name of a nested type, and at
  the array or type argument that is nullable.'
rubric: correctness
severity: blocking
---

**Rule.** `@Nullable` MUST appear immediately before the type it annotates:
`private @Nullable String mValue;`, never on its own line or before the
modifiers. For a nested type it goes before the inner type name
(`Outer.@Nullable Inner`). For arrays and generics, its position says what is
nullable: `String @Nullable[]` is a nullable array of non-null strings,
`@Nullable String[]` a non-null array of nullable strings, and
`List<@Nullable String>` a non-null list of nullable strings.

**Why.** In JSpecify mode `@Nullable` is a `TYPE_USE` annotation: placed on a
separate line or before modifiers it does not compile, and placed on the wrong
part of an array or generic type it marks the wrong thing nullable.

## Examples

Bad:

```java
@Nullable
private String mValue;
private @Nullable Outer.Inner mNestedType;
```

Good:

```java
private @Nullable String mValue;
private Outer.@Nullable Inner mNestedType;
private String @Nullable[] mNullableArrayOfNonNullString;
private @Nullable Callback<@Nullable String> mNullableCallbackOfNullableString;
```
