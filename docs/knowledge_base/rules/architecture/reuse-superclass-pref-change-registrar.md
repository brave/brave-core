---
name: brave/reuse-superclass-pref-change-registrar
description:
  'Before adding a PrefChangeRegistrar member, check whether a superclass
  already has a pref_change_registrar_ to reuse.'
rubric: design
severity: nit
---

**Rule.** When a subclass adds a `PrefChangeRegistrar` member, check whether its
superclass (often the upstream class it extends) already has a
`pref_change_registrar_`; if so, add the subclass's prefs to that one instead of
declaring a second registrar.

**Why.** A second registrar for the same prefs service is redundant state, and
two members of the same name shadow each other confusingly.

## Examples

Bad:

```cpp
class BraveFooHandler : public FooHandler {
 private:
  PrefChangeRegistrar pref_change_registrar_;  // FooHandler already has one
};
```

Good:

```cpp
void BraveFooHandler::Init() {
  FooHandler::Init();
  pref_change_registrar_.Add(kBravePref, base::BindRepeating(...));
}
```
