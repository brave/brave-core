use jubjub::*;
use rand_core::{RngCore, SeedableRng};
use rand_xorshift::XorShiftRng;

pub const NUM_BLACK_BOX_CHECKS: u32 = 2000;

pub fn new_rng() -> XorShiftRng {
    XorShiftRng::from_seed([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])
}

pub trait MyRandom {
    fn new_random<T: RngCore>(rng: &mut T) -> Self;
}

impl MyRandom for Fq {
    fn new_random<T: RngCore>(rng: &mut T) -> Self {
        let mut random_bytes = [0u8; 64];
        rng.fill_bytes(&mut random_bytes);
        Fq::from_bytes_wide(&random_bytes)
    }
}

impl MyRandom for Fr {
    fn new_random<T: RngCore>(rng: &mut T) -> Self {
        let mut random_bytes = [0u8; 64];
        rng.fill_bytes(&mut random_bytes);
        Fr::from_bytes_wide(&random_bytes)
    }
}
