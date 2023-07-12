use super::{biguint_from_vec, BigUint};

use crate::big_digit::BigDigit;
use crate::std_alloc::{Box, Vec};

#[cfg(feature = "quickcheck")]
impl quickcheck::Arbitrary for BigUint {
    fn arbitrary<G: quickcheck::Gen>(g: &mut G) -> Self {
        // Use arbitrary from Vec
        biguint_from_vec(Vec::<BigDigit>::arbitrary(g))
    }

    fn shrink(&self) -> Box<dyn Iterator<Item = Self>> {
        // Use shrinker from Vec
        Box::new(self.data.shrink().map(biguint_from_vec))
    }
}

#[cfg(feature = "arbitrary")]
impl arbitrary::Arbitrary for BigUint {
    fn arbitrary(u: &mut arbitrary::Unstructured<'_>) -> arbitrary::Result<Self> {
        Ok(biguint_from_vec(Vec::<BigDigit>::arbitrary(u)?))
    }

    fn shrink(&self) -> Box<dyn Iterator<Item = Self>> {
        Box::new(self.data.shrink().map(biguint_from_vec))
    }
}
