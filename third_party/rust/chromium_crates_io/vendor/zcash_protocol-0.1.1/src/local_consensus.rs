use crate::consensus::{BlockHeight, NetworkType, NetworkUpgrade, Parameters};

/// a `LocalNetwork` setup should define the activation heights
/// of network upgrades. `None` is considered as "not activated"
/// These heights are not validated. Callers shall initialized
/// them according to the settings used on the Full Nodes they
/// are connecting to.
///
/// Example:
///     Regtest Zcashd using the following `zcash.conf`
///     ```
///     ## NUPARAMS
///     nuparams=5ba81b19:1 # Overwinter
///     nuparams=76b809bb:1 # Sapling
///     nuparams=2bb40e60:1 # Blossom
///     nuparams=f5b9230b:1 # Heartwood
///     nuparams=e9ff75a6:1 # Canopy
///     nuparams=c2d6d0b4:1 # NU5
///     ```
///     would use the following `LocalNetwork` struct
///     ```
///     let regtest = LocalNetwork {
///         overwinter: Some(BlockHeight::from_u32(1)),
///         sapling: Some(BlockHeight::from_u32(1)),
///         blossom: Some(BlockHeight::from_u32(1)),
///         heartwood: Some(BlockHeight::from_u32(1)),
///         canopy: Some(BlockHeight::from_u32(1)),
///         nu5: Some(BlockHeight::from_u32(1)),
///         };
///     ```
///     
#[derive(Clone, PartialEq, Eq, Copy, Debug, Hash)]
pub struct LocalNetwork {
    pub overwinter: Option<BlockHeight>,
    pub sapling: Option<BlockHeight>,
    pub blossom: Option<BlockHeight>,
    pub heartwood: Option<BlockHeight>,
    pub canopy: Option<BlockHeight>,
    pub nu5: Option<BlockHeight>,
    #[cfg(zcash_unstable = "nu6")]
    pub nu6: Option<BlockHeight>,
    #[cfg(zcash_unstable = "zfuture")]
    pub z_future: Option<BlockHeight>,
}

/// Parameters implementation for `LocalNetwork`
impl Parameters for LocalNetwork {
    fn network_type(&self) -> NetworkType {
        NetworkType::Regtest
    }

    fn activation_height(&self, nu: NetworkUpgrade) -> Option<BlockHeight> {
        match nu {
            NetworkUpgrade::Overwinter => self.overwinter,
            NetworkUpgrade::Sapling => self.sapling,
            NetworkUpgrade::Blossom => self.blossom,
            NetworkUpgrade::Heartwood => self.heartwood,
            NetworkUpgrade::Canopy => self.canopy,
            NetworkUpgrade::Nu5 => self.nu5,
            #[cfg(zcash_unstable = "nu6")]
            NetworkUpgrade::Nu6 => self.nu6,
            #[cfg(zcash_unstable = "zfuture")]
            NetworkUpgrade::ZFuture => self.z_future,
        }
    }
}

#[cfg(test)]
mod tests {
    use crate::{
        consensus::{BlockHeight, NetworkConstants, NetworkUpgrade, Parameters},
        constants,
        local_consensus::LocalNetwork,
    };

    #[test]
    fn regtest_nu_activation() {
        let expected_overwinter = BlockHeight::from_u32(1);
        let expected_sapling = BlockHeight::from_u32(2);
        let expected_blossom = BlockHeight::from_u32(3);
        let expected_heartwood = BlockHeight::from_u32(4);
        let expected_canopy = BlockHeight::from_u32(5);
        let expected_nu5 = BlockHeight::from_u32(6);
        #[cfg(zcash_unstable = "nu6")]
        let expected_nu6 = BlockHeight::from_u32(7);
        #[cfg(zcash_unstable = "zfuture")]
        let expected_z_future = BlockHeight::from_u32(7);

        let regtest = LocalNetwork {
            overwinter: Some(expected_overwinter),
            sapling: Some(expected_sapling),
            blossom: Some(expected_blossom),
            heartwood: Some(expected_heartwood),
            canopy: Some(expected_canopy),
            nu5: Some(expected_nu5),
            #[cfg(zcash_unstable = "nu6")]
            nu6: Some(expected_nu6),
            #[cfg(zcash_unstable = "zfuture")]
            z_future: Some(expected_z_future),
        };

        assert!(regtest.is_nu_active(NetworkUpgrade::Overwinter, expected_overwinter));
        assert!(regtest.is_nu_active(NetworkUpgrade::Sapling, expected_sapling));
        assert!(regtest.is_nu_active(NetworkUpgrade::Blossom, expected_blossom));
        assert!(regtest.is_nu_active(NetworkUpgrade::Heartwood, expected_heartwood));
        assert!(regtest.is_nu_active(NetworkUpgrade::Canopy, expected_canopy));
        assert!(regtest.is_nu_active(NetworkUpgrade::Nu5, expected_nu5));
        #[cfg(zcash_unstable = "nu6")]
        assert!(regtest.is_nu_active(NetworkUpgrade::Nu6, expected_nu6));
        #[cfg(zcash_unstable = "zfuture")]
        assert!(!regtest.is_nu_active(NetworkUpgrade::ZFuture, expected_nu5));
    }

    #[test]
    fn regtest_activation_heights() {
        let expected_overwinter = BlockHeight::from_u32(1);
        let expected_sapling = BlockHeight::from_u32(2);
        let expected_blossom = BlockHeight::from_u32(3);
        let expected_heartwood = BlockHeight::from_u32(4);
        let expected_canopy = BlockHeight::from_u32(5);
        let expected_nu5 = BlockHeight::from_u32(6);
        #[cfg(zcash_unstable = "nu6")]
        let expected_nu6 = BlockHeight::from_u32(7);
        #[cfg(zcash_unstable = "zfuture")]
        let expected_z_future = BlockHeight::from_u32(7);

        let regtest = LocalNetwork {
            overwinter: Some(expected_overwinter),
            sapling: Some(expected_sapling),
            blossom: Some(expected_blossom),
            heartwood: Some(expected_heartwood),
            canopy: Some(expected_canopy),
            nu5: Some(expected_nu5),
            #[cfg(zcash_unstable = "nu6")]
            nu6: Some(expected_nu6),
            #[cfg(zcash_unstable = "zfuture")]
            z_future: Some(expected_z_future),
        };

        assert_eq!(
            regtest.activation_height(NetworkUpgrade::Overwinter),
            Some(expected_overwinter)
        );
        assert_eq!(
            regtest.activation_height(NetworkUpgrade::Sapling),
            Some(expected_sapling)
        );
        assert_eq!(
            regtest.activation_height(NetworkUpgrade::Blossom),
            Some(expected_blossom)
        );
        assert_eq!(
            regtest.activation_height(NetworkUpgrade::Heartwood),
            Some(expected_heartwood)
        );
        assert_eq!(
            regtest.activation_height(NetworkUpgrade::Canopy),
            Some(expected_canopy)
        );
        assert_eq!(
            regtest.activation_height(NetworkUpgrade::Nu5),
            Some(expected_nu5)
        );
        #[cfg(zcash_unstable = "zfuture")]
        assert_eq!(
            regtest.activation_height(NetworkUpgrade::ZFuture),
            Some(expected_z_future)
        );
    }

    #[test]
    fn regtests_constants() {
        let expected_overwinter = BlockHeight::from_u32(1);
        let expected_sapling = BlockHeight::from_u32(2);
        let expected_blossom = BlockHeight::from_u32(3);
        let expected_heartwood = BlockHeight::from_u32(4);
        let expected_canopy = BlockHeight::from_u32(5);
        let expected_nu5 = BlockHeight::from_u32(6);
        #[cfg(zcash_unstable = "nu6")]
        let expected_nu6 = BlockHeight::from_u32(7);
        #[cfg(zcash_unstable = "zfuture")]
        let expected_z_future = BlockHeight::from_u32(7);

        let regtest = LocalNetwork {
            overwinter: Some(expected_overwinter),
            sapling: Some(expected_sapling),
            blossom: Some(expected_blossom),
            heartwood: Some(expected_heartwood),
            canopy: Some(expected_canopy),
            nu5: Some(expected_nu5),
            #[cfg(zcash_unstable = "nu6")]
            nu6: Some(expected_nu6),
            #[cfg(zcash_unstable = "zfuture")]
            z_future: Some(expected_z_future),
        };

        assert_eq!(regtest.coin_type(), constants::regtest::COIN_TYPE);
        assert_eq!(
            regtest.hrp_sapling_extended_spending_key(),
            constants::regtest::HRP_SAPLING_EXTENDED_SPENDING_KEY
        );
        assert_eq!(
            regtest.hrp_sapling_extended_full_viewing_key(),
            constants::regtest::HRP_SAPLING_EXTENDED_FULL_VIEWING_KEY
        );
        assert_eq!(
            regtest.hrp_sapling_payment_address(),
            constants::regtest::HRP_SAPLING_PAYMENT_ADDRESS
        );
        assert_eq!(
            regtest.b58_pubkey_address_prefix(),
            constants::regtest::B58_PUBKEY_ADDRESS_PREFIX
        );
        assert_eq!(
            regtest.b58_script_address_prefix(),
            constants::regtest::B58_SCRIPT_ADDRESS_PREFIX
        );
    }
}
