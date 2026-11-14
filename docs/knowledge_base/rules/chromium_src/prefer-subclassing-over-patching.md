---
name: brave/prefer-subclassing-over-patching
description:
  To change several behaviours of an upstream C++ or Java class, subclass it in
  Brave code and change only which class is used, instead of patching its
  methods.
rubric: design
severity: should_fix
---

**Rule.** When a change patches the bodies of upstream methods to alter a
class's behaviour, ask for a Brave subclass instead (`BraveFoo : Foo` in C++,
`BraveFoo extends Foo` in Java) that overrides those methods, so that the only
upstream change left is the one that makes upstream use the subclass: the class
constructed, or the superclass a Java class extends. That one change is made
without a new `#define` rename ([brave/no-new-macros-editing-upstream-code]).

**Why.** A subclass is easier to understand and to maintain across rebases than
several patches scattered through upstream methods, each of which can conflict
on its own.

## Examples

Bad:

```diff
// patches/chrome-browser-ui-views-foo-foo_view.cc.patch, one hunk per method
 void FooView::OnPaint(gfx::Canvas* canvas) {
+  if (brave::ShouldPaintBraveFoo()) { PaintBraveFoo(canvas); return; }
...
 gfx::Size FooView::CalculatePreferredSize(
     const views::SizeBounds& available_size) const {
+  if (brave::ShouldPaintBraveFoo()) return GetBraveFooSize();
```

Good:

```cpp
// brave/browser/ui/views/foo/brave_foo_view.h
class BraveFooView : public FooView {
 public:
  void OnPaint(gfx::Canvas* canvas) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
};
```
