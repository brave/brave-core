---
name: brave/objc-export-on-headers-only
description:
  'OBJC_EXPORT appears only on the header declaration, never in the
  implementation file.'
rubric: correctness
severity: should_fix
---

**Rule.** Put `OBJC_EXPORT` on the declaration in the header (`@interface`,
exported functions and constants). Do not repeat it on the `@implementation` or
on definitions in the `.m`/`.mm` file.

**Why.** The export attribute belongs to the declaration that other modules see;
repeating it in the implementation is redundant and can disagree with the
header.

## Examples

Bad:

```objc
// brave_utils.mm
OBJC_EXPORT
@implementation BraveUtils
@end
```

Good:

```objc
// brave_utils.h
OBJC_EXPORT
@interface BraveUtils : NSObject
@end

// brave_utils.mm
@implementation BraveUtils
@end
```
