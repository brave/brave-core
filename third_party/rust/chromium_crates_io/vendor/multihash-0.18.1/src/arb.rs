use quickcheck::Gen;
use rand::{
    distributions::{weighted::WeightedIndex, Distribution},
    Rng, RngCore, SeedableRng,
};

use arbitrary::{size_hint, Unstructured};

use crate::MultihashGeneric;

/// Generates a random valid multihash.
impl<const S: usize> quickcheck::Arbitrary for MultihashGeneric<S> {
    fn arbitrary(g: &mut Gen) -> MultihashGeneric<S> {
        // In real world lower multihash codes are more likely to happen, hence distribute them
        // with bias towards smaller values.
        let weights = [128, 64, 32, 16, 8, 4, 2, 1];
        let dist = WeightedIndex::new(weights.iter()).unwrap();
        let mut rng = rand::rngs::SmallRng::seed_from_u64(u64::arbitrary(g));
        let code = match dist.sample(&mut rng) {
            0 => rng.gen_range(0..u64::pow(2, 7)),
            1 => rng.gen_range(u64::pow(2, 7)..u64::pow(2, 14)),
            2 => rng.gen_range(u64::pow(2, 14)..u64::pow(2, 21)),
            3 => rng.gen_range(u64::pow(2, 21)..u64::pow(2, 28)),
            4 => rng.gen_range(u64::pow(2, 28)..u64::pow(2, 35)),
            5 => rng.gen_range(u64::pow(2, 35)..u64::pow(2, 42)),
            6 => rng.gen_range(u64::pow(2, 42)..u64::pow(2, 49)),
            7 => rng.gen_range(u64::pow(2, 56)..u64::pow(2, 63)),
            _ => unreachable!(),
        };

        // Maximum size is S byte due to the generic.
        let size = rng.gen_range(0..S);
        let mut data = [0; S];
        rng.fill_bytes(&mut data);
        MultihashGeneric::wrap(code, &data[..size]).unwrap()
    }
}

impl<'a, const S: usize> arbitrary::Arbitrary<'a> for MultihashGeneric<S> {
    fn arbitrary(u: &mut Unstructured<'a>) -> arbitrary::Result<Self> {
        let mut code = 0u64;
        let mut len_choice = u.arbitrary::<u8>()? | 1;

        while len_choice & 1 == 1 {
            len_choice >>= 1;

            let x = u.arbitrary::<u8>();
            let next = code
                .checked_shl(8)
                .zip(x.ok())
                .map(|(next, x)| next.saturating_add(x as u64));

            match next {
                None => break,
                Some(next) => code = next,
            }
        }

        let size = u.int_in_range(0..=S)?;
        let data = u.bytes(size)?;

        Ok(MultihashGeneric::wrap(code, data).unwrap())
    }

    fn size_hint(depth: usize) -> (usize, Option<usize>) {
        size_hint::and(<[u8; 3]>::size_hint(depth), (0, Some(S + 8)))
    }
}

#[cfg(test)]
mod tests {
    use crate::MultihashGeneric;
    use arbitrary::{Arbitrary, Unstructured};

    #[test]
    fn arbitrary() {
        let mut u = Unstructured::new(&[2, 4, 13, 5, 6, 7, 8, 9, 6]);

        let mh = <MultihashGeneric<16> as Arbitrary>::arbitrary(&mut u).unwrap();
        let mh2 = MultihashGeneric::<16>::wrap(1037, &[6, 7, 8, 9, 6]).unwrap();
        assert_eq!(mh, mh2);
    }
}
