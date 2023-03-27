# Changelog

Changes to the FVM's shared encoding utilities.

## 0.2.3 [2022-12-12]

Publicly use `serde` to expose it when developing actors.
Add new `IpldBlock` type that supports both `DAG_CBOR` and `IPLD_RAW` codecs

## 0.2.2 [2022-06-13]

Change the hash length assert into an actual check, just in case.

## 0.2.1 [2022-05-19]

Update `serde_ipld_cbor` to 0.2.2.

## 0.2.0 [2022-04-29]

Update `serde_ipld_cbor` to 0.2.0, switching to cbor4ii.

The only breaking change is that `from_reader` now requires `io::BufRead`, not just `io::Read`.
