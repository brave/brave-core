---
name: brave/top-level-mojom-enums-for-ios
description:
  'Mojom enums of targets that include iOS are declared at the top level of the
  .mojom file, never nested inside a struct.'
rubric: correctness
severity: blocking
---

**Rule.** Declare every mojom enum at the top level of its `.mojom` file. NEVER
nest an enum inside a mojom struct in a target that builds for iOS.

**Why.** The Objective-C++ bindings generator produces invalid code for nested
enums, so the iOS build fails (in `*.mojom.objc.mm`).

## Examples

Bad:

```mojom
struct ModelConfig {
  enum Category {
    kChat = 0,
    kCompletion = 1,
  };
  Category category;
};
```

Good:

```mojom
enum ModelCategory {
  kChat = 0,
  kCompletion = 1,
};

struct ModelConfig {
  ModelCategory category;
};
```
