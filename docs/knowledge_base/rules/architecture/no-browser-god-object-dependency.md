---
name: brave/no-browser-god-object-dependency
description:
  'Features do not take or keep a Browser* to drill through for what they need;
  they take the narrow objects they use.'
rubric: design
severity: should_fix
---

**Rule.** Do not give a feature a `Browser*` (as a constructor parameter or a
member) so that it can reach whatever it needs through it
(`browser_->GetProfile()->GetPrefs()`). Inject the specific objects it uses,
such as the `tabs::TabInterface`, a `PageActionController` or a `PrefService`.

**Why.** `Browser` is a god object: a feature that depends on it can reach
everything, hides which parts it uses, cannot be tested without a whole browser,
and blocks modularization.

## Examples

Bad:

```cpp
FooFeature(Browser* browser) : browser_(browser) {}
void FooFeature::DoStuff() {
  DoStuffWith(browser_->GetProfile()->GetPrefs());
}
```

Good:

```cpp
FooFeature(tabs::TabInterface& tab,
           page_actions::PageActionController& page_action_controller)
    : tab_(tab), page_action_controller_(page_action_controller) {}
```
