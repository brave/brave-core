---
name: brave/test-only-proguard-rules-in-apk-for-test-flags
description:
  ProGuard keep rules needed only by tests go in
  android/java/apk_for_test.flags; android/java/proguard.flags holds only rules
  for reflection at runtime.
rubric: perf
severity: blocking
---

**Rule.** A ProGuard keep rule MUST go in the file matching its purpose:
`android/java/proguard.flags` only for classes and methods accessed via
reflection at runtime, and `android/java/apk_for_test.flags` for rules needed
only during testing (such as keeping names that `BytecodeTest` checks or fields
tests inspect). NEVER add a test-only keep rule to `proguard.flags`.

**Why.** Every rule in `proguard.flags` applies to the production APK: a
test-only rule there keeps code from being shrunk or obfuscated in the shipped
browser for no runtime benefit, increasing its size.

## Examples

Bad:

```
# android/java/proguard.flags
# Keep the field names that BytecodeTest checks.
-keep class org.chromium.brave_wallet.mojom.TxData { *; }
```

Good:

```
# android/java/apk_for_test.flags
# Do not obfuscate the above classes as we check on fields in tests
-keep class org.chromium.brave_wallet.mojom.TxData { *; }
```
