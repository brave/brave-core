#![no_main]
use libfuzzer_sys::fuzz_target;

use arbitrary::Arbitrary;
use sharks::{Share, Sharks};

#[derive(Debug, Arbitrary)]
// Limit threshold parameters to 16 bits so we don't immediately oom.
struct Parameters {
    pub threshold: u16,
    pub secret: Vec<u8>,
    pub n_shares: u16,
}

fuzz_target!(|params: Parameters| {
    let sharks = Sharks(params.threshold.into());
    if let Ok(dealer) = sharks.dealer(&params.secret) {
        let _shares: Vec<Share> = dealer.take(params.n_shares.into()).collect();
    }
});
