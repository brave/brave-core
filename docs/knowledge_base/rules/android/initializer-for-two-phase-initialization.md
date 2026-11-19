---
name: brave/initializer-for-two-phase-initialization
description:
  The second-phase method of a class with two-phase initialization is annotated
  @Initializer, and setters always called together become one initialize()
  method.
rubric: correctness
severity: should_fix
---

**Rule.** When a class sets its non-null fields in a second initialization phase
(an `initialize()` method called right after construction), annotate that method
`@Initializer`. Android's `onCreate()` and similar methods are already
implicitly `@Initializer`. When several setters are always called together to
initialize the object, replace them with a single `initialize()` method.

**Why.** `@Initializer` lets NullAway check the non-null fields as if the method
ran right after the constructor. NullAway does not verify that `@Initializer`
methods are actually called, so one `initialize()` keeps that unverified
assumption to a single, obvious call.

## Examples

Bad:

```java
public class MyComponent {
    private @Nullable SomeService mService;

    public void initialize(SomeService service) {
        mService = service;
    }
}
```

Good:

```java
public class MyComponent {
    private SomeService mService;

    @Initializer
    public void initialize(SomeService service) {
        mService = service;
    }
}
```
