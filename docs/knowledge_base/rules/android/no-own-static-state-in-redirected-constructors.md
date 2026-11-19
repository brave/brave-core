---
name: brave/no-own-static-state-in-redirected-constructors
description:
  The constructor of a type that bytecode rewriting allocates in place of an
  upstream class never reads its own class-initialized static state; that work
  goes in a static block after the fields it uses.
rubric: correctness
severity: blocking
---

**Rule.** When a bytecode adapter redirects an allocation (for example with
`redirectConstructor()`) to a Brave subclass or replacement type, that type's
constructor MUST NOT read its own static fields that class initialization sets
up. Move work that consumes them into a `static {}` block placed after those
fields, and keep the constructor free of such reads. Test the path where a
static field is read first in an isolated class loader or a fresh process when
an earlier test could initialize either class.

**Why.** When reading a `BraveClass` static field is what first triggers
`BraveClass` initialization, Java first initializes its superclass
`UpstreamClass`, whose rewritten static allocation re-enters the `BraveClass`
constructor before `BraveClass`'s static fields are initialized. The constructor
then sees them as null.

## Examples

Bad:

```java
class BraveClass extends UpstreamClass {
    private static final List<Entry> sEntries = List.of(new Entry());

    BraveClass() {
        Registry.register(sEntries);
    }
}
```

Good:

```java
class BraveClass extends UpstreamClass {
    private static final List<Entry> sEntries = List.of(new Entry());

    static {
        Registry.register(sEntries);
    }

    BraveClass() {}
}
```
