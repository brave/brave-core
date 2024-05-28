use std::time::{Duration, Instant};

#[cfg(feature = "pairing")]
use bls12_381::G2Projective;
#[cfg(feature = "blst")]
use blstrs::G2Projective;

use bls_signatures::*;
use rand::{Rng, SeedableRng};
use rand_chacha::ChaCha8Rng;
use rayon::prelude::*;

macro_rules! measure {
    ($name:expr, $code:block) => {
        println!("\t{}", $name);
        let start = Instant::now();
        let mut duration = Duration::new(0, 0);

        $code;

        duration += start.elapsed();
        let total =
            { f64::from(duration.subsec_nanos()) / 1_000_000_000f64 + (duration.as_secs() as f64) };

        println!("\t  took {:.6}s", total);
    };
    ($name:expr, $num:expr, $code:block) => {
        println!("\t{}", $name);
        let start = Instant::now();
        let mut duration = Duration::new(0, 0);

        $code;

        duration += start.elapsed();

        let total =
            { f64::from(duration.subsec_nanos()) / 1_000_000_000f64 + (duration.as_secs() as f64) };
        let per_msg = {
            let avg = duration / $num as u32;
            f64::from(avg.subsec_nanos()) / 1_000_000f64 + (avg.as_secs() as f64 * 1000f64)
        };

        println!("\t  took {:.6}s ({:.3}ms per message)", total, per_msg);
    };
}

fn run(num_messages: usize) {
    println!("dancing with {} messages", num_messages);

    let mut rng = ChaCha8Rng::seed_from_u64(12);

    // generate private keys
    let private_keys: Vec<_> = (0..num_messages)
        .map(|_| PrivateKey::generate(&mut rng))
        .collect();

    // generate messages
    let messages: Vec<Vec<u8>> = (0..num_messages)
        .map(|_| (0..64).map(|_| rng.gen()).collect())
        .collect();

    // sign messages
    let sigs: Vec<Signature>;
    measure!("signing", num_messages, {
        sigs = messages
            .par_iter()
            .zip(private_keys.par_iter())
            .map(|(message, pk)| pk.sign(message))
            .collect::<Vec<Signature>>();
    });

    let aggregated_signature: Signature;
    measure!("aggregate signatures", num_messages, {
        aggregated_signature = aggregate(&sigs).expect("failed to aggregate");
    });

    let serialized_signature: Vec<_>;
    measure!("serialize signature", {
        serialized_signature = aggregated_signature.as_bytes();
    });

    let hashes: Vec<G2Projective>;
    measure!("hashing messages", num_messages, {
        hashes = messages
            .par_iter()
            .map(|message| hash(message))
            .collect::<Vec<_>>();
    });
    let public_keys: Vec<PublicKey>;
    measure!("extracting public keys", num_messages, {
        public_keys = private_keys
            .par_iter()
            .map(|pk| pk.public_key())
            .collect::<Vec<_>>();
    });

    let agg_sig: Signature;
    measure!("deserialize signature", {
        agg_sig = Signature::from_bytes(&serialized_signature).unwrap();
    });

    measure!("verification", num_messages, {
        assert!(verify(&agg_sig, &hashes, &public_keys));
    });

    measure!("verification messages", num_messages, {
        let messages = messages.iter().map(|r| &r[..]).collect::<Vec<_>>();
        assert!(verify_messages(&agg_sig, &messages[..], &public_keys));
    });
}

fn main() {
    run(10_000);
}
