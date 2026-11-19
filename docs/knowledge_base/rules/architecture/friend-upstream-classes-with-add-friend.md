---
name: brave/friend-upstream-classes-with-add-friend
description:
  'A friend declaration is added to an upstream class with the add_friend
  plaster rewriter, not a #define in a chromium_src header override.'
rubric: design
severity: should_fix
---

**Rule.** To make a Brave class a friend of an upstream class, add an
`add_friend` substitution to the upstream header's plaster in `rewrite/`. Do not
inject the `friend` declaration with a `#define` in a `chromium_src/` header
override (for example one piggybacking on an existing method name), nor with a
`regex` substitution.

**Why.** A `#define` rewrites every occurrence of its identifier in the
translation unit, invisibly; `add_friend` edits the one class, is visible in the
generated patch, and fails loudly when the class changes.

## Examples

Bad:

```cpp
// chromium_src/chrome/browser/extensions/component_loader.h
#define AddNetworkSpeechSynthesisExtension    \
  AddNetworkSpeechSynthesisExtensionUnused(); \
  friend class BraveComponentLoader;          \
  void AddNetworkSpeechSynthesisExtension
```

Good:

```yaml
# rewrite/chrome/browser/extensions/component_loader.h.yaml
substitutions:
  - description: 'Friend BraveComponentLoader'
    add_friend:
      class_name: ComponentLoader
      friend_type: class BraveComponentLoader
```
