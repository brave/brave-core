use core::fmt;
use core::result;
use core::marker::PhantomData;
use core::mem::MaybeUninit;
use serde::ser::{Serialize, Serializer, SerializeTuple};
use serde::de::{Deserialize, Deserializer, Visitor, SeqAccess, Error};

pub trait BigArray<'de>: Sized {
    fn serialize<S>(&self, serializer: S) -> result::Result<S::Ok, S::Error>
        where S: Serializer;
    fn deserialize<D>(deserializer: D) -> result::Result<Self, D::Error>
        where D: Deserializer<'de>;
}
impl<'de, T, const N: usize> BigArray<'de> for [T; N]
    where T: Serialize + Deserialize<'de>
{
    fn serialize<S>(&self, serializer: S) -> result::Result<S::Ok, S::Error>
        where S: Serializer
    {
        let mut seq = serializer.serialize_tuple(self.len())?;
        for elem in &self[..] {
            seq.serialize_element(elem)?;
        }
        seq.end()
    }

    fn deserialize<D>(deserializer: D) -> result::Result<Self, D::Error>
        where D: Deserializer<'de>
    {
        struct ArrayVisitor<T> {
            element: PhantomData<T>,
        }

        impl<'de, T, const N: usize> Visitor<'de> for ArrayVisitor<[T; N]>
            where T: Deserialize<'de>
        {
            type Value = [T; N];

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "an array of length {}", N)
            }

            fn visit_seq<A>(self, mut seq: A) -> result::Result<[T; N], A::Error>
                where A: SeqAccess<'de>
            {
                unsafe {
                    let mut arr: MaybeUninit<[T; N]> = MaybeUninit::uninit();
                    for i in 0 .. N {
                        let p = (arr.as_mut_ptr() as * mut T).wrapping_add(i);
                        core::ptr::write(p, seq.next_element()?
                            .ok_or_else(|| Error::invalid_length(i, &self))?);
                    }
                    Ok(arr.assume_init())
                }
            }
        }

        let visitor = ArrayVisitor { element: PhantomData };
        // The allow is needed to support (32 + 33) like expressions
        #[allow(unused_parens)]
        deserializer.deserialize_tuple(N, visitor)
    }
}
