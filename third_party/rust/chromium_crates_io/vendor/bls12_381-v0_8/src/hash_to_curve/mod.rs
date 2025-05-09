//! This module implements hash_to_curve, hash_to_field and related
//! hashing primitives for use with BLS signatures.

use core::ops::Add;

use subtle::Choice;

pub(crate) mod chain;

mod expand_msg;
pub use self::expand_msg::{
    ExpandMessage, ExpandMessageState, ExpandMsgXmd, ExpandMsgXof, InitExpandMessage,
};

mod map_g1;
mod map_g2;
mod map_scalar;

use crate::generic_array::{typenum::Unsigned, ArrayLength, GenericArray};

/// Enables a byte string to be hashed into one or more field elements for a given curve.
///
/// Implements [section 5 of `draft-irtf-cfrg-hash-to-curve-12`][hash_to_field].
///
/// [hash_to_field]: https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-hash-to-curve-12#section-5
pub trait HashToField: Sized {
    /// The length of the data used to produce an individual field element.
    ///
    /// This must be set to `m * L = m * ceil((ceil(log2(p)) + k) / 8)`, where `p` is the
    /// characteristic of `Self`, `m` is the extension degree of `Self`, and `k` is the
    /// security parameter.
    type InputLength: ArrayLength<u8>;

    /// Interprets the given output keying material as a big endian integer, and reduces
    /// it into a field element.
    fn from_okm(okm: &GenericArray<u8, Self::InputLength>) -> Self;

    /// Hashes a byte string of arbitrary length into one or more elements of `Self`,
    /// using [`ExpandMessage`] variant `X`.
    ///
    /// Implements [section 5.3 of `draft-irtf-cfrg-hash-to-curve-12`][hash_to_field].
    ///
    /// [hash_to_field]: https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-hash-to-curve-12#section-5.3
    fn hash_to_field<X: ExpandMessage>(message: &[u8], dst: &[u8], output: &mut [Self]) {
        let len_per_elm = Self::InputLength::to_usize();
        let len_in_bytes = output.len() * len_per_elm;
        let mut expander = X::init_expand(message, dst, len_in_bytes);

        let mut buf = GenericArray::<u8, Self::InputLength>::default();
        output.iter_mut().for_each(|item| {
            expander.read_into(&mut buf[..]);
            *item = Self::from_okm(&buf);
        });
    }
}

/// Allow conversion from the output of hashed or encoded input into points on the curve
pub trait MapToCurve: Sized {
    /// The field element type.
    type Field: Copy + Default + HashToField;

    /// Maps an element of the finite field `Self::Field` to a point on the curve `Self`.
    fn map_to_curve(elt: &Self::Field) -> Self;

    /// Clears the cofactor, sending a point on curve E to the target group (G1/G2).
    fn clear_h(&self) -> Self;
}

/// Implementation of random oracle maps to the curve.
pub trait HashToCurve<X: ExpandMessage>: MapToCurve + for<'a> Add<&'a Self, Output = Self> {
    /// Implements a uniform encoding from byte strings to elements of `Self`.
    ///
    /// This function is suitable for most applications requiring a random
    /// oracle returning points in `Self`.
    fn hash_to_curve(message: impl AsRef<[u8]>, dst: &[u8]) -> Self {
        let mut u = [Self::Field::default(); 2];
        Self::Field::hash_to_field::<X>(message.as_ref(), dst, &mut u);
        let p1 = Self::map_to_curve(&u[0]);
        let p2 = Self::map_to_curve(&u[1]);
        (p1 + &p2).clear_h()
    }

    /// Implements a **non-uniform** encoding from byte strings to elements of `Self`.
    ///
    /// The distribution of its output is not uniformly random in `Self`: the set of
    /// possible outputs of this function is only a fraction of the points in `Self`, and
    /// some elements of this set are more likely to be output than others. See
    /// [section 10.1 of `draft-irtf-cfrg-hash-to-curve-12`][encode_to_curve-distribution]
    /// for a more precise definition of `encode_to_curve`'s output distribution.
    ///
    /// [encode_to_curve-distribution]: https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-hash-to-curve-12#section-10.1
    fn encode_to_curve(message: impl AsRef<[u8]>, dst: &[u8]) -> Self {
        let mut u = [Self::Field::default(); 1];
        Self::Field::hash_to_field::<X>(message.as_ref(), dst, &mut u);
        let p = Self::map_to_curve(&u[0]);
        p.clear_h()
    }
}

impl<G, X> HashToCurve<X> for G
where
    G: MapToCurve + for<'a> Add<&'a Self, Output = Self>,
    X: ExpandMessage,
{
}

pub(crate) trait Sgn0 {
    /// Returns either 0 or 1 indicating the "sign" of x, where sgn0(x) == 1
    /// just when x is "negative". (In other words, this function always considers 0 to be positive.)
    /// <https://tools.ietf.org/html/draft-irtf-cfrg-hash-to-curve-10#section-4.1>
    /// The equivalent for draft 6 would be `lexicographically_largest`.
    fn sgn0(&self) -> Choice;
}
