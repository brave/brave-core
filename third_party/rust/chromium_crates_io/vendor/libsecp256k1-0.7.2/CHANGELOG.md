# Changelog

The format is based on [Keep a Changelog].

[Keep a Changelog]: http://keepachangelog.com/en/1.0.0/

## [0.5.0] - 2021-05-18
- Add standard non-overflowing signature parsing `Signature::parse_standard`. The previous behavior `Signature::parse` is considered non-standard and renamed to `Signature::parse_overflowing`. Unless you have a specific need, you should switch to use the new `Signature::parse_standard` function. (PR #67)

## [0.3.5] - 2020-02-06
- Implement `std::error::Error` and `Display` for `Error`. (PR #29)
- Fix the PartialEq impl of Field. (PR #30)
- Add `LowerHex` implementation for `SecretKey` and `Scalar`. (PR #32)
- Put signing behind feature flag. (PR #33)
