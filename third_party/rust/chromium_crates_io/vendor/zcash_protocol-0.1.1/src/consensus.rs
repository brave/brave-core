//! Consensus logic and parameters.

use memuse::DynamicUsage;
use std::cmp::{Ord, Ordering};
use std::convert::TryFrom;
use std::fmt;
use std::ops::{Add, Bound, RangeBounds, Sub};

use crate::constants::{mainnet, regtest, testnet};

/// A wrapper type representing blockchain heights.
///
/// Safe conversion from various integer types, as well as addition and subtraction, are
/// provided.
#[repr(transparent)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct BlockHeight(u32);

memuse::impl_no_dynamic_usage!(BlockHeight);

/// The height of the genesis block on a network.
pub const H0: BlockHeight = BlockHeight(0);

impl BlockHeight {
    pub const fn from_u32(v: u32) -> BlockHeight {
        BlockHeight(v)
    }

    /// Subtracts the provided value from this height, returning `H0` if this would result in
    /// underflow of the wrapped `u32`.
    pub fn saturating_sub(self, v: u32) -> BlockHeight {
        BlockHeight(self.0.saturating_sub(v))
    }
}

impl fmt::Display for BlockHeight {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.0.fmt(formatter)
    }
}

impl Ord for BlockHeight {
    fn cmp(&self, other: &Self) -> Ordering {
        self.0.cmp(&other.0)
    }
}

impl PartialOrd for BlockHeight {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl From<u32> for BlockHeight {
    fn from(value: u32) -> Self {
        BlockHeight(value)
    }
}

impl From<BlockHeight> for u32 {
    fn from(value: BlockHeight) -> u32 {
        value.0
    }
}

impl TryFrom<u64> for BlockHeight {
    type Error = std::num::TryFromIntError;

    fn try_from(value: u64) -> Result<Self, Self::Error> {
        u32::try_from(value).map(BlockHeight)
    }
}

impl From<BlockHeight> for u64 {
    fn from(value: BlockHeight) -> u64 {
        value.0 as u64
    }
}

impl TryFrom<i32> for BlockHeight {
    type Error = std::num::TryFromIntError;

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        u32::try_from(value).map(BlockHeight)
    }
}

impl TryFrom<i64> for BlockHeight {
    type Error = std::num::TryFromIntError;

    fn try_from(value: i64) -> Result<Self, Self::Error> {
        u32::try_from(value).map(BlockHeight)
    }
}

impl From<BlockHeight> for i64 {
    fn from(value: BlockHeight) -> i64 {
        value.0 as i64
    }
}

impl Add<u32> for BlockHeight {
    type Output = Self;

    fn add(self, other: u32) -> Self {
        BlockHeight(self.0 + other)
    }
}

impl Add for BlockHeight {
    type Output = Self;

    fn add(self, other: Self) -> Self {
        self + other.0
    }
}

impl Sub<u32> for BlockHeight {
    type Output = Self;

    fn sub(self, other: u32) -> Self {
        if other > self.0 {
            panic!("Subtraction resulted in negative block height.");
        }

        BlockHeight(self.0 - other)
    }
}

impl Sub for BlockHeight {
    type Output = Self;

    fn sub(self, other: Self) -> Self {
        self - other.0
    }
}

/// Constants associated with a given Zcash network.
pub trait NetworkConstants: Clone {
    /// The coin type for ZEC, as defined by [SLIP 44].
    ///
    /// [SLIP 44]: https://github.com/satoshilabs/slips/blob/master/slip-0044.md
    fn coin_type(&self) -> u32;

    /// Returns the human-readable prefix for Bech32-encoded Sapling extended spending keys
    /// for the network to which this NetworkConstants value applies.
    ///
    /// Defined in [ZIP 32].
    ///
    /// [`ExtendedSpendingKey`]: zcash_primitives::zip32::ExtendedSpendingKey
    /// [ZIP 32]: https://github.com/zcash/zips/blob/master/zip-0032.rst
    fn hrp_sapling_extended_spending_key(&self) -> &'static str;

    /// Returns the human-readable prefix for Bech32-encoded Sapling extended full
    /// viewing keys for the network to which this NetworkConstants value applies.
    ///
    /// Defined in [ZIP 32].
    ///
    /// [`ExtendedFullViewingKey`]: zcash_primitives::zip32::ExtendedFullViewingKey
    /// [ZIP 32]: https://github.com/zcash/zips/blob/master/zip-0032.rst
    fn hrp_sapling_extended_full_viewing_key(&self) -> &'static str;

    /// Returns the Bech32-encoded human-readable prefix for Sapling payment addresses
    /// for the network to which this NetworkConstants value applies.
    ///
    /// Defined in section 5.6.4 of the [Zcash Protocol Specification].
    ///
    /// [`PaymentAddress`]: zcash_primitives::primitives::PaymentAddress
    /// [Zcash Protocol Specification]: https://github.com/zcash/zips/blob/master/protocol/protocol.pdf
    fn hrp_sapling_payment_address(&self) -> &'static str;

    /// Returns the human-readable prefix for Base58Check-encoded Sprout
    /// payment addresses for the network to which this NetworkConstants value
    /// applies.
    ///
    /// Defined in the [Zcash Protocol Specification section 5.6.3][sproutpaymentaddrencoding].
    ///
    /// [sproutpaymentaddrencoding]: https://zips.z.cash/protocol/protocol.pdf#sproutpaymentaddrencoding
    fn b58_sprout_address_prefix(&self) -> [u8; 2];

    /// Returns the human-readable prefix for Base58Check-encoded transparent
    /// pay-to-public-key-hash payment addresses for the network to which this NetworkConstants value
    /// applies.
    ///
    /// [`TransparentAddress::PublicKey`]: zcash_primitives::legacy::TransparentAddress::PublicKey
    fn b58_pubkey_address_prefix(&self) -> [u8; 2];

    /// Returns the human-readable prefix for Base58Check-encoded transparent pay-to-script-hash
    /// payment addresses for the network to which this NetworkConstants value applies.
    ///
    /// [`TransparentAddress::Script`]: zcash_primitives::legacy::TransparentAddress::Script
    fn b58_script_address_prefix(&self) -> [u8; 2];

    /// Returns the Bech32-encoded human-readable prefix for TEX addresses, for the
    /// network to which this `NetworkConstants` value applies.
    ///
    /// Defined in [ZIP 320].
    ///
    /// [ZIP 320]: https://zips.z.cash/zip-0320
    fn hrp_tex_address(&self) -> &'static str;
}

/// The enumeration of known Zcash network types.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum NetworkType {
    /// Zcash Mainnet.
    Main,
    /// Zcash Testnet.
    Test,
    /// Private integration / regression testing, used in `zcashd`.
    ///
    /// For some address types there is no distinction between test and regtest encodings;
    /// those will always be parsed as `Network::Test`.
    Regtest,
}

memuse::impl_no_dynamic_usage!(NetworkType);

impl NetworkConstants for NetworkType {
    fn coin_type(&self) -> u32 {
        match self {
            NetworkType::Main => mainnet::COIN_TYPE,
            NetworkType::Test => testnet::COIN_TYPE,
            NetworkType::Regtest => regtest::COIN_TYPE,
        }
    }

    fn hrp_sapling_extended_spending_key(&self) -> &'static str {
        match self {
            NetworkType::Main => mainnet::HRP_SAPLING_EXTENDED_SPENDING_KEY,
            NetworkType::Test => testnet::HRP_SAPLING_EXTENDED_SPENDING_KEY,
            NetworkType::Regtest => regtest::HRP_SAPLING_EXTENDED_SPENDING_KEY,
        }
    }

    fn hrp_sapling_extended_full_viewing_key(&self) -> &'static str {
        match self {
            NetworkType::Main => mainnet::HRP_SAPLING_EXTENDED_FULL_VIEWING_KEY,
            NetworkType::Test => testnet::HRP_SAPLING_EXTENDED_FULL_VIEWING_KEY,
            NetworkType::Regtest => regtest::HRP_SAPLING_EXTENDED_FULL_VIEWING_KEY,
        }
    }

    fn hrp_sapling_payment_address(&self) -> &'static str {
        match self {
            NetworkType::Main => mainnet::HRP_SAPLING_PAYMENT_ADDRESS,
            NetworkType::Test => testnet::HRP_SAPLING_PAYMENT_ADDRESS,
            NetworkType::Regtest => regtest::HRP_SAPLING_PAYMENT_ADDRESS,
        }
    }

    fn b58_sprout_address_prefix(&self) -> [u8; 2] {
        match self {
            NetworkType::Main => mainnet::B58_SPROUT_ADDRESS_PREFIX,
            NetworkType::Test => testnet::B58_SPROUT_ADDRESS_PREFIX,
            NetworkType::Regtest => regtest::B58_SPROUT_ADDRESS_PREFIX,
        }
    }

    fn b58_pubkey_address_prefix(&self) -> [u8; 2] {
        match self {
            NetworkType::Main => mainnet::B58_PUBKEY_ADDRESS_PREFIX,
            NetworkType::Test => testnet::B58_PUBKEY_ADDRESS_PREFIX,
            NetworkType::Regtest => regtest::B58_PUBKEY_ADDRESS_PREFIX,
        }
    }

    fn b58_script_address_prefix(&self) -> [u8; 2] {
        match self {
            NetworkType::Main => mainnet::B58_SCRIPT_ADDRESS_PREFIX,
            NetworkType::Test => testnet::B58_SCRIPT_ADDRESS_PREFIX,
            NetworkType::Regtest => regtest::B58_SCRIPT_ADDRESS_PREFIX,
        }
    }

    fn hrp_tex_address(&self) -> &'static str {
        match self {
            NetworkType::Main => mainnet::HRP_TEX_ADDRESS,
            NetworkType::Test => testnet::HRP_TEX_ADDRESS,
            NetworkType::Regtest => regtest::HRP_TEX_ADDRESS,
        }
    }
}

/// Zcash consensus parameters.
pub trait Parameters: Clone {
    /// Returns the type of network configured by this set of consensus parameters.
    fn network_type(&self) -> NetworkType;

    /// Returns the activation height for a particular network upgrade,
    /// if an activation height has been set.
    fn activation_height(&self, nu: NetworkUpgrade) -> Option<BlockHeight>;

    /// Determines whether the specified network upgrade is active as of the
    /// provided block height on the network to which this Parameters value applies.
    fn is_nu_active(&self, nu: NetworkUpgrade, height: BlockHeight) -> bool {
        self.activation_height(nu).map_or(false, |h| h <= height)
    }
}

impl<P: Parameters> NetworkConstants for P {
    fn coin_type(&self) -> u32 {
        self.network_type().coin_type()
    }

    fn hrp_sapling_extended_spending_key(&self) -> &'static str {
        self.network_type().hrp_sapling_extended_spending_key()
    }

    fn hrp_sapling_extended_full_viewing_key(&self) -> &'static str {
        self.network_type().hrp_sapling_extended_full_viewing_key()
    }

    fn hrp_sapling_payment_address(&self) -> &'static str {
        self.network_type().hrp_sapling_payment_address()
    }

    fn b58_sprout_address_prefix(&self) -> [u8; 2] {
        self.network_type().b58_sprout_address_prefix()
    }

    fn b58_pubkey_address_prefix(&self) -> [u8; 2] {
        self.network_type().b58_pubkey_address_prefix()
    }

    fn b58_script_address_prefix(&self) -> [u8; 2] {
        self.network_type().b58_script_address_prefix()
    }

    fn hrp_tex_address(&self) -> &'static str {
        self.network_type().hrp_tex_address()
    }
}

/// Marker struct for the production network.
#[derive(PartialEq, Eq, Copy, Clone, Debug)]
pub struct MainNetwork;

memuse::impl_no_dynamic_usage!(MainNetwork);

/// The production network.
pub const MAIN_NETWORK: MainNetwork = MainNetwork;

impl Parameters for MainNetwork {
    fn network_type(&self) -> NetworkType {
        NetworkType::Main
    }

    fn activation_height(&self, nu: NetworkUpgrade) -> Option<BlockHeight> {
        match nu {
            NetworkUpgrade::Overwinter => Some(BlockHeight(347_500)),
            NetworkUpgrade::Sapling => Some(BlockHeight(419_200)),
            NetworkUpgrade::Blossom => Some(BlockHeight(653_600)),
            NetworkUpgrade::Heartwood => Some(BlockHeight(903_000)),
            NetworkUpgrade::Canopy => Some(BlockHeight(1_046_400)),
            NetworkUpgrade::Nu5 => Some(BlockHeight(1_687_104)),
            #[cfg(zcash_unstable = "nu6")]
            NetworkUpgrade::Nu6 => None,
            #[cfg(zcash_unstable = "zfuture")]
            NetworkUpgrade::ZFuture => None,
        }
    }
}

/// Marker struct for the test network.
#[derive(PartialEq, Eq, Copy, Clone, Debug)]
pub struct TestNetwork;

memuse::impl_no_dynamic_usage!(TestNetwork);

/// The test network.
pub const TEST_NETWORK: TestNetwork = TestNetwork;

impl Parameters for TestNetwork {
    fn network_type(&self) -> NetworkType {
        NetworkType::Test
    }

    fn activation_height(&self, nu: NetworkUpgrade) -> Option<BlockHeight> {
        match nu {
            NetworkUpgrade::Overwinter => Some(BlockHeight(207_500)),
            NetworkUpgrade::Sapling => Some(BlockHeight(280_000)),
            NetworkUpgrade::Blossom => Some(BlockHeight(584_000)),
            NetworkUpgrade::Heartwood => Some(BlockHeight(903_800)),
            NetworkUpgrade::Canopy => Some(BlockHeight(1_028_500)),
            NetworkUpgrade::Nu5 => Some(BlockHeight(1_842_420)),
            #[cfg(zcash_unstable = "nu6")]
            NetworkUpgrade::Nu6 => None,
            #[cfg(zcash_unstable = "zfuture")]
            NetworkUpgrade::ZFuture => None,
        }
    }
}

/// The enumeration of known Zcash networks.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Network {
    /// Zcash Mainnet.
    MainNetwork,
    /// Zcash Testnet.
    TestNetwork,
}

memuse::impl_no_dynamic_usage!(Network);

impl Parameters for Network {
    fn network_type(&self) -> NetworkType {
        match self {
            Network::MainNetwork => NetworkType::Main,
            Network::TestNetwork => NetworkType::Test,
        }
    }

    fn activation_height(&self, nu: NetworkUpgrade) -> Option<BlockHeight> {
        match self {
            Network::MainNetwork => MAIN_NETWORK.activation_height(nu),
            Network::TestNetwork => TEST_NETWORK.activation_height(nu),
        }
    }
}

/// An event that occurs at a specified height on the Zcash chain, at which point the
/// consensus rules enforced by the network are altered.
///
/// See [ZIP 200](https://zips.z.cash/zip-0200) for more details.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum NetworkUpgrade {
    /// The [Overwinter] network upgrade.
    ///
    /// [Overwinter]: https://z.cash/upgrade/overwinter/
    Overwinter,
    /// The [Sapling] network upgrade.
    ///
    /// [Sapling]: https://z.cash/upgrade/sapling/
    Sapling,
    /// The [Blossom] network upgrade.
    ///
    /// [Blossom]: https://z.cash/upgrade/blossom/
    Blossom,
    /// The [Heartwood] network upgrade.
    ///
    /// [Heartwood]: https://z.cash/upgrade/heartwood/
    Heartwood,
    /// The [Canopy] network upgrade.
    ///
    /// [Canopy]: https://z.cash/upgrade/canopy/
    Canopy,
    /// The [Nu5] network upgrade.
    ///
    /// [Nu5]: https://z.cash/upgrade/nu5/
    Nu5,
    /// The [Nu6] network upgrade.
    ///
    /// [Nu6]: https://z.cash/upgrade/nu6/
    #[cfg(zcash_unstable = "nu6")]
    Nu6,
    /// The ZFUTURE network upgrade.
    ///
    /// This upgrade is expected never to activate on mainnet;
    /// it is intended for use in integration testing of functionality
    /// that is a candidate for integration in a future network upgrade.
    #[cfg(zcash_unstable = "zfuture")]
    ZFuture,
}

memuse::impl_no_dynamic_usage!(NetworkUpgrade);

impl fmt::Display for NetworkUpgrade {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            NetworkUpgrade::Overwinter => write!(f, "Overwinter"),
            NetworkUpgrade::Sapling => write!(f, "Sapling"),
            NetworkUpgrade::Blossom => write!(f, "Blossom"),
            NetworkUpgrade::Heartwood => write!(f, "Heartwood"),
            NetworkUpgrade::Canopy => write!(f, "Canopy"),
            NetworkUpgrade::Nu5 => write!(f, "Nu5"),
            #[cfg(zcash_unstable = "nu6")]
            NetworkUpgrade::Nu6 => write!(f, "Nu6"),
            #[cfg(zcash_unstable = "zfuture")]
            NetworkUpgrade::ZFuture => write!(f, "ZFUTURE"),
        }
    }
}

impl NetworkUpgrade {
    fn branch_id(self) -> BranchId {
        match self {
            NetworkUpgrade::Overwinter => BranchId::Overwinter,
            NetworkUpgrade::Sapling => BranchId::Sapling,
            NetworkUpgrade::Blossom => BranchId::Blossom,
            NetworkUpgrade::Heartwood => BranchId::Heartwood,
            NetworkUpgrade::Canopy => BranchId::Canopy,
            NetworkUpgrade::Nu5 => BranchId::Nu5,
            #[cfg(zcash_unstable = "nu6")]
            NetworkUpgrade::Nu6 => BranchId::Nu6,
            #[cfg(zcash_unstable = "zfuture")]
            NetworkUpgrade::ZFuture => BranchId::ZFuture,
        }
    }
}

/// The network upgrades on the Zcash chain in order of activation.
///
/// This order corresponds to the activation heights, but because Rust enums are
/// full-fledged algebraic data types, we need to define it manually.
const UPGRADES_IN_ORDER: &[NetworkUpgrade] = &[
    NetworkUpgrade::Overwinter,
    NetworkUpgrade::Sapling,
    NetworkUpgrade::Blossom,
    NetworkUpgrade::Heartwood,
    NetworkUpgrade::Canopy,
    NetworkUpgrade::Nu5,
    #[cfg(zcash_unstable = "nu6")]
    NetworkUpgrade::Nu6,
];

/// The "grace period" defined in [ZIP 212].
///
/// [ZIP 212]: https://zips.z.cash/zip-0212#changes-to-the-process-of-receiving-sapling-or-orchard-notes
pub const ZIP212_GRACE_PERIOD: u32 = 32256;

/// A globally-unique identifier for a set of consensus rules within the Zcash chain.
///
/// Each branch ID in this enum corresponds to one of the epochs between a pair of Zcash
/// network upgrades. For example, `BranchId::Overwinter` corresponds to the blocks
/// starting at Overwinter activation, and ending the block before Sapling activation.
///
/// The main use of the branch ID is in signature generation: transactions commit to a
/// specific branch ID by including it as part of [`signature_hash`]. This ensures
/// two-way replay protection for transactions across network upgrades.
///
/// See [ZIP 200](https://zips.z.cash/zip-0200) for more details.
///
/// [`signature_hash`]: https://docs.rs/zcash_primitives/latest/zcash_primitives/transaction/sighash/fn.signature_hash.html
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BranchId {
    /// The consensus rules at the launch of Zcash.
    Sprout,
    /// The consensus rules deployed by [`NetworkUpgrade::Overwinter`].
    Overwinter,
    /// The consensus rules deployed by [`NetworkUpgrade::Sapling`].
    Sapling,
    /// The consensus rules deployed by [`NetworkUpgrade::Blossom`].
    Blossom,
    /// The consensus rules deployed by [`NetworkUpgrade::Heartwood`].
    Heartwood,
    /// The consensus rules deployed by [`NetworkUpgrade::Canopy`].
    Canopy,
    /// The consensus rules deployed by [`NetworkUpgrade::Nu5`].
    Nu5,
    /// The consensus rules deployed by [`NetworkUpgrade::Nu6`].
    #[cfg(zcash_unstable = "nu6")]
    Nu6,
    /// Candidates for future consensus rules; this branch will never
    /// activate on mainnet.
    #[cfg(zcash_unstable = "zfuture")]
    ZFuture,
}

memuse::impl_no_dynamic_usage!(BranchId);

impl TryFrom<u32> for BranchId {
    type Error = &'static str;

    fn try_from(value: u32) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(BranchId::Sprout),
            0x5ba8_1b19 => Ok(BranchId::Overwinter),
            0x76b8_09bb => Ok(BranchId::Sapling),
            0x2bb4_0e60 => Ok(BranchId::Blossom),
            0xf5b9_230b => Ok(BranchId::Heartwood),
            0xe9ff_75a6 => Ok(BranchId::Canopy),
            0xc2d6_d0b4 => Ok(BranchId::Nu5),
            #[cfg(zcash_unstable = "nu6")]
            0xc8e7_1055 => Ok(BranchId::Nu6),
            #[cfg(zcash_unstable = "zfuture")]
            0xffff_ffff => Ok(BranchId::ZFuture),
            _ => Err("Unknown consensus branch ID"),
        }
    }
}

impl From<BranchId> for u32 {
    fn from(consensus_branch_id: BranchId) -> u32 {
        match consensus_branch_id {
            BranchId::Sprout => 0,
            BranchId::Overwinter => 0x5ba8_1b19,
            BranchId::Sapling => 0x76b8_09bb,
            BranchId::Blossom => 0x2bb4_0e60,
            BranchId::Heartwood => 0xf5b9_230b,
            BranchId::Canopy => 0xe9ff_75a6,
            BranchId::Nu5 => 0xc2d6_d0b4,
            #[cfg(zcash_unstable = "nu6")]
            BranchId::Nu6 => 0xc8e7_1055,
            #[cfg(zcash_unstable = "zfuture")]
            BranchId::ZFuture => 0xffff_ffff,
        }
    }
}

impl BranchId {
    /// Returns the branch ID corresponding to the consensus rule set that is active at
    /// the given height.
    ///
    /// This is the branch ID that should be used when creating transactions.
    pub fn for_height<P: Parameters>(parameters: &P, height: BlockHeight) -> Self {
        for nu in UPGRADES_IN_ORDER.iter().rev() {
            if parameters.is_nu_active(*nu, height) {
                return nu.branch_id();
            }
        }

        // Sprout rules apply before any network upgrade
        BranchId::Sprout
    }

    /// Returns the range of heights for the consensus epoch associated with this branch id.
    ///
    /// The resulting tuple implements the [`RangeBounds<BlockHeight>`] trait.
    pub fn height_range<P: Parameters>(&self, params: &P) -> Option<impl RangeBounds<BlockHeight>> {
        self.height_bounds(params).map(|(lower, upper)| {
            (
                Bound::Included(lower),
                upper.map_or(Bound::Unbounded, Bound::Excluded),
            )
        })
    }

    /// Returns the range of heights for the consensus epoch associated with this branch id.
    ///
    /// The return type of this value is slightly more precise than [`Self::height_range`]:
    /// - `Some((x, Some(y)))` means that the consensus rules corresponding to this branch id
    ///   are in effect for the range `x..y`
    /// - `Some((x, None))` means that the consensus rules corresponding to this branch id are
    ///   in effect for the range `x..`
    /// - `None` means that the consensus rules corresponding to this branch id are never in effect.
    pub fn height_bounds<P: Parameters>(
        &self,
        params: &P,
    ) -> Option<(BlockHeight, Option<BlockHeight>)> {
        match self {
            BranchId::Sprout => params
                .activation_height(NetworkUpgrade::Overwinter)
                .map(|upper| (BlockHeight(0), Some(upper))),
            BranchId::Overwinter => params
                .activation_height(NetworkUpgrade::Overwinter)
                .map(|lower| (lower, params.activation_height(NetworkUpgrade::Sapling))),
            BranchId::Sapling => params
                .activation_height(NetworkUpgrade::Sapling)
                .map(|lower| (lower, params.activation_height(NetworkUpgrade::Blossom))),
            BranchId::Blossom => params
                .activation_height(NetworkUpgrade::Blossom)
                .map(|lower| (lower, params.activation_height(NetworkUpgrade::Heartwood))),
            BranchId::Heartwood => params
                .activation_height(NetworkUpgrade::Heartwood)
                .map(|lower| (lower, params.activation_height(NetworkUpgrade::Canopy))),
            BranchId::Canopy => params
                .activation_height(NetworkUpgrade::Canopy)
                .map(|lower| (lower, params.activation_height(NetworkUpgrade::Nu5))),
            BranchId::Nu5 => params.activation_height(NetworkUpgrade::Nu5).map(|lower| {
                #[cfg(zcash_unstable = "zfuture")]
                let upper = params.activation_height(NetworkUpgrade::ZFuture);
                #[cfg(not(zcash_unstable = "zfuture"))]
                let upper = None;
                (lower, upper)
            }),
            #[cfg(zcash_unstable = "nu6")]
            BranchId::Nu6 => None,
            #[cfg(zcash_unstable = "zfuture")]
            BranchId::ZFuture => params
                .activation_height(NetworkUpgrade::ZFuture)
                .map(|lower| (lower, None)),
        }
    }

    pub fn sprout_uses_groth_proofs(&self) -> bool {
        !matches!(self, BranchId::Sprout | BranchId::Overwinter)
    }
}

#[cfg(any(test, feature = "test-dependencies"))]
pub mod testing {
    use proptest::sample::select;
    use proptest::strategy::{Just, Strategy};

    use super::{BlockHeight, BranchId, Parameters};

    pub fn arb_branch_id() -> impl Strategy<Value = BranchId> {
        select(vec![
            BranchId::Sprout,
            BranchId::Overwinter,
            BranchId::Sapling,
            BranchId::Blossom,
            BranchId::Heartwood,
            BranchId::Canopy,
            BranchId::Nu5,
            #[cfg(zcash_unstable = "nu6")]
            BranchId::Nu6,
            #[cfg(zcash_unstable = "zfuture")]
            BranchId::ZFuture,
        ])
    }

    pub fn arb_height<P: Parameters>(
        branch_id: BranchId,
        params: &P,
    ) -> impl Strategy<Value = Option<BlockHeight>> {
        branch_id
            .height_bounds(params)
            .map_or(Strategy::boxed(Just(None)), |(lower, upper)| {
                Strategy::boxed(
                    (lower.0..upper.map_or(std::u32::MAX, |u| u.0))
                        .prop_map(|h| Some(BlockHeight(h))),
                )
            })
    }

    #[cfg(feature = "test-dependencies")]
    impl incrementalmerkletree::testing::TestCheckpoint for BlockHeight {
        fn from_u64(value: u64) -> Self {
            BlockHeight(u32::try_from(value).expect("Test checkpoint ids do not exceed 32 bits"))
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{
        BlockHeight, BranchId, NetworkUpgrade, Parameters, MAIN_NETWORK, UPGRADES_IN_ORDER,
    };
    use std::convert::TryFrom;

    #[test]
    fn nu_ordering() {
        for i in 1..UPGRADES_IN_ORDER.len() {
            let nu_a = UPGRADES_IN_ORDER[i - 1];
            let nu_b = UPGRADES_IN_ORDER[i];
            match (
                MAIN_NETWORK.activation_height(nu_a),
                MAIN_NETWORK.activation_height(nu_b),
            ) {
                (Some(a), Some(b)) if a < b => (),
                (Some(_), None) => (),
                (None, None) => (),
                _ => panic!(
                    "{} should not be before {} in UPGRADES_IN_ORDER",
                    nu_a, nu_b
                ),
            }
        }
    }

    #[test]
    fn nu_is_active() {
        assert!(!MAIN_NETWORK.is_nu_active(NetworkUpgrade::Overwinter, BlockHeight(0)));
        assert!(!MAIN_NETWORK.is_nu_active(NetworkUpgrade::Overwinter, BlockHeight(347_499)));
        assert!(MAIN_NETWORK.is_nu_active(NetworkUpgrade::Overwinter, BlockHeight(347_500)));
    }

    #[test]
    fn branch_id_from_u32() {
        assert_eq!(BranchId::try_from(0), Ok(BranchId::Sprout));
        assert!(BranchId::try_from(1).is_err());
    }

    #[test]
    fn branch_id_for_height() {
        assert_eq!(
            BranchId::for_height(&MAIN_NETWORK, BlockHeight(0)),
            BranchId::Sprout,
        );
        assert_eq!(
            BranchId::for_height(&MAIN_NETWORK, BlockHeight(419_199)),
            BranchId::Overwinter,
        );
        assert_eq!(
            BranchId::for_height(&MAIN_NETWORK, BlockHeight(419_200)),
            BranchId::Sapling,
        );
        assert_eq!(
            BranchId::for_height(&MAIN_NETWORK, BlockHeight(903_000)),
            BranchId::Heartwood,
        );
        assert_eq!(
            BranchId::for_height(&MAIN_NETWORK, BlockHeight(1_046_400)),
            BranchId::Canopy,
        );
        assert_eq!(
            BranchId::for_height(&MAIN_NETWORK, BlockHeight(1_687_104)),
            BranchId::Nu5,
        );
        assert_eq!(
            BranchId::for_height(&MAIN_NETWORK, BlockHeight(5_000_000)),
            BranchId::Nu5,
        );
    }
}
