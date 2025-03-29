# Changelog

## 0.5.0 (March 6, 2024)
* Just a version bump from v0.5.0-pre.7

## 0.5.0-pre.7 (January 11, 2024)
* Updated to be in sync with RFC 9497

## 0.5.0-pre.6 (July 24, 2023)
* Updated curve25519-dalek dependency to 4

## 0.5.0-pre.5 (June 27, 2023)
* Updated curve25519-dalek dependency to 4.0.0-rc.3

## 0.5.0-pre.4 (May 20, 2023)
* Updated curve25519-dalek dependency to 4.0.0-rc.2

## 0.5.0-pre.3 (March 4, 2023)
* Updated to be in sync with draft-irtf-cfrg-voprf-19
* Increased MSRV to 1.65
* Updated p256 dependency to v0.13
* Added p384 tests

## 0.5.0-pre.2 (February 3, 2023)
* Increased MSRV to 1.60
* Updated p256 dependency to v0.12
* Updated curve25519-dalek dependency to 4.0.0-rc.1

## 0.5.0-pre.1 (December 19, 2022)
* Updated curve25519-dalek dependency to 4.0.0-pre.5

## 0.4.0 (September 15, 2022)
* Updated to be in sync with draft-irtf-cfrg-voprf-11, with
  the addition of the POPRF mode
* Adds the evaluate() function to the servers to calculate the output of the OPRF
  directly
* Renames the former evaluate() function to blind_evaluate to match the spec
* Fixes the order of parameters for PoprfClient::blind to align it with the
  other clients
* Exposes the derive_key function under the "danger" feature
* Added support for running the API without performing allocations
* Revamped the way the Group trait was used, so as to be more easily
  extendable to other groups
* Added common traits for each public-facing struct, including serde
  support

## 0.3.0 (October 25, 2021)

* Updated to be in sync with draft-irtf-cfrg-voprf-08

## 0.2.0 (October 18, 2021)

* Removed the CipherSuite interface
* Added the "danger" feature for exposing internal functions
* General improvements to the group interface

## 0.1.0 (September 29, 2021)

* Initial release
