---
name: brave/objc-ns-swift-name-for-swift-apis
description:
  'Objective-C APIs exposed to Swift use NS_SWIFT_NAME to give idiomatic Swift
  names, especially factory methods and C-style functions.'
rubric: readability
severity: nit
---

**Rule.** When the name Swift would import for an Objective-C declaration is not
idiomatic Swift, give it one with `NS_SWIFT_NAME`. Check factory methods
(`+fooWithBar:`) and C-style functions and constants in particular.

**Why.** Brave's Objective-C APIs are mostly called from Swift; without
`NS_SWIFT_NAME`, factory methods and C-style names import as awkward Swift names
that every caller then has to use.

## Examples

Bad:

```objc
+ (instancetype)walletWithKeyring:(Keyring*)keyring;
OBJC_EXPORT NSString* BraveWalletDefaultChainId(void);
```

Good:

```objc
+ (instancetype)walletWithKeyring:(Keyring*)keyring
    NS_SWIFT_NAME(init(keyring:));
OBJC_EXPORT NSString* BraveWalletDefaultChainId(void)
    NS_SWIFT_NAME(BraveWallet.defaultChainId());
```
