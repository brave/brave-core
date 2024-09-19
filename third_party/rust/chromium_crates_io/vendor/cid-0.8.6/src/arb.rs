#![cfg(feature = "arb")]

use std::convert::TryFrom;

use multihash::{Code, MultihashDigest, MultihashGeneric};
use quickcheck::Gen;
use rand::{
    distributions::{weighted::WeightedIndex, Distribution},
    Rng,
};

use arbitrary::{size_hint, Unstructured};

use crate::{CidGeneric, Version};

impl quickcheck::Arbitrary for Version {
    fn arbitrary<G: Gen>(g: &mut G) -> Self {
        let version = if g.gen_bool(0.7) { 1 } else { 0 };
        Version::try_from(version).unwrap()
    }
}

impl<const S: usize> quickcheck::Arbitrary for CidGeneric<S> {
    fn arbitrary<G: Gen>(g: &mut G) -> Self {
        if S >= 32 && <Version as quickcheck::Arbitrary>::arbitrary(g) == Version::V0 {
            let data: Vec<u8> = quickcheck::Arbitrary::arbitrary(g);
            let hash = Code::Sha2_256
                .digest(&data)
                .resize()
                .expect("digest too large");
            CidGeneric::new_v0(hash).expect("sha2_256 is a valid hash for cid v0")
        } else {
            // In real world lower IPLD Codec codes more likely to happen, hence distribute them
            // with bias towards smaller values.
            let weights = [128, 32, 4, 4, 2, 2, 1, 1];
            let dist = WeightedIndex::new(weights.iter()).unwrap();
            let codec = match dist.sample(g) {
                0 => g.gen_range(0, u64::pow(2, 7)),
                1 => g.gen_range(u64::pow(2, 7), u64::pow(2, 14)),
                2 => g.gen_range(u64::pow(2, 14), u64::pow(2, 21)),
                3 => g.gen_range(u64::pow(2, 21), u64::pow(2, 28)),
                4 => g.gen_range(u64::pow(2, 28), u64::pow(2, 35)),
                5 => g.gen_range(u64::pow(2, 35), u64::pow(2, 42)),
                6 => g.gen_range(u64::pow(2, 42), u64::pow(2, 49)),
                7 => g.gen_range(u64::pow(2, 56), u64::pow(2, 63)),
                _ => unreachable!(),
            };

            let hash: MultihashGeneric<S> = quickcheck::Arbitrary::arbitrary(g);
            CidGeneric::new_v1(codec, hash)
        }
    }
}

impl<'a, const S: usize> arbitrary::Arbitrary<'a> for CidGeneric<S> {
    fn arbitrary(u: &mut Unstructured<'a>) -> arbitrary::Result<Self> {
        if S >= 32 && u.ratio(1, 10)? {
            let mh = MultihashGeneric::wrap(Code::Sha2_256.into(), u.bytes(32)?).unwrap();
            return Ok(CidGeneric::new_v0(mh).expect("32 bytes is correct for v0"));
        }

        let mut codec = 0u64;
        let mut len_choice = u.arbitrary::<u8>()? | 1;

        while len_choice & 1 == 1 {
            len_choice >>= 1;

            let x = u.arbitrary::<u8>();
            let next = codec
                .checked_shl(8)
                .zip(x.ok())
                .map(|(next, x)| next.saturating_add(x as u64));

            match next {
                None => break,
                Some(next) => codec = next,
            }
        }

        Ok(CidGeneric::new_v1(codec, u.arbitrary()?))
    }

    fn size_hint(depth: usize) -> (usize, Option<usize>) {
        let v1 = size_hint::and_all(&[
            <[u8; 2]>::size_hint(depth),
            (0, Some(8)),
            <MultihashGeneric<S> as arbitrary::Arbitrary>::size_hint(depth),
        ]);
        if S >= 32 {
            size_hint::and(<u8>::size_hint(depth), size_hint::or((32, Some(32)), v1))
        } else {
            v1
        }
    }
}

#[cfg(test)]
mod tests {
    use crate::CidGeneric;
    use arbitrary::{Arbitrary, Unstructured};
    use multihash::MultihashGeneric;

    #[test]
    fn arbitrary() {
        let mut u = Unstructured::new(&[
            1, 22, 41, 13, 5, 6, 7, 8, 9, 6, 10, 243, 43, 231, 123, 43, 153, 127, 67, 76, 24, 91,
            23, 32, 32, 23, 65, 98, 193, 108, 3,
        ]);
        let c = <CidGeneric<16> as Arbitrary>::arbitrary(&mut u).unwrap();
        let c2 =
            CidGeneric::<16>::new_v1(22, MultihashGeneric::wrap(13, &[6, 7, 8, 9, 6]).unwrap());
        assert_eq!(c.hash(), c2.hash());
        assert_eq!(c.codec(), c2.codec());
        assert_eq!(c, c2)
    }
}
