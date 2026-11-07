---
name: overlap/hook-point-moved
rubric: overlap
severity: regression
---

**Rule.** The call, or the logic, that a Brave `#define` or plaster anchors to
is still where Brave hooks it.

**Why.** When upstream moves it into another function or file, Brave's hook no
longer runs on the path that matters, and nothing fails to build.

**How to check.** For each candidate touching an overridden file, check whether
the hooked call or logic moved out of the function Brave anchors to, and follow
it to its new home. A new enabled-by-default flag can do the same inside one
class: read new features against the virtuals Brave overrides in the files they
gate.

## Examples

- `ManagedUIHandler::Initialize()` moved to another file, and chrome://history
  crashed.
- Clipboard sanitising moved into `FrameClipboardContext`.
- M159, 8133.1: `kOmniboxSkipRedundantLocationBarUpdates` made
  `LocationBarView::Update()` call a non-virtual `OnChangedImpl()`, past
  `BraveLocationBarView::OnChanged()`.
