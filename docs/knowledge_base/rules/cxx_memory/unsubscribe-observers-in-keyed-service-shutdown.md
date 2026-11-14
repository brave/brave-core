---
name: brave/unsubscribe-observers-in-keyed-service-shutdown
description:
  'KeyedService::Shutdown() must explicitly unsubscribe observers and pref
  registrars, even ones held in a ScopedObservation.'
rubric: safety
severity: blocking
---

**Rule.** In a `KeyedService`, `Shutdown()` MUST explicitly remove every
observer and pref registration the service holds (`observation_.Reset()`,
`pref_change_registrar_.RemoveAll()`, `RemoveObserver(this)`). Do not rely on a
`base::ScopedObservation` or `PrefChangeRegistrar` member to unsubscribe in its
destructor.

**Why.** The destructor runs only after every service has shut down. In between,
another service can trigger the event during its own shutdown, and a pref
observer or other event-triggered callback then fires on a service that has
already shut down, which leads to use-after-free.

## Examples

Bad:

```cpp
class MyService : public KeyedService {
  // Relies solely on the ScopedObservation destructor.
  base::ScopedObservation<PrefService, PrefObserver> observation_{this};
};
```

Good:

```cpp
void MyService::Shutdown() {
  pref_change_registrar_.RemoveAll();
  observation_.Reset();
}
```
