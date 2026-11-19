---
name: brave/constructor-injection-over-setter-callbacks
description:
  "A service's dependencies, callbacks included, are passed through its
  constructor, not set afterwards with a Set*Callback() method."
rubric: design
severity: should_fix
---

**Rule.** Give a service what it depends on, including callbacks, as constructor
parameters. Do not add a `Set*Callback()` method that some other code, such as
an unrelated factory, has to call after construction.

**Why.** A dependency set after construction is invisible in the constructor,
leaves a window in which the service runs without it, and makes the
initialization order matter.

## Examples

Bad:

```cpp
void BraveVpnServiceFactory::BuildServiceInstanceFor(...) {
  auto* manager = BraveVPNConnectionManager::GetInstance();
  manager->SetInstallSystemServiceCallback(base::BindRepeating(...));
}
```

Good:

```cpp
BraveVPNConnectionManager::BraveVPNConnectionManager(
    base::RepeatingCallback<void()> install_callback)
    : install_system_service_callback_(std::move(install_callback)) {}
```
