---
name: brave/objc-headers-assume-nonnull
description:
  'Objective-C headers wrap their declarations in NS_ASSUME_NONNULL_BEGIN/END
  and mark nullable parameters explicitly.'
rubric: correctness
severity: should_fix
---

**Rule.** Wrap the declarations of an Objective-C header between
`NS_ASSUME_NONNULL_BEGIN` and `NS_ASSUME_NONNULL_END`, and mark each parameter,
property and return value that can be `nil` with `nullable`.

**Why.** Swift imports unannotated pointers as implicitly unwrapped optionals,
which crash when `nil`; with the annotations, Swift sees non-optional and
optional types and the compiler checks how they are used.

## Examples

Bad:

```objc
@interface BraveManager : NSObject
- (void)loadURL:(NSURL*)url completion:(CompletionBlock)completion;
@end
```

Good:

```objc
NS_ASSUME_NONNULL_BEGIN

@interface BraveManager : NSObject
- (void)loadURL:(NSURL*)url completion:(nullable CompletionBlock)completion;
@end

NS_ASSUME_NONNULL_END
```
