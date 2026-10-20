# BraveTransferCore

The Swift implementation of the Brave Transfer protocol
([`../core/PROTOCOL.md`](../core/PROTOCOL.md)) for iOS. It is a standalone
Swift package with no BraveCore dependency, so it builds and tests in
seconds without a Chromium checkout.

- Apple platforms use CryptoKit and the package has no dependencies.
- On Linux it uses [swift-crypto](https://github.com/apple/swift-crypto)
  (same API), declared only when the manifest is evaluated on Linux.
- protobuf messages use a hand-written codec, since brave-ios does not
  bundle SwiftProtobuf.

## What is here

| File | Contents |
|---|---|
| `ChainKeys.swift` | seed → root, PSK and discovery keys; tags and TXT records; static keys and fingerprints |
| `Noise.swift` | `Noise_XXpsk3_25519_ChaChaPoly_SHA256` |
| `ProtoWire.swift`, `Messages.swift` | wire codec and the messages in `brave_transfer.proto` |
| `SecureChannel.swift` | length-prefix framing, `InitiatorHandshake`, `ResponderHandshake`, `SecureChannel` |
| `TransferSession.swift` | sans-IO transfer state machine and offer validation |
| `FilenameSanitizer.swift` | receiver file name sanitizer |
| `Sources/BraveTransferPeer` | a stdio CLI peer for interop tests; not for the app |

Everything is I/O-free. The iOS engine (`ios/brave-ios/Sources/BraveTransfer`,
to be written) owns the `NWListener`, Bonjour advertising and file access,
and drives these types from one queue.

## Tests

```sh
swift test
```

The tests read the shared vectors from `../core/test/data`. Interop with the
Python reference peer runs from `tools/brave_transfer`:

```sh
cd ../../../tools/brave_transfer && python3 -m pytest tests/test_swift_interop.py
```

## Using it from brave-ios

Add the package to `ios/brave-ios/Package.swift`:

```swift
.package(name: "BraveTransferCore", path: "../../components/brave_transfer/swift"),
```

and depend on `.product(name: "BraveTransferCore", package: "BraveTransferCore")`.
