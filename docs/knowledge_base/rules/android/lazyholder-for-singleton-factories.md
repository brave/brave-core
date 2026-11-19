---
name: brave/lazyholder-for-singleton-factories
description:
  Java singleton service factories hold their instance in a nested holder
  class's static final field, not behind a synchronized block on a lock Object.
rubric: design
severity: should_fix
---

**Rule.** A singleton service factory MUST create its instance in a
`static final` field of a private nested holder class (`LazyHolder.INSTANCE`)
and return that field from `getInstance()`. Do not write lazy initialization
with a lock `Object`, a `synchronized` block and a null check.

**Why.** Java initializes the nested class, and its field, once and
thread-safely the first time `getInstance()` reads it, so the holder gives the
same lazy, thread-safe singleton as the lock with less code and no lock on every
call.

## Examples

Bad:

```java
public class BraveAccountServiceFactory {
    private static final Object sLock = new Object();
    private static BraveAccountServiceFactory sInstance;

    public static BraveAccountServiceFactory getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new BraveAccountServiceFactory();
            }
            return sInstance;
        }
    }
}
```

Good:

```java
public class BraveAccountServiceFactory {
    private static class LazyHolder {
        static final BraveAccountServiceFactory INSTANCE = new BraveAccountServiceFactory();
    }

    public static BraveAccountServiceFactory getInstance() {
        return LazyHolder.INSTANCE;
    }
}
```
