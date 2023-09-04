#![no_main]
use libfuzzer_sys::fuzz_target;

use arbitrary::Arbitrary;
use sharks::{Share, Sharks};

#[derive(Debug, Arbitrary)]
// Limit threshhold to 16 bits so we don't exhaust memory.
struct Parameters {
    pub threshold: u16,
    pub shares: Vec<Share>,
}

fuzz_target!(|params: Parameters| {
    let sharks = Sharks(params.threshold.into());
    let _secret = sharks.recover(&params.shares);
});
