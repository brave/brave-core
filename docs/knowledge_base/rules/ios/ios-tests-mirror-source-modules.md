---
name: brave/ios-tests-mirror-source-modules
description:
  'iOS test files are placed in the test directory that mirrors the module and
  folder of the code they test.'
rubric: design
severity: nit
---

**Rule.** Put a new test file in the test target and folder that mirror the
source it tests: tests of a class in `BraveCore/Wallet/` go in
`BraveCoreTests/Wallet/`, tests of `Sources/BraveWallet/` code go in
`Tests/BraveWalletTests/`.

**Why.** Mirrored paths make a class's tests easy to find, and keep each test
target depending only on the module it tests.

## Examples

Bad:

```
Sources/BraveWallet/Crypto/Stores/SendTokenStore.swift
Tests/ClientTests/SendTokenStoreTests.swift
```

Good:

```
Sources/BraveWallet/Crypto/Stores/SendTokenStore.swift
Tests/BraveWalletTests/SendTokenStoreTests.swift
```
