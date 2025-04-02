# Changelog

Entries are listed in reverse chronological order.

## 0.5.1

* MSRV is now 1.65.0
* Refactor & optimize the NAF (#63)
* Updated `frost-rerandomized` to 0.6.0 (#67)

## 0.5.0

* Add Pallas and Jubjub ciphersuites and FROST support (#33)
* Migrate to `group` 0.13, `jubjub` 0.10, `pasta_curves` 0.5 (#44)

## 0.4.0

* MSRV is now 1.60.0 (note: this was noticed after the crate was released)
* port improvements from Zebra (#40)
* clippy fixes; remove old FROST code (#32)
* Update `criterion` requirement from 0.3 to 0.4 (#29)
* Label Zcash consensus rules in `reddsa` (#27)
* Fix alloc feature (#28)
* fix category (no\_std -> no-std) (#25)

## 0.3.0

* Migrate to `group` 0.12, `jubjub` 0.9, `pasta_curves` 0.4
* Added support for `no-std` builds, via new (default-enabled) `std` and `alloc`
  feature flags. Module `batch` is supported on `alloc` feature only. Module
  `frost` is supported on `std` feature only.

## 0.2.0

* MSRV is now 1.56.0
* Migrate to `pasta_curves` 0.3, `blake2b_simd` 1, removed unneeded `digest` (#10)
* Update the include_str support to fix CI on nightly (#12)

## 0.1.0

Initial release of the `reddsa` crate, extracted from `redjubjub`. Changes
relative to `redjubjub 0.4.0`:

* Generalised the codebase, to enable usage for both RedJubjub and RedPallas.

  * Introduce `SpendAuth: SigType` and `Binding: SigType` traits.
  * The prior `SpendAuth` and `Binding` enums have been renamed to
    `sapling::{SpendAuth, Binding}`.
  * Added `orchard::{SpendAuth, Binding}` enums.

* Migrated to `group 0.11`, `jubjub 0.8`.

* Fixed a bug where small-order verification keys (including the identity) were
  handled inconsistently: the `VerificationKey` parsing logic rejected them, but
  the identity `VerificationKey` could be produced from the zero `SigningKey`.
  The behaviour is now to consistently accept all small-order verification keys,
  matching the RedDSA specification.

  * Downstream users who currently rely on the inconsistent behaviour (for e.g.
    consensus compatibility, either explicitly wanting to reject small-order
    verification keys, or on the belief that this crate implemented the RedDSA
    specification) should continue to use previous versions of this crate, until
    they can either move the checks into their own code, or migrate their
    consensus rules to match the RedDSA specification.

