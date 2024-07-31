# ed25519-dalek-bip32

A simple BIP32 implementation for ed25519 public keys. Although there exists [another very good
library that does this](https://docs.rs/ed25519-bip32), this library preserves 32 byte secret
keys and doesn't allow for extended public keys or "normal" child indexes, so that it can be as
close to the BIP32 specifications as possible, allowing for compatibility with libraries like
`trezor-crypto`

License: MIT OR Apache-2.0
