use blake2b_simd::Params;
use ff::Field;
use rand_core::{CryptoRng, RngCore};

use super::{note_encryption::Zip212Enforcement, Rseed};

pub fn hash_to_scalar(persona: &[u8], a: &[u8], b: &[u8]) -> jubjub::Fr {
    let mut hasher = Params::new().hash_length(64).personal(persona).to_state();
    hasher.update(a);
    hasher.update(b);
    let ret = hasher.finalize();
    jubjub::Fr::from_bytes_wide(ret.as_array())
}

pub fn generate_random_rseed<R: RngCore + CryptoRng>(
    zip212_enforcement: Zip212Enforcement,
    rng: &mut R,
) -> Rseed {
    generate_random_rseed_internal(zip212_enforcement, rng)
}

pub(crate) fn generate_random_rseed_internal<R: RngCore>(
    zip212_enforcement: Zip212Enforcement,
    rng: &mut R,
) -> Rseed {
    match zip212_enforcement {
        Zip212Enforcement::Off => Rseed::BeforeZip212(jubjub::Fr::random(rng)),
        Zip212Enforcement::GracePeriod | Zip212Enforcement::On => {
            let mut buffer = [0u8; 32];
            rng.fill_bytes(&mut buffer);
            Rseed::AfterZip212(buffer)
        }
    }
}
