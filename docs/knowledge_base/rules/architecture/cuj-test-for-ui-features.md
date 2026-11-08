---
name: brave/cuj-test-for-ui-features
description:
  'A change adding or modifying user-visible native (Views) UI includes at least
  one InteractiveBrowserTest Critical User Journey test; never ask for one for
  WebUI, trivial UI or non-UI code.'
rubric: testing
severity: should_fix
---

**Rule.** When a change adds or modifies user-visible native (Views) UI, such as
a new view, button or dialog, it should come with at least one Critical User
Journey test: an `InteractiveBrowserTest` that drives the UI as a user would and
checks the visible result. NEVER ask for a CUJ test for:

- code with no user-visible interaction (tab helpers, services, background
  logic), which unit or browser tests cover;
- WebUI pages such as `brave://settings` or `chrome://history`;
- trivial UI with no meaningful logic, such as a toggle wired to a pref;
- changes following an existing untested pattern in the surrounding code.

**Why.** A CUJ test checks the feature end to end the way users meet it, and
catches the regressions in wiring and visibility that unit tests miss.

## Examples

Bad:

```cpp
// browser/ui/views/foo/foo_bubble_view.cc adds a bubble with a toggle button,
// and the change has no test that opens the bubble and presses the button.
```

Good:

```cpp
IN_PROC_BROWSER_TEST_F(MyFeatureInteractiveTest, UserCanToggleFeature) {
  RunTestSequence(
      PressButton(kMyButton),
      WaitForShow(kResultView),
      CheckViewProperty(kResultView, &views::Label::GetText, u"Enabled"));
}
```
