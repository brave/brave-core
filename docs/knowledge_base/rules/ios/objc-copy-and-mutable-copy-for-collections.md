---
name: brave/objc-copy-and-mutable-copy-for-collections
description:
  'Objective-C code takes immutable collections with copy and mutable ones with
  mutableCopy, and never passes one mutability where the other is expected.'
rubric: correctness
severity: should_fix
---

**Rule.** Get an immutable collection with `copy` and a mutable one with
`mutableCopy`; never assign the result of `copy` to an `NSMutable*` variable. Do
not store or hand on a mutable collection that its owner keeps mutating where an
immutable one is expected: `copy` it. A mutable collection built locally and not
mutated afterwards needs no `copy` to be returned or assigned as its immutable
type; NEVER ask for one there.

**Why.** `copy` of a mutable collection is immutable, so mutating it throws at
run time; and an `NSArray*` that is really an `NSMutableArray` still mutated
elsewhere changes under the code that holds it. A local collection that nobody
mutates any more has neither problem, and copying it only allocates.

## Examples

Bad:

```objc
NSMutableArray* items = [self.items copy];
[items addObject:item];  // Throws: copy is immutable.

- (void)setTabs:(NSMutableArray*)tabs {
  _tabs = tabs;  // Caller keeps mutating tabs.
}
```

Good:

```objc
NSMutableArray* items = [self.items mutableCopy];
[items addObject:item];

- (void)setTabs:(NSArray*)tabs {
  _tabs = [tabs copy];
}

- (NSArray*)titles {
  NSMutableArray* titles = [NSMutableArray array];
  // ... fill titles.
  return titles;  // Built locally, not mutated afterwards.
}
```
