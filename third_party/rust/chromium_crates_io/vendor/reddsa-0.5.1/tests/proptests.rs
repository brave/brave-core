use core::convert::TryFrom;

use proptest::prelude::*;
use rand_core::{CryptoRng, RngCore};

use reddsa::*;

/// A signature test-case, containing signature data and expected validity.
#[derive(Clone, Debug)]
struct SignatureCase<T: SigType> {
    msg: Vec<u8>,
    sig: Signature<T>,
    pk_bytes: VerificationKeyBytes<T>,
    invalid_pk_bytes: VerificationKeyBytes<T>,
    is_valid: bool,
}

/// A modification to a test-case.
#[derive(Copy, Clone, Debug)]
enum Tweak {
    /// No-op, used to check that unchanged cases verify.
    None,
    /// Change the message the signature is defined for, invalidating the signature.
    ChangeMessage,
    /// Change the public key the signature is defined for, invalidating the signature.
    ChangePubkey,
    /* XXX implement this -- needs to regenerate a custom signature because the
       nonce commitment is fed into the hash, so it has to have torsion at signing
       time.
    /// Change the case to have a torsion component in the signature's `r` value.
    AddTorsion,
    */
    /* XXX implement this -- needs custom handling of field arithmetic.
    /// Change the signature's `s` scalar to be unreduced (mod L), invalidating the signature.
    UnreducedScalar,
    */
}

impl<T: SigType> SignatureCase<T> {
    fn new<R: RngCore + CryptoRng>(mut rng: R, msg: Vec<u8>) -> Self {
        let sk = SigningKey::new(&mut rng);
        let sig = sk.sign(&mut rng, &msg);
        let pk_bytes = VerificationKey::from(&sk).into();
        let invalid_pk_bytes = VerificationKey::from(&SigningKey::new(&mut rng)).into();
        Self {
            msg,
            sig,
            pk_bytes,
            invalid_pk_bytes,
            is_valid: true,
        }
    }

    // Check that signature verification succeeds or fails, as expected.
    fn check(&self) -> bool {
        // The signature data is stored in (refined) byte types, but do a round trip
        // conversion to raw bytes to exercise those code paths.
        let sig = {
            let bytes: [u8; 64] = self.sig.into();
            Signature::<T>::from(bytes)
        };
        let pk_bytes = {
            let bytes: [u8; 32] = self.pk_bytes.into();
            VerificationKeyBytes::<T>::from(bytes)
        };

        // Check that the verification key is a valid RedDSA verification key.
        let pub_key = VerificationKey::try_from(pk_bytes)
            .expect("The test verification key to be well-formed.");

        // Check that signature validation has the expected result.
        self.is_valid == pub_key.verify(&self.msg, &sig).is_ok()
    }

    fn apply_tweak(&mut self, tweak: &Tweak) {
        match tweak {
            Tweak::None => {}
            Tweak::ChangeMessage => {
                // Changing the message makes the signature invalid.
                self.msg.push(90);
                self.is_valid = false;
            }
            Tweak::ChangePubkey => {
                // Changing the public key makes the signature invalid.
                self.pk_bytes = self.invalid_pk_bytes;
                self.is_valid = false;
            }
        }
    }
}

fn tweak_strategy() -> impl Strategy<Value = Tweak> {
    prop_oneof![
        10 => Just(Tweak::None),
        1 => Just(Tweak::ChangeMessage),
        1 => Just(Tweak::ChangePubkey),
    ]
}

use rand_chacha::ChaChaRng;
use rand_core::SeedableRng;

proptest! {

     #[test]
    fn tweak_signature(
        tweaks in prop::collection::vec(tweak_strategy(), (0,5)),
        rng_seed in prop::array::uniform32(any::<u8>()),
    ) {
        // Use a deterministic RNG so that test failures can be reproduced.
        // Seeding with 64 bits of entropy is INSECURE and this code should
        // not be copied outside of this test!
        let mut rng = ChaChaRng::from_seed(rng_seed);

        // Create a test case for each signature type.
        let msg = b"test message for proptests";
        let mut binding = SignatureCase::<sapling::Binding>::new(&mut rng, msg.to_vec());
        let mut spendauth = SignatureCase::<sapling::SpendAuth>::new(&mut rng, msg.to_vec());

        // Apply tweaks to each case.
        for t in &tweaks {
            binding.apply_tweak(t);
            spendauth.apply_tweak(t);
        }

        assert!(binding.check());
        assert!(spendauth.check());
    }

    #[test]
    fn randomization_commutes_with_pubkey_homomorphism(rng_seed in prop::array::uniform32(any::<u8>())) {
        // Use a deterministic RNG so that test failures can be reproduced.
        let mut rng = ChaChaRng::from_seed(rng_seed);

        let r = {
            // XXX-jubjub: better API for this
            let mut bytes = [0; 64];
            rng.fill_bytes(&mut bytes[..]);
            jubjub::Scalar::from_bytes_wide(&bytes)
        };

        let sk = SigningKey::<sapling::SpendAuth>::new(&mut rng);
        let pk = VerificationKey::from(&sk);

        let sk_r = sk.randomize(&r);
        let pk_r = pk.randomize(&r);

        let pk_r_via_sk_rand: [u8; 32] = VerificationKeyBytes::from(VerificationKey::from(&sk_r)).into();
        let pk_r_via_pk_rand: [u8; 32] = VerificationKeyBytes::from(pk_r).into();

        assert_eq!(pk_r_via_pk_rand, pk_r_via_sk_rand);
    }
}
