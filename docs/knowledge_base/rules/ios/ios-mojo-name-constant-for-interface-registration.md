---
name: brave/ios-mojo-name-constant-for-interface-registration
description:
  'Mojo interfaces are registered under the generated ::Name_ constant, never
  under a hard-coded string.'
rubric: correctness
severity: should_fix
---

**Rule.** When registering a Mojo interface or binder by name, pass the
generated `Interface::Name_` constant (`ai_chat::mojom::Service::Name_`). Do not
spell the interface name as a string literal (`"ai_chat.mojom.Service"`).

**Why.** `Name_` is generated from the `.mojom` file, so it follows renames and
moves of the interface; a literal compiles after such a change and silently
stops matching.

## Examples

Bad:

```cpp
registry->AddInterface("ai_chat.mojom.Service", std::move(binder));
```

Good:

```cpp
registry->AddInterface(ai_chat::mojom::Service::Name_, std::move(binder));
```
