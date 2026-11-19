---
name: brave/ios-impl-suffix-for-concrete-implementations
description:
  'Concrete implementations are named with the Impl suffix, not with a platform
  suffix such as IOS or _ios.'
rubric: readability
severity: nit
---

**Rule.** Name the concrete implementation of an interface `<Interface>Impl`
(`BraveWalletServiceImpl`). Do not name it after the platform
(`BraveWalletServiceIOS`, `brave_wallet_service_ios`).

**Why.** The name should say that the class is the implementation of the
interface; the platform is already given by where the file lives.

## Examples

Bad:

```cpp
class BraveWalletServiceIOS : public mojom::BraveWalletService { ... };
```

Good:

```cpp
class BraveWalletServiceImpl : public mojom::BraveWalletService { ... };
```
