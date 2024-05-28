//! # Regtest constants
//!
//! `regtest` is a `zcashd`-specific environment used for local testing. They mostly reuse
//! the testnet constants.
//! These constants are defined in [the `zcashd` codebase].
//!
//! [the `zcashd` codebase]: <https://github.com/zcash/zcash/blob/128d863fb8be39ee294fda397c1ce3ba3b889cb2/src/chainparams.cpp#L482-L496>

/// The regtest cointype reuses the testnet cointype
pub const COIN_TYPE: u32 = 1;

/// The HRP for a Bech32-encoded regtest Sapling [`ExtendedSpendingKey`].
///
/// It is defined in [the `zcashd` codebase].
///
/// [`ExtendedSpendingKey`]: https://docs.rs/sapling-crypto/latest/sapling_crypto/zip32/struct.ExtendedSpendingKey.html
/// [the `zcashd` codebase]: <https://github.com/zcash/zcash/blob/128d863fb8be39ee294fda397c1ce3ba3b889cb2/src/chainparams.cpp#L496>
pub const HRP_SAPLING_EXTENDED_SPENDING_KEY: &str = "secret-extended-key-regtest";

/// The HRP for a Bech32-encoded regtest Sapling [`ExtendedFullViewingKey`].
///
/// It is defined in [the `zcashd` codebase].
///
/// [`ExtendedFullViewingKey`]: https://docs.rs/sapling-crypto/latest/sapling_crypto/zip32/struct.ExtendedFullViewingKey.html
/// [the `zcashd` codebase]: <https://github.com/zcash/zcash/blob/128d863fb8be39ee294fda397c1ce3ba3b889cb2/src/chainparams.cpp#L494>
pub const HRP_SAPLING_EXTENDED_FULL_VIEWING_KEY: &str = "zxviewregtestsapling";

/// The HRP for a Bech32-encoded regtest Sapling [`PaymentAddress`].
///
/// It is defined in [the `zcashd` codebase].
///
/// [`PaymentAddress`]: https://docs.rs/sapling-crypto/latest/sapling_crypto/struct.PaymentAddress.html
/// [the `zcashd` codebase]: <https://github.com/zcash/zcash/blob/128d863fb8be39ee294fda397c1ce3ba3b889cb2/src/chainparams.cpp#L493>
pub const HRP_SAPLING_PAYMENT_ADDRESS: &str = "zregtestsapling";

/// The prefix for a Base58Check-encoded regtest Sprout address.
///
/// Defined in the [Zcash Protocol Specification section 5.6.3][sproutpaymentaddrencoding].
/// Same as the testnet prefix.
///
/// [sproutpaymentaddrencoding]: https://zips.z.cash/protocol/protocol.pdf#sproutpaymentaddrencoding
pub const B58_SPROUT_ADDRESS_PREFIX: [u8; 2] = [0x16, 0xb6];

/// The prefix for a Base58Check-encoded regtest transparent [`PublicKeyHash`].
/// Same as the testnet prefix.
///
/// [`PublicKeyHash`]: https://docs.rs/zcash_primitives/latest/zcash_primitives/legacy/enum.TransparentAddress.html
pub const B58_PUBKEY_ADDRESS_PREFIX: [u8; 2] = [0x1d, 0x25];

/// The prefix for a Base58Check-encoded regtest transparent [`ScriptHash`].
/// Same as the testnet prefix.
///
/// [`ScriptHash`]: https://docs.rs/zcash_primitives/latest/zcash_primitives/legacy/enum.TransparentAddress.html
pub const B58_SCRIPT_ADDRESS_PREFIX: [u8; 2] = [0x1c, 0xba];

/// The HRP for a Bech32m-encoded regtest [ZIP 320] TEX address.
///
/// [ZIP 320]: https://zips.z.cash/zip-0320
pub const HRP_TEX_ADDRESS: &str = "texregtest";
