# derivation-path

A simple struct for dealing with derivation paths as defined by BIP32, BIP44 and BIP49 of the
Bitcoin protocol. This crate provides interfaces for dealing with hardened vs normal child
indexes, as well as display and parsing derivation paths from strings

## Example

```rust
let path = DerivationPath::bip44(0, 1, 0, 1).unwrap();
assert_eq!(&path.to_string(), "m/44'/0'/1'/0/1");
assert_eq!(path.path()[2], ChildIndex::Hardened(1));

let path: DerivationPath = "m/49'/0'/0'/1/0".parse().unwrap();
assert_eq!(path.path()[4], ChildIndex::Normal(0));
assert_eq!(path.path_type(), DerivationPathType::BIP49);
```

License: MIT OR Apache-2.0
