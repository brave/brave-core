---
name: brave/objc-static-only-classes-unavailable-init
description:
  'An Objective-C class with only class methods marks init as NS_UNAVAILABLE;
  marking new is not needed.'
rubric: design
severity: should_fix
---

**Rule.** When an Objective-C class has only class methods and must not be
instantiated, declare `- (instancetype)init NS_UNAVAILABLE;` in its
`@interface`. Do not ask for `+ (instancetype)new NS_UNAVAILABLE;` as well.

**Why.** Marking `init` unavailable makes an accidental instantiation a compile
error, in Objective-C and in Swift. `new` needs no annotation because the class
is used mostly from Swift, which never calls `new`.

## Examples

Bad:

```objc
@interface BraveUtils : NSObject
+ (NSString*)userAgent;
@end
```

Good:

```objc
@interface BraveUtils : NSObject
+ (NSString*)userAgent;
- (instancetype)init NS_UNAVAILABLE;
@end
```
