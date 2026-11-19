---
name: brave/objc-compare-strings-with-is-equal-to-string
description:
  'NSString values are compared with isEqualToString:, never with == or !=.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER compare `NSString` values with `==` or `!=`, against a literal
(`@"expected"`), a constant (`kFooKey`) or another string. Use
`[string isEqualToString:other]`.

**Why.** `==` compares pointer identity, not contents: two equal strings at
different addresses compare unequal, and the result can change with compiler or
runtime string interning.

## Examples

Bad:

```objc
if (string == @"expected") { ... }
```

Good:

```objc
if ([string isEqualToString:@"expected"]) { ... }
```
