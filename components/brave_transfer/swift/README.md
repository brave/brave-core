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

Everything in `BraveTransferCore` is I/O-free.

`Sources/BraveTransferEngine` builds the iPhone side on top of it, still with
Foundation only, so it is tested here on Linux as well:

| File | Contents |
|---|---|
| `TransferService.swift` | the Swift counterpart of the desktop `BraveTransferService`: listening, Sync-chain trust and key pinning, offers, accept/decline, saving, inbox, sending |
| `PeerConnection.swift` | drives a handshake and a `TransferSession` over a `ByteStream` |
| `TrustSource.swift`, `TrustStore.swift` | the Sync snapshot shared with the share extension, and pinned keys |
| `FileSink.swift`, `InboxStore.swift` | received files (temporary file, then a finalizer) and the inbox |
| `OutgoingItems.swift`, `Outbox.swift` | items to send, and the share extension's hand-off to the app |
| `Discovery.swift`, `Scheduler.swift`, `ByteStream.swift` | the protocols the platform implements |

The Apple-only pieces (`NWListener` and Bonjour, keychain, Photos, UI) are in
`ios/brave-ios/Sources/BraveTransfer` and `BraveTransferUI`.

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

and depend on `.product(name: "BraveTransferCore", package: "BraveTransferCore")`
and `.product(name: "BraveTransferEngine", package: "BraveTransferCore")`.
