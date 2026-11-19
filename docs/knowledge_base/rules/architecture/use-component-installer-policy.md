---
name: brave/use-component-installer-policy
description:
  'A component does not get a component manager of its own; it is registered
  through a component installer policy.'
rubric: design
severity: should_fix
---

**Rule.** Do not add a new manager class to install, register or track a single
component. Implement a `component_updater::ComponentInstallerPolicy` for it and
register that with the component updater.

**Why.** The component installer policy already covers installing, verifying and
updating a component; a manager per component duplicates that machinery.

## Examples

Bad:

```cpp
class FooComponentManager {
 public:
  void RegisterFooComponent();
  void OnFooComponentReady(const base::FilePath& path);
};
```

Good:

```cpp
class FooComponentInstallerPolicy
    : public component_updater::ComponentInstallerPolicy {
  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      base::Value::Dict manifest) override;
};
```
