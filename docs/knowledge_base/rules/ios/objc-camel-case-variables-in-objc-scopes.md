---
name: brave/objc-camel-case-variables-in-objc-scopes
description:
  'Variables in Objective-C classes and types are camelCase; Objective-C values
  held in C++ classes follow C++ snake_case naming.'
rubric: readability
severity: should_fix
---

**Rule.** Name variables by the construct they are in, not by their type:

- inside an Objective-C `@interface`, `@implementation`, category or method, use
  camelCase for locals, parameters, properties and ivars (`_camelCase`), even
  when they hold C++ values (`std::string requestId`);
- inside a C++ class, use C++ snake*case (`snake_case` locals and parameters,
  `snake_case*` members), even when they hold Objective-C objects (`NSString\*
  display*name*`).

**Why.** Matching the enclosing language keeps each scope consistent with the
code around it and with the style guide of that language.

## Examples

Bad:

```objc
@implementation BraveBookmarksAPI
- (void)addBookmark:(NSString*)bookmark_title {
  std::u16string title_utf16 = base::SysNSStringToUTF16(bookmark_title);
}
@end

class BookmarkModelListener {
  NSString* displayName;
};
```

Good:

```objc
@implementation BraveBookmarksAPI
- (void)addBookmark:(NSString*)bookmarkTitle {
  std::u16string titleUTF16 = base::SysNSStringToUTF16(bookmarkTitle);
}
@end

class BookmarkModelListener {
  NSString* display_name_;
};
```
