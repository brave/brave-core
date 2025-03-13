##  The OPAQUE key exchange protocol ![Build Status](https://github.com/facebook/opaque-ke/workflows/Rust%20CI/badge.svg)

[OPAQUE](https://eprint.iacr.org/2018/163.pdf) is an augmented password-authenticated key exchange protocol. It allows a client to authenticate to a server using a password, without ever having to expose the plaintext password to the server.

This implementation is based on the [Internet Draft for OPAQUE](https://github.com/cfrg/draft-irtf-cfrg-opaque).

Background
----------

Augmented Password Authenticated Key Exchange (aPAKE) protocols are designed to provide password authentication and mutually authenticated key exchange without relying on PKI (except during user/password registration) and without disclosing passwords to servers or other entities other than the client machine.

OPAQUE is a PKI-free aPAKE that is secure against pre-computation attacks and capable of using a secret salt.

Documentation
-------------

The API can be found [here](https://docs.rs/opaque-ke/) along with an example for usage. More examples can be found in the [examples](./examples) directory.

Installation
------------

Add the following line to the dependencies of your `Cargo.toml`:

```
opaque-ke = "3"
```

### Minimum Supported Rust Version

Rust **1.74** or higher.

Audit
-----

This library was audited by NCC Group in June of 2021. The audit was sponsored by WhatsApp for its use in [enabling end-to-end encrypted backups](https://engineering.fb.com/2021/09/10/security/whatsapp-e2ee-backups/).

The audit found issues in release `v0.5.0`, and the fixes were subsequently incorporated into release `v1.2.0`. See the [full audit report here](https://research.nccgroup.com/2021/12/13/public-report-whatsapp-opaque-ke-cryptographic-implementation-review/).

Resources
---------

- [OPAQUE academic publication](https://eprint.iacr.org/2018/163.pdf), including formal definitions and a proof of security
- [draft-irtf-cfrg-opaque-16](https://datatracker.ietf.org/doc/draft-irtf-cfrg-opaque/16/), containing a detailed (byte-level) specification for OPAQUE
- ["Let's talk about PAKE"](https://blog.cryptographyengineering.com/2018/10/19/lets-talk-about-pake/), an introductory blog post written by Matthew Green that covers OPAQUE
- [@serenity-kit/opaque](https://github.com/serenity-kit/opaque), a WebAssembly package for this library
- [opaque-wasm](https://github.com/marucjmar/opaque-wasm), a WebAssembly package for this library. A comparison between `@serenity-kit/opaque` and `opaque-wasm` can be found [here](https://opaque-documentation.netlify.app/docs/faq#how-does-it-compare-to-opaque-wasm)
- [react-native-opaque](https://github.com/serenity-kit/react-native-opaque), a React Native package for this library matching the API of `@serenity-kit/opaque`

Contributors
------------

The authors of this code are Kevin Lewi
([@kevinlewi](https://github.com/kevinlewi)) and François Garillot ([@huitseeker](https://github.com/huitseeker)).
To learn more about contributing to this project, [see this document](./CONTRIBUTING.md).

#### Acknowledgments

Special thanks go to Hugo Krawczyk and Chris Wood for helping to clarify discrepancies and making suggestions for improving
this implementation. Additional credit goes to @daxpedda for adding no_std support, p256 support, and making other general
improvements to the library.

License
-------

This project is dual-licensed under either the [MIT license](./LICENSE-MIT)
or the [Apache License, Version 2.0](./LICENSE-APACHE).
You may select, at your option, one of the above-listed licenses.
