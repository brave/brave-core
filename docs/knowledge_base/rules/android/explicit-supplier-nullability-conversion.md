---
name: brave/explicit-supplier-nullability-conversion
description:
  A Supplier<T> passed where Supplier<@Nullable T> is expected is cast
  explicitly, and the reverse goes through SupplierUtils.asNonNull(), or the
  parameter type changes.
rubric: correctness
severity: should_fix
---

**Rule.** To pass a `Supplier<T>` where a `Supplier<@Nullable T>` is expected,
cast it explicitly: `(Supplier<@Nullable String>) nonNullSupplier`. To pass a
`Supplier<@Nullable T>` where a `Supplier<T>` is expected, wrap it with
`SupplierUtils.asNonNull()`. When the supplied value might actually be null,
change the parameter type to `Supplier<@Nullable T>` and handle the null
instead.

**Why.** Java generics are invariant, so the two supplier types are not
assignment-compatible and NullAway reports the conversion. An explicit cast or
utility makes the conversion visible; an `asNonNull()` over a value that really
can be null only hides it.

## Examples

Bad:

```java
Supplier<String> nonNullSupplier = () -> "hello";
acceptsNullableSupplier(nonNullSupplier);
```

Good:

```java
Supplier<String> nonNullSupplier = () -> "hello";
acceptsNullableSupplier((Supplier<@Nullable String>) nonNullSupplier);

Supplier<@Nullable String> nullableSupplier = () -> maybeNull();
acceptsNonNullSupplier(SupplierUtils.asNonNull(nullableSupplier));
```
