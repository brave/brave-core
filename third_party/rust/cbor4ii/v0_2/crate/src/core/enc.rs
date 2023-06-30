//! encode module

use core::convert::TryFrom;
use crate::core::{ types, major, marker };
pub use crate::error::EncodeError as Error;


/// Write trait
///
/// This is similar to `Write` of standard library,
/// but it can define its own error type and work in `no_std`.
pub trait Write {
    #[cfg(feature = "use_std")]
    type Error: std::error::Error + 'static;

    #[cfg(not(feature = "use_std"))]
    type Error: core::fmt::Display + core::fmt::Debug;

    /// write all data
    fn push(&mut self, input: &[u8]) -> Result<(), Self::Error>;
}

/// Encode trait
pub trait Encode {
    /// Write the type to writer by CBOR encoding.
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>>;
}

impl<T: Encode> Encode for &'_ T {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        <T as Encode>::encode(self, writer)
    }
}

impl<'a, T: Write> Write for &'a mut T {
    type Error = T::Error;

    #[inline]
    fn push(&mut self, input: &[u8]) -> Result<(), Self::Error> {
        (**self).push(input)
    }
}

struct TypeNum<V> {
    type_: u8,
    value: V
}

impl<V> TypeNum<V> {
    #[inline]
    const fn new(type_: u8, value: V) -> TypeNum<V> {
        TypeNum { type_, value }
    }
}

impl Encode for TypeNum<u8> {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        match self.value {
            x @ 0x00 ..= 0x17 => writer.push(&[self.type_ | x])?,
            x => writer.push(&[self.type_ | 0x18, x])?
        }
        Ok(())
    }
}

impl Encode for TypeNum<u16> {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        match u8::try_from(self.value) {
            Ok(x) => TypeNum::new(self.type_, x).encode(writer)?,
            Err(_) => {
                let [x0, x1] = self.value.to_be_bytes();
                writer.push(&[self.type_ | 0x19, x0, x1])?
            }
        }
        Ok(())
    }
}

impl Encode for TypeNum<u32> {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        match u16::try_from(self.value) {
            Ok(x) => TypeNum::new(self.type_, x).encode(writer)?,
            Err(_) =>{
                let [x0, x1, x2, x3] = self.value.to_be_bytes();
                writer.push(&[self.type_ | 0x1a, x0, x1, x2, x3])?;
            }
        }
        Ok(())
    }
}

impl Encode for TypeNum<u64> {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        match u32::try_from(self.value) {
            Ok(x) => TypeNum::new(self.type_, x).encode(writer)?,
            Err(_) => {
                let [x0, x1, x2, x3, x4, x5, x6, x7] = self.value.to_be_bytes();
                writer.push(&[self.type_ | 0x1b, x0, x1, x2, x3, x4, x5, x6, x7])?;
            }
        }
        Ok(())
    }
}

#[inline]
fn strip_zero(input: &[u8]) -> &[u8] {
    let pos = input.iter()
        .position(|&n| n != 0x0)
        .unwrap_or(input.len());
    &input[pos..]
}

impl Encode for u128 {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        let x = *self;
        match u64::try_from(x) {
            Ok(x) => TypeNum::new(major::UNSIGNED << 5, x).encode(writer),
            Err(_) => {
                let x = x.to_be_bytes();
                let bytes = types::Bytes(strip_zero(&x));
                types::Tag(2, bytes).encode(writer)
            }
        }
    }
}

impl Encode for i128 {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        let x = *self;

        if let Ok(x) = u128::try_from(x) {
            x.encode(writer)
        } else {
            let x = -1 - x;

            if let Ok(x) = u64::try_from(x) {
                types::Negative(x).encode(writer)
            } else {
                let x = x.to_be_bytes();
                let bytes = types::Bytes(strip_zero(&x));
                types::Tag(3, bytes).encode(writer)
            }
        }
    }
}

macro_rules! encode_ux {
    ( $( $t:ty ),* ) => {
        $(
            impl Encode for $t {
                #[inline]
                fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
                    TypeNum::new(major::UNSIGNED << 5, *self).encode(writer)
                }
            }
        )*
    }
}

macro_rules! encode_nx {
    ( $( $t:ty ),* ) => {
        $(
            impl Encode for types::Negative<$t> {
                #[inline]
                fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
                    TypeNum::new(major::NEGATIVE << 5, self.0).encode(writer)
                }
            }
        )*
    }
}

macro_rules! encode_ix {
    ( $( $t:ty = $t2:ty );* $( ; )? ) => {
        $(
            impl Encode for $t {
                #[inline]
                fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
                    let x = *self;
                    match <$t2>::try_from(x) {
                        Ok(x) => x.encode(writer),
                        Err(_) => types::Negative((-1 - x) as $t2).encode(writer)
                    }
                }
            }
        )*
    }
}

encode_ux!(u8, u16, u32, u64);
encode_nx!(u8, u16, u32, u64);
encode_ix!(
    i8 = u8;
    i16 = u16;
    i32 = u32;
    i64 = u64;
);

impl Encode for types::Bytes<&'_ [u8]> {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        TypeNum::new(major::BYTES << 5, self.0.len() as u64).encode(writer)?;
        writer.push(self.0)?;
        Ok(())
    }
}

pub struct BytesStart;

impl Encode for BytesStart {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        writer.push(&[(major::BYTES << 5) | marker::START])?;
        Ok(())
    }
}

impl Encode for &'_ str {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        TypeNum::new(major::STRING << 5, self.len() as u64).encode(writer)?;
        writer.push(self.as_bytes())?;
        Ok(())
    }
}

impl Encode for types::BadStr<&'_ [u8]> {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        TypeNum::new(major::STRING << 5, self.0.len() as u64).encode(writer)?;
        writer.push(self.0)?;
        Ok(())
    }
}

pub struct StrStart;

impl Encode for StrStart {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        writer.push(&[(major::STRING << 5) | marker::START])?;
        Ok(())
    }
}

impl<T: Encode> Encode for &'_ [T] {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        ArrayStartBounded(self.len()).encode(writer)?;
        for value in self.iter() {
            value.encode(writer)?;
        }
        Ok(())
    }
}

pub struct ArrayStartBounded(pub usize);
pub struct ArrayStartUnbounded;

impl Encode for ArrayStartBounded {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        TypeNum::new(major::ARRAY << 5, self.0 as u64).encode(writer)?;
        Ok(())
    }
}

impl Encode for ArrayStartUnbounded {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        writer.push(&[(major::ARRAY << 5) | marker::START])?;
        Ok(())
    }
}

impl<K: Encode, V: Encode> Encode for types::Map<&'_ [(K, V)]> {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        MapStartBounded(self.0.len()).encode(writer)?;
        for (k, v) in self.0.iter() {
            k.encode(writer)?;
            v.encode(writer)?;
        }
        Ok(())
    }
}

pub struct MapStartBounded(pub usize);
pub struct MapStartUnbounded;

impl Encode for MapStartBounded {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        TypeNum::new(major::MAP << 5, self.0 as u64).encode(writer)?;
        Ok(())
    }
}

impl Encode for MapStartUnbounded {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        writer.push(&[(major::MAP << 5) | marker::START])?;
        Ok(())
    }
}

impl<T: Encode> Encode for types::Tag<T> {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        TypeNum::new(major::TAG << 5, self.0).encode(writer)?;
        self.1.encode(writer)
    }
}

impl Encode for types::Simple {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        TypeNum::new(major::SIMPLE << 5, self.0).encode(writer)
    }
}

impl Encode for bool {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        writer.push(&[match *self {
            true => marker::TRUE,
            false => marker::FALSE
        }])?;
        Ok(())
    }
}

impl Encode for types::Null {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        writer.push(&[marker::NULL])?;
        Ok(())
    }
}

impl Encode for types::Undefined {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        writer.push(&[marker::UNDEFINED])?;
        Ok(())
    }
}

#[cfg(feature = "half-f16")]
impl Encode for half::f16 {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        let [x0, x1] = self.to_be_bytes();
        writer.push(&[marker::F16, x0, x1])?;
        Ok(())
    }
}

impl Encode for types::F16 {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        let [x0, x1] = self.0.to_be_bytes();
        writer.push(&[marker::F16, x0, x1])?;
        Ok(())
    }
}

impl Encode for f32 {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        let [x0, x1, x2, x3] = self.to_be_bytes();
        writer.push(&[marker::F32, x0, x1, x2, x3])?;
        Ok(())
    }
}

impl Encode for f64 {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        let [x0, x1, x2, x3, x4, x5, x6, x7] = self.to_be_bytes();
        writer.push(&[marker::F64, x0, x1, x2, x3, x4, x5, x6, x7])?;
        Ok(())
    }
}

pub struct End;

impl Encode for End {
    #[inline]
    fn encode<W: Write>(&self, writer: &mut W) -> Result<(), Error<W::Error>> {
        writer.push(&[marker::BREAK])?;
        Ok(())
    }
}

// from https://www.rfc-editor.org/rfc/rfc8949.html#name-examples-of-encoded-cbor-da
#[test]
#[cfg(feature = "use_std")]
fn test_encoded() -> anyhow::Result<()> {
    pub struct Buffer(Vec<u8>);

    impl Write for Buffer {
        type Error = std::convert::Infallible;

        fn push(&mut self, input: &[u8]) -> Result<(), Self::Error> {
            self.0.extend_from_slice(input);
            Ok(())
        }
    }

    fn hex(input: &[u8]) -> String {
        let mut buf = String::from("0x");
        data_encoding::HEXLOWER.encode_append(input, &mut buf);
        buf
    }

    let mut buf = Buffer(Vec::new());

    macro_rules! test {
        ( $( $( @ #[$cfg_meta:meta] )* $input:expr , $expected:expr );* $( ; )? ) => {
            $(
                $( #[$cfg_meta] )*
                {
                    buf.0.clear();
                    ($input).encode(&mut buf)?;
                    let output = hex(&buf.0);
                    assert_eq!(output, $expected, "{:?}", stringify!($input));
                }
            )*
        }
    }

    let strbuf_ud800_udd51 = {
        let iter = char::decode_utf16([0xd800u16, 0xdd51u16]);
        let mut buf = String::new();
        for ret in iter {
            buf.push(ret?);
        }
        buf
    };

    test!{
        0u64, "0x00";
        1u64, "0x01";
        10u64, "0x0a";
        23u64, "0x17";
        24u64, "0x1818";
        25u64, "0x1819";
        100u64, "0x1864";
        1000u64, "0x1903e8";
        1000000u64, "0x1a000f4240";
        1000000000000u64, "0x1b000000e8d4a51000";
        18446744073709551615u64, "0x1bffffffffffffffff";
        18446744073709551616u128, "0xc249010000000000000000";
        types::Negative((-18446744073709551616i128 - 1) as u64), "0x3bffffffffffffffff";
        -18446744073709551617i128, "0xc349010000000000000000";

        -1i64, "0x20";
        -10i64, "0x29";
        -100i64, "0x3863";
        -1000i64, "0x3903e7";

        @ #[cfg(feature = "half-f16")] half::f16::from_f32(0.0), "0xf90000";
        @ #[cfg(feature = "half-f16")] half::f16::from_f32(-0.0), "0xf98000";
        @ #[cfg(feature = "half-f16")] half::f16::from_f32(1.0), "0xf93c00";
        1.1f64, "0xfb3ff199999999999a";
        @ #[cfg(feature = "half-f16")] half::f16::from_f32(1.5), "0xf93e00";
        @ #[cfg(feature = "half-f16")] half::f16::from_f32(65504.0), "0xf97bff";
        100000.0f32, "0xfa47c35000";
        3.4028234663852886e+38f32, "0xfa7f7fffff";
        1.0e+300f64, "0xfb7e37e43c8800759c";
        @ #[cfg(feature = "half-f16")] half::f16::from_f32(5.960464477539063e-8), "0xf90001";
        @ #[cfg(feature = "half-f16")] half::f16::from_f32(0.00006103515625), "0xf90400";
        @ #[cfg(feature = "half-f16")] half::f16::from_f32(-4.0), "0xf9c400";
        -4.1f64, "0xfbc010666666666666";
        @ #[cfg(feature = "half-f16")] half::f16::INFINITY, "0xf97c00";
        @ #[cfg(feature = "half-f16")] half::f16::NAN, "0xf97e00";
        @ #[cfg(feature = "half-f16")] half::f16::NEG_INFINITY, "0xf9fc00";
        f32::INFINITY, "0xfa7f800000";
        f32::NAN, "0xfa7fc00000";
        f32::NEG_INFINITY, "0xfaff800000";
        f64::INFINITY, "0xfb7ff0000000000000";
        f64::NAN, "0xfb7ff8000000000000";
        f64::NEG_INFINITY, "0xfbfff0000000000000";

        false, "0xf4";
        true, "0xf5";
        types::Null, "0xf6";
        types::Undefined, "0xf7";

        types::Simple(16), "0xf0";
        types::Simple(255), "0xf8ff";

        types::Tag(0, "2013-03-21T20:04:00Z"), "0xc074323031332d30332d32315432303a30343a30305a";
        types::Tag(1, 1363896240u64), "0xc11a514b67b0";
        types::Tag(1, 1363896240.5f64), "0xc1fb41d452d9ec200000";
        types::Tag(23, types::Bytes(&[0x01, 0x02, 0x03, 0x04][..])), "0xd74401020304";
        types::Tag(24, types::Bytes(&[0x64, 0x49, 0x45, 0x54, 0x46][..])), "0xd818456449455446";
        types::Tag(32, "http://www.example.com"), "0xd82076687474703a2f2f7777772e6578616d706c652e636f6d";

        types::Bytes(&[0u8; 0][..]), "0x40";
        types::Bytes(&[0x01, 0x02, 0x03, 0x04][..]), "0x4401020304";

        "", "0x60";
        "a", "0x6161";
        "IETF", "0x6449455446";
        "\"\\", "0x62225c";
        "\u{00fc}", "0x62c3bc";
        "\u{6c34}", "0x63e6b0b4";
        strbuf_ud800_udd51.as_str(), "0x64f0908591";

        &[0u8; 0][..], "0x80";
        &[1u8, 2, 3][..], "0x83010203";

        // TODO any array

        &[1u8, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25][..],
        "0x98190102030405060708090a0b0c0d0e0f101112131415161718181819";

        types::Map(&[(0u8, 0u8); 0][..]), "0xa0";
        types::Map(&[(1u8, 2u8), (3u8, 4u8)][..]), "0xa201020304";

        // TODO any map

        types::Map(&[("a", "A"), ("b", "B"), ("c", "C"), ("d", "D"), ("e", "E")][..]),
        "0xa56161614161626142616361436164614461656145";

        // TODO more map and array
    }

    // [1, [2, 3], [4, 5]]
    {
        buf.0.clear();
        ArrayStartBounded(3).encode(&mut buf)?;
        1u8.encode(&mut buf)?;
        (&[2u64, 3u64][..]).encode(&mut buf)?;
        (&[4i32, 5i32][..]).encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0x8301820203820405");
    }

    // {"a": 1, "b": [2, 3]}
    {
        buf.0.clear();
        MapStartBounded(2).encode(&mut buf)?;
        "a".encode(&mut buf)?;
        1u32.encode(&mut buf)?;
        "b".encode(&mut buf)?;
        (&[2u32, 3u32][..]).encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0xa26161016162820203");
    }

    // ["a", {"b": "c"}]
    {
        buf.0.clear();
        ArrayStartBounded(2).encode(&mut buf)?;
        "a".encode(&mut buf)?;
        MapStartBounded(1).encode(&mut buf)?;
        "b".encode(&mut buf)?;
        "c".encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0x826161a161626163");
    }

    // (_ h'0102', h'030405')
    {
        buf.0.clear();
        BytesStart.encode(&mut buf)?;
        types::Bytes(&[0x01, 0x02][..]).encode(&mut buf)?;
        types::Bytes(&[0x03, 0x04, 0x05][..]).encode(&mut buf)?;
        End.encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0x5f42010243030405ff");
    }

    // (_ "strea", "ming")
    {
        buf.0.clear();
        StrStart.encode(&mut buf)?;
        "strea".encode(&mut buf)?;
        "ming".encode(&mut buf)?;
        End.encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0x7f657374726561646d696e67ff");
    }

    // [_ ]
    {
        buf.0.clear();
        ArrayStartUnbounded.encode(&mut buf)?;
        End.encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0x9fff");
    }

    // [_ 1, [2, 3], [_ 4, 5]]
    {
        buf.0.clear();
        ArrayStartUnbounded.encode(&mut buf)?;
        1u64.encode(&mut buf)?;
        (&[2u32, 3u32][..]).encode(&mut buf)?;
        ArrayStartUnbounded.encode(&mut buf)?;
        4u64.encode(&mut buf)?;
        5u64.encode(&mut buf)?;
        End.encode(&mut buf)?;
        End.encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0x9f018202039f0405ffff");
    }

    // [_ 1, [2, 3], [4, 5]]
    {
        buf.0.clear();
        ArrayStartUnbounded.encode(&mut buf)?;
        1u64.encode(&mut buf)?;
        (&[2u32, 3u32][..]).encode(&mut buf)?;
        (&[4u32, 5u32][..]).encode(&mut buf)?;
        End.encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0x9f01820203820405ff");
    }

    // [1, [2, 3], [_ 4, 5]]
    {
        buf.0.clear();
        ArrayStartBounded(3).encode(&mut buf)?;
        1u64.encode(&mut buf)?;
        (&[2u32, 3u32][..]).encode(&mut buf)?;
        ArrayStartUnbounded.encode(&mut buf)?;
        4u64.encode(&mut buf)?;
        5u64.encode(&mut buf)?;
        End.encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0x83018202039f0405ff");
    }

    // [1, [_ 2, 3], [4, 5]]
    {
        buf.0.clear();
        ArrayStartBounded(3).encode(&mut buf)?;
        1u64.encode(&mut buf)?;
        ArrayStartUnbounded.encode(&mut buf)?;
        2u64.encode(&mut buf)?;
        3u64.encode(&mut buf)?;
        End.encode(&mut buf)?;
        (&[4u32, 5u32][..]).encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0x83019f0203ff820405");
    }

    // [_ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25]
    {
        buf.0.clear();
        ArrayStartUnbounded.encode(&mut buf)?;
        for i in 1u32..=25 {
            i.encode(&mut buf)?;
        }
        End.encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0x9f0102030405060708090a0b0c0d0e0f101112131415161718181819ff");
    }

    // {_ "a": 1, "b": [_ 2, 3]}
    {
        buf.0.clear();
        MapStartUnbounded.encode(&mut buf)?;
        "a".encode(&mut buf)?;
        1u32.encode(&mut buf)?;
        "b".encode(&mut buf)?;
        ArrayStartUnbounded.encode(&mut buf)?;
        2i32.encode(&mut buf)?;
        3i32.encode(&mut buf)?;
        End.encode(&mut buf)?;
        End.encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0xbf61610161629f0203ffff");
    }

    // ["a", {_ "b": "c"}]
    {
        buf.0.clear();
        ArrayStartBounded(2).encode(&mut buf)?;
        "a".encode(&mut buf)?;
        MapStartUnbounded.encode(&mut buf)?;
        "b".encode(&mut buf)?;
        "c".encode(&mut buf)?;
        End.encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0x826161bf61626163ff");
    }

    // {_ "Fun": true, "Amt": -2}
    {
        buf.0.clear();
        MapStartUnbounded.encode(&mut buf)?;
        "Fun".encode(&mut buf)?;
        true.encode(&mut buf)?;
        "Amt".encode(&mut buf)?;
        (-2i32).encode(&mut buf)?;
        End.encode(&mut buf)?;
        let output = hex(&buf.0);
        assert_eq!(output, "0xbf6346756ef563416d7421ff");
    }

    assert!(strip_zero(&[0x0]).is_empty());

    Ok(())
}
