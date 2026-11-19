---
name: brave/no-sync-os-crypt-in-new-code
description:
  'New code must use the async OSCrypt interface, not the deprecated synchronous
  OSCrypt one.'
rubric: design
severity: blocking
---

**Rule.** New code MUST NOT call the synchronous `OSCrypt::` functions (such as
`OSCrypt::EncryptString()`) or include `components/os_crypt/sync/`. Obtain an
encryptor from the async interface (`os_crypt_async::OSCryptAsync`) instead.

**Why.** The synchronous interface is deprecated (see
`components/os_crypt/sync/README.md`).

## Examples

Bad:

```cpp
OSCrypt::EncryptString(plaintext, &ciphertext);
```

Good:

```cpp
os_crypt_async_->GetInstance(
    base::BindOnce(&MyClass::OnOSCryptReady, weak_factory_.GetWeakPtr()));
```
