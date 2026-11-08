---
name: brave/ios-wire-complete-observer-bridge-chains
description:
  'A C++ observer exposed to Objective-C or Swift is wired end to end: C++
  observer, bridge object, and Objective-C/Swift delegate.'
rubric: correctness
severity: should_fix
---

**Rule.** When a change adds or extends a C++ observer that reports to
Objective-C or Swift, check every link: the bridge is added as an observer of
the C++ service (and removed), each new observer method forwards to the bridge's
Objective-C delegate or protocol method, and the Swift or Objective-C side
implements that method and is set as the delegate.

**Why.** A missing link compiles and fails silently: the C++ event fires and
nothing reaches the UI.

## Examples

Bad:

```objc
// The bridge forwards the new event...
void BookmarkModelListener::BookmarkNodeMoved(...) {
  [observer_ bookmarkNodeMoved:...];
}
// ...but the protocol has no such method, and nothing implements it.
```

Good:

```objc
@protocol BookmarkModelObserver
- (void)bookmarkNodeMoved:(IOSBookmarkNode*)node;
@end

void BookmarkModelListener::BookmarkNodeMoved(...) {
  [observer_ bookmarkNodeMoved:...];
}
// Swift: the observer conforms and is registered with the listener.
```
