---
name: brave/constructor-for-struct-like-class-fields
description:
  A struct-like Java class sets its non-primitive, non-@Nullable public fields
  in a constructor, with /* paramName= */ comments at call sites where
  readability suffers.
rubric: correctness
severity: should_fix
---

**Rule.** A class with public fields MUST have a constructor that sets every
non-primitive, non-`@Nullable` public field. Do not leave such fields to be
assigned after construction. When a call to that constructor becomes hard to
read, label its arguments with `/* paramName= */` comments.

**Why.** NullAway has no special handling for struct-like classes: it warns on
each non-primitive, non-`@Nullable` public field that no constructor sets,
because such a field is null until someone assigns it.

## Examples

Bad:

```java
public class TabInfo {
    public String title;
    public String url;
}
```

Good:

```java
public class TabInfo {
    public final String title;
    public final String url;

    public TabInfo(String title, String url) {
        this.title = title;
        this.url = url;
    }
}

new TabInfo(/* title= */ "Home", /* url= */ "https://brave.com");
```
