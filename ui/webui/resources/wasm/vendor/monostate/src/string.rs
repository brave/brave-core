use crate::alphabet::{self, len};
use core::mem::ManuallyDrop;
use core::slice;
use core::str;

// Equivalent to `pub struct MustBeStr<const str: &'static str>;` but using
// the type encoding described in impl/src/lib.rs to avoid depending on
// #![feature(adt_const_params)] for now.
pub enum MustBeStr<V: ConstStr> {
    __Phantom(void::MustBeStr<V>),
    MustBeStr,
}

pub mod value {
    #[doc(hidden)]
    pub use crate::string::MustBeStr::MustBeStr;
}

pub trait ConstStr: Sealed {
    const VALUE: &'static str;
}

#[doc(hidden)]
impl<T> ConstStr for T
where
    T: Sealed,
{
    const VALUE: &'static str = T::__private.0;
}

pub trait Sealed {
    #[allow(private_interfaces)]
    const __private: StringValue;
}

struct StringValue(&'static str);

impl<T, const N: usize> Sealed for (len<N>, T)
where
    T: StringBuffer,
{
    #[allow(private_interfaces)]
    const __private: StringValue = StringValue(unsafe {
        str::from_utf8_unchecked(slice::from_raw_parts(
            const {
                &Cast::<T, N> {
                    encoded: ManuallyDrop::new(T::BYTES),
                }
                .array
            }
            .as_ptr(),
            N,
        ))
    });
}

union Cast<T: StringBuffer, const N: usize> {
    encoded: ManuallyDrop<T::Type>,
    array: [u8; N],
}

const TAG_CONT: u8 = 0b1000_0000;
const TAG_TWO_B: u8 = 0b1100_0000;
const TAG_THREE_B: u8 = 0b1110_0000;
const TAG_FOUR_B: u8 = 0b1111_0000;

pub unsafe trait StringBuffer {
    // SAFETY: Must contain no padding bytes. Must have alignment of 1.
    type Type: 'static;
    // SAFETY: Contents viewed as bytes must be a valid UTF-8 encoding.
    const BYTES: Self::Type;
}

unsafe impl<const CH: char> StringBuffer for alphabet::char<CH> {
    type Type = u8;
    const BYTES: Self::Type = CH as u8;
}

unsafe impl<const CH: char> StringBuffer for alphabet::two::char<CH> {
    type Type = [u8; 2];
    const BYTES: Self::Type = [
        ((CH as u32 >> 6) & 0x1F) as u8 | TAG_TWO_B,
        (CH as u32 & 0x3F) as u8 | TAG_CONT,
    ];
}

unsafe impl<const CH: char> StringBuffer for alphabet::three::char<CH> {
    type Type = [u8; 3];
    const BYTES: Self::Type = [
        ((CH as u32 >> 12) & 0x0F) as u8 | TAG_THREE_B,
        ((CH as u32 >> 6) & 0x3F) as u8 | TAG_CONT,
        (CH as u32 & 0x3F) as u8 | TAG_CONT,
    ];
}

unsafe impl<const CH: char> StringBuffer for alphabet::four::char<CH> {
    type Type = [u8; 4];
    const BYTES: Self::Type = [
        ((CH as u32 >> 18) & 0x07) as u8 | TAG_FOUR_B,
        ((CH as u32 >> 12) & 0x3F) as u8 | TAG_CONT,
        ((CH as u32 >> 6) & 0x3F) as u8 | TAG_CONT,
        (CH as u32 & 0x3F) as u8 | TAG_CONT,
    ];
}

unsafe impl StringBuffer for () {
    type Type = ();
    const BYTES: Self::Type = ();
}

#[repr(C)]
pub struct Concat2<A, B>(A, B);

unsafe impl<A, B> StringBuffer for (A, B)
where
    A: StringBuffer,
    B: StringBuffer,
{
    type Type = Concat2<A::Type, B::Type>;
    const BYTES: Self::Type = Concat2(A::BYTES, B::BYTES);
}

#[repr(C)]
pub struct Concat3<A, B, C>(A, B, C);

unsafe impl<A, B, C> StringBuffer for (A, B, C)
where
    A: StringBuffer,
    B: StringBuffer,
    C: StringBuffer,
{
    type Type = Concat3<A::Type, B::Type, C::Type>;
    const BYTES: Self::Type = Concat3(A::BYTES, B::BYTES, C::BYTES);
}

#[repr(C)]
pub struct Concat4<A, B, C, D>(A, B, C, D);

unsafe impl<A, B, C, D> StringBuffer for (A, B, C, D)
where
    A: StringBuffer,
    B: StringBuffer,
    C: StringBuffer,
    D: StringBuffer,
{
    type Type = Concat4<A::Type, B::Type, C::Type, D::Type>;
    const BYTES: Self::Type = Concat4(A::BYTES, B::BYTES, C::BYTES, D::BYTES);
}

#[repr(C)]
pub struct Concat5<A, B, C, D, E>(A, B, C, D, E);

unsafe impl<A, B, C, D, E> StringBuffer for (A, B, C, D, E)
where
    A: StringBuffer,
    B: StringBuffer,
    C: StringBuffer,
    D: StringBuffer,
    E: StringBuffer,
{
    type Type = Concat5<A::Type, B::Type, C::Type, D::Type, E::Type>;
    const BYTES: Self::Type = Concat5(A::BYTES, B::BYTES, C::BYTES, D::BYTES, E::BYTES);
}

#[repr(C)]
pub struct Concat6<A, B, C, D, E, F>(A, B, C, D, E, F);

unsafe impl<A, B, C, D, E, F> StringBuffer for (A, B, C, D, E, F)
where
    A: StringBuffer,
    B: StringBuffer,
    C: StringBuffer,
    D: StringBuffer,
    E: StringBuffer,
    F: StringBuffer,
{
    type Type = Concat6<A::Type, B::Type, C::Type, D::Type, E::Type, F::Type>;
    const BYTES: Self::Type = Concat6(A::BYTES, B::BYTES, C::BYTES, D::BYTES, E::BYTES, F::BYTES);
}

mod void {
    use core::marker::PhantomData;

    enum Void {}

    impl Copy for Void {}

    impl Clone for Void {
        fn clone(&self) -> Self {
            *self
        }
    }

    pub struct MustBeStr<T>(PhantomData<T>, Void);

    impl<T> Copy for MustBeStr<T> {}

    impl<T> Clone for MustBeStr<T> {
        fn clone(&self) -> Self {
            *self
        }
    }
}
