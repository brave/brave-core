---
name: brave/prefer-native-side-validation
description:
  Validation logic is not reimplemented in Android Java code when unified
  validation exists, or belongs, on the core/native side.
rubric: design
severity: should_fix
---

**Rule.** Before validation logic (of addresses, URLs, user input and the like)
is written in Android Java code, check whether unified validation already exists
on the core/native side. Call it (through the existing Mojo interface or JNI
bridge) instead of reimplementing it in Java, and prefer adding missing
validation to the core side over adding it to Java.

**Why.** Validation implemented separately per platform drifts: Android, iOS and
desktop end up accepting different inputs. One core-side implementation keeps
them consistent.

## Examples

Bad:

```java
private static boolean isSolanaAddress(String address) {
    return address.matches("[1-9A-HJ-NP-Za-km-z]{32,44}");
}
```

Good:

```java
mBraveWalletService.isBase58EncodedSolanaPubkey(
        address, isValid -> onAddressValidated(isValid));
```
