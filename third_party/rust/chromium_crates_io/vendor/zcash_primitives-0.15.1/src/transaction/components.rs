//! Structs representing the components within Zcash transactions.
pub mod amount {
    pub use zcash_protocol::value::{
        BalanceError, ZatBalance as Amount, Zatoshis as NonNegativeAmount, COIN,
    };

    #[cfg(feature = "test-dependencies")]
    pub mod testing {
        pub use zcash_protocol::value::testing::{
            arb_positive_zat_balance as arb_positive_amount, arb_zat_balance as arb_amount,
            arb_zatoshis as arb_nonnegative_amount,
        };
    }
}
pub mod orchard;
pub mod sapling;
pub mod sprout;
pub mod transparent;
#[cfg(zcash_unstable = "zfuture")]
pub mod tze;

pub use self::{
    amount::Amount,
    sprout::JsDescription,
    transparent::{OutPoint, TxIn, TxOut},
};
pub use crate::sapling::bundle::{OutputDescription, SpendDescription};

#[cfg(zcash_unstable = "zfuture")]
pub use self::tze::{TzeIn, TzeOut};

// π_A + π_B + π_C
pub const GROTH_PROOF_SIZE: usize = 48 + 96 + 48;
