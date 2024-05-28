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
pub use self::{
    amount::Amount
};

