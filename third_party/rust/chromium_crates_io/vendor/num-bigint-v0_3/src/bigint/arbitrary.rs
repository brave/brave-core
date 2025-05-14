use super::{BigInt, Sign};

use crate::std_alloc::Box;
use crate::BigUint;

#[cfg(feature = "quickcheck")]
impl quickcheck::Arbitrary for BigInt {
    fn arbitrary<G: quickcheck::Gen>(g: &mut G) -> Self {
        let positive = bool::arbitrary(g);
        let sign = if positive { Sign::Plus } else { Sign::Minus };
        Self::from_biguint(sign, BigUint::arbitrary(g))
    }

    fn shrink(&self) -> Box<dyn Iterator<Item = Self>> {
        let sign = self.sign();
        let unsigned_shrink = self.data.shrink();
        Box::new(unsigned_shrink.map(move |x| BigInt::from_biguint(sign, x)))
    }
}

#[cfg(feature = "arbitrary")]
impl arbitrary::Arbitrary for BigInt {
    fn arbitrary(u: &mut arbitrary::Unstructured<'_>) -> arbitrary::Result<Self> {
        let positive = bool::arbitrary(u)?;
        let sign = if positive { Sign::Plus } else { Sign::Minus };
        Ok(Self::from_biguint(sign, BigUint::arbitrary(u)?))
    }

    fn shrink(&self) -> Box<dyn Iterator<Item = Self>> {
        let sign = self.sign();
        let unsigned_shrink = self.data.shrink();
        Box::new(unsigned_shrink.map(move |x| BigInt::from_biguint(sign, x)))
    }
}
