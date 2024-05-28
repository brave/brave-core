//! Constants for the Zcash test network.

/// The testnet coin type for ZEC, as defined by [SLIP 44].
///
/// [SLIP 44]: https://github.com/satoshilabs/slips/blob/master/slip-0044.md
pub const COIN_TYPE: u32 = 1;

/// The HRP for a Bech32-encoded testnet Sapling [`ExtendedSpendingKey`].
///
/// Defined in [ZIP 32].
///
/// [`ExtendedSpendingKey`]: https://docs.rs/sapling-crypto/latest/sapling_crypto/zip32/struct.ExtendedSpendingKey.html
/// [ZIP 32]: https://github.com/zcash/zips/blob/master/zip-0032.rst
pub const HRP_SAPLING_EXTENDED_SPENDING_KEY: &str = "secret-extended-key-test";

/// The HRP for a Bech32-encoded testnet Sapling [`ExtendedFullViewingKey`].
///
/// Defined in [ZIP 32].
///
/// [`ExtendedFullViewingKey`]: https://docs.rs/sapling-crypto/latest/sapling_crypto/zip32/struct.ExtendedFullViewingKey.html
/// [ZIP 32]: https://github.com/zcash/zips/blob/master/zip-0032.rst
pub const HRP_SAPLING_EXTENDED_FULL_VIEWING_KEY: &str = "zxviewtestsapling";

/// The HRP for a Bech32-encoded testnet Sapling [`PaymentAddress`].
///
/// Defined in section 5.6.4 of the [Zcash Protocol Specification].
///
/// [`PaymentAddress`]: https://docs.rs/sapling-crypto/latest/sapling_crypto/struct.PaymentAddress.html
/// [Zcash Protocol Specification]: https://github.com/zcash/zips/blob/master/protocol/protocol.pdf
pub const HRP_SAPLING_PAYMENT_ADDRESS: &str = "ztestsapling";

/// The prefix for a Base58Check-encoded testnet Sprout address.
///
/// Defined in the [Zcash Protocol Specification section 5.6.3][sproutpaymentaddrencoding].
///
/// [sproutpaymentaddrencoding]: https://zips.z.cash/protocol/protocol.pdf#sproutpaymentaddrencoding
pub const B58_SPROUT_ADDRESS_PREFIX: [u8; 2] = [0x16, 0xb6];

/// The prefix for a Base58Check-encoded testnet transparent [`PublicKeyHash`].
///
/// [`PublicKeyHash`]: https://docs.rs/zcash_primitives/latest/zcash_primitives/legacy/enum.TransparentAddress.html
pub const B58_PUBKEY_ADDRESS_PREFIX: [u8; 2] = [0x1d, 0x25];

/// The prefix for a Base58Check-encoded testnet transparent [`ScriptHash`].
///
/// [`ScriptHash`]: https://docs.rs/zcash_primitives/latest/zcash_primitives/legacy/enum.TransparentAddress.html
pub const B58_SCRIPT_ADDRESS_PREFIX: [u8; 2] = [0x1c, 0xba];

/// The HRP for a Bech32m-encoded testnet [ZIP 320] TEX address.
///
/// [ZIP 320]: https://zips.z.cash/zip-0320
pub const HRP_TEX_ADDRESS: &str = "textest";
