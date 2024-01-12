//! CBOR encoder.

use std::cmp::Ordering;
use std::collections::BTreeMap;
use std::io::Write;
use std::iter::FromIterator;
use std::ops::Deref;
use std::sync::Arc;

use byteorder::{BigEndian, ByteOrder};
use libipld_core::cid::Cid;
use libipld_core::codec::Encode;
use libipld_core::error::Result;
use libipld_core::ipld::Ipld;

use crate::cbor::{MajorKind, FALSE, TRUE};
use crate::error::NumberOutOfRange;
use crate::DagCborCodec as DagCbor;

/// Writes a null byte to a cbor encoded byte stream.
pub fn write_null<W: Write>(w: &mut W) -> Result<()> {
    w.write_all(&[0xf6])?;
    Ok(())
}

/// Writes a u8 to a cbor encoded byte stream.
pub fn write_u8<W: Write>(w: &mut W, major: MajorKind, value: u8) -> Result<()> {
    let major = major as u8;
    if value <= 0x17 {
        let buf = [major << 5 | value];
        w.write_all(&buf)?;
    } else {
        let buf = [major << 5 | 24, value];
        w.write_all(&buf)?;
    }
    Ok(())
}

/// Writes a u16 to a cbor encoded byte stream.
pub fn write_u16<W: Write>(w: &mut W, major: MajorKind, value: u16) -> Result<()> {
    if value <= u16::from(u8::max_value()) {
        write_u8(w, major, value as u8)?;
    } else {
        let mut buf = [(major as u8) << 5 | 25, 0, 0];
        BigEndian::write_u16(&mut buf[1..], value);
        w.write_all(&buf)?;
    }
    Ok(())
}

/// Writes a u32 to a cbor encoded byte stream.
pub fn write_u32<W: Write>(w: &mut W, major: MajorKind, value: u32) -> Result<()> {
    if value <= u32::from(u16::max_value()) {
        write_u16(w, major, value as u16)?;
    } else {
        let mut buf = [(major as u8) << 5 | 26, 0, 0, 0, 0];
        BigEndian::write_u32(&mut buf[1..], value);
        w.write_all(&buf)?;
    }
    Ok(())
}

/// Writes a u64 to a cbor encoded byte stream.
pub fn write_u64<W: Write>(w: &mut W, major: MajorKind, value: u64) -> Result<()> {
    if value <= u64::from(u32::max_value()) {
        write_u32(w, major, value as u32)?;
    } else {
        let mut buf = [(major as u8) << 5 | 27, 0, 0, 0, 0, 0, 0, 0, 0];
        BigEndian::write_u64(&mut buf[1..], value);
        w.write_all(&buf)?;
    }
    Ok(())
}

/// Writes a tag to a cbor encoded byte stream.
pub fn write_tag<W: Write>(w: &mut W, tag: u64) -> Result<()> {
    write_u64(w, MajorKind::Tag, tag)
}

impl Encode<DagCbor> for bool {
    fn encode<W: Write>(&self, _: DagCbor, w: &mut W) -> Result<()> {
        let buf = if *self { [TRUE.into()] } else { [FALSE.into()] };
        w.write_all(&buf)?;
        Ok(())
    }
}

impl Encode<DagCbor> for u8 {
    fn encode<W: Write>(&self, _: DagCbor, w: &mut W) -> Result<()> {
        write_u8(w, MajorKind::UnsignedInt, *self)
    }
}

impl Encode<DagCbor> for u16 {
    fn encode<W: Write>(&self, _: DagCbor, w: &mut W) -> Result<()> {
        write_u16(w, MajorKind::UnsignedInt, *self)
    }
}

impl Encode<DagCbor> for u32 {
    fn encode<W: Write>(&self, _: DagCbor, w: &mut W) -> Result<()> {
        write_u32(w, MajorKind::UnsignedInt, *self)
    }
}

impl Encode<DagCbor> for u64 {
    fn encode<W: Write>(&self, _: DagCbor, w: &mut W) -> Result<()> {
        write_u64(w, MajorKind::UnsignedInt, *self)
    }
}

impl Encode<DagCbor> for i8 {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        if self.is_negative() {
            write_u8(w, MajorKind::NegativeInt, -(*self + 1) as u8)
        } else {
            (*self as u8).encode(c, w)
        }
    }
}

impl Encode<DagCbor> for i16 {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        if self.is_negative() {
            write_u16(w, MajorKind::NegativeInt, -(*self + 1) as u16)
        } else {
            (*self as u16).encode(c, w)
        }
    }
}

impl Encode<DagCbor> for i32 {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        if self.is_negative() {
            write_u32(w, MajorKind::NegativeInt, -(*self + 1) as u32)
        } else {
            (*self as u32).encode(c, w)
        }
    }
}

impl Encode<DagCbor> for i64 {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        if self.is_negative() {
            write_u64(w, MajorKind::NegativeInt, -(*self + 1) as u64)
        } else {
            (*self as u64).encode(c, w)
        }
    }
}

impl Encode<DagCbor> for f32 {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        // IPLD maximally encodes floats.
        f64::from(*self).encode(c, w)
    }
}

impl Encode<DagCbor> for f64 {
    fn encode<W: Write>(&self, _: DagCbor, w: &mut W) -> Result<()> {
        // IPLD forbids nan, infinities, etc.
        if !self.is_finite() {
            return Err(NumberOutOfRange::new::<f64>().into());
        }
        let mut buf = [0xfb, 0, 0, 0, 0, 0, 0, 0, 0];
        BigEndian::write_f64(&mut buf[1..], *self);
        w.write_all(&buf)?;
        Ok(())
    }
}

impl Encode<DagCbor> for [u8] {
    fn encode<W: Write>(&self, _: DagCbor, w: &mut W) -> Result<()> {
        write_u64(w, MajorKind::ByteString, self.len() as u64)?;
        w.write_all(self)?;
        Ok(())
    }
}

impl Encode<DagCbor> for Box<[u8]> {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        self[..].encode(c, w)
    }
}

impl Encode<DagCbor> for str {
    fn encode<W: Write>(&self, _: DagCbor, w: &mut W) -> Result<()> {
        write_u64(w, MajorKind::TextString, self.len() as u64)?;
        w.write_all(self.as_bytes())?;
        Ok(())
    }
}

impl Encode<DagCbor> for String {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        self.as_str().encode(c, w)
    }
}

impl Encode<DagCbor> for i128 {
    fn encode<W: Write>(&self, _: DagCbor, w: &mut W) -> Result<()> {
        if *self < 0 {
            if -(*self + 1) > u64::max_value() as i128 {
                return Err(NumberOutOfRange::new::<i128>().into());
            }
            write_u64(w, MajorKind::NegativeInt, -(*self + 1) as u64)?;
        } else {
            if *self > u64::max_value() as i128 {
                return Err(NumberOutOfRange::new::<i128>().into());
            }
            write_u64(w, MajorKind::UnsignedInt, *self as u64)?;
        }
        Ok(())
    }
}

impl Encode<DagCbor> for Cid {
    fn encode<W: Write>(&self, _: DagCbor, w: &mut W) -> Result<()> {
        write_tag(w, 42)?;
        // insert zero byte per https://github.com/ipld/specs/blob/master/block-layer/codecs/dag-cbor.md#links
        // TODO: don't allocate
        let buf = self.to_bytes();
        let len = buf.len();
        write_u64(w, MajorKind::ByteString, len as u64 + 1)?;
        w.write_all(&[0])?;
        w.write_all(&buf[..len])?;
        Ok(())
    }
}

impl<T: Encode<DagCbor>> Encode<DagCbor> for Option<T> {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        if let Some(value) = self {
            value.encode(c, w)?;
        } else {
            write_null(w)?;
        }
        Ok(())
    }
}

impl<T: Encode<DagCbor>> Encode<DagCbor> for Vec<T> {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        write_u64(w, MajorKind::Array, self.len() as u64)?;
        for value in self {
            value.encode(c, w)?;
        }
        Ok(())
    }
}

impl<T: Encode<DagCbor> + 'static> Encode<DagCbor> for BTreeMap<String, T> {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        write_u64(w, MajorKind::Map, self.len() as u64)?;
        // CBOR RFC-7049 specifies a canonical sort order, where keys are sorted by length first.
        // This was later revised with RFC-8949, but we need to stick to the original order to stay
        // compatible with existing data.
        let mut cbor_order = Vec::from_iter(self);
        cbor_order.sort_unstable_by(|&(key_a, _), &(key_b, _)| {
            match key_a.len().cmp(&key_b.len()) {
                Ordering::Greater => Ordering::Greater,
                Ordering::Less => Ordering::Less,
                Ordering::Equal => key_a.cmp(key_b),
            }
        });
        for (k, v) in cbor_order {
            k.encode(c, w)?;
            v.encode(c, w)?;
        }
        Ok(())
    }
}

impl Encode<DagCbor> for Ipld {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        match self {
            Self::Null => write_null(w),
            Self::Bool(b) => b.encode(c, w),
            Self::Integer(i) => i.encode(c, w),
            Self::Float(f) => f.encode(c, w),
            Self::Bytes(b) => b.as_slice().encode(c, w),
            Self::String(s) => s.encode(c, w),
            Self::List(l) => l.encode(c, w),
            Self::Map(m) => m.encode(c, w),
            Self::Link(cid) => cid.encode(c, w),
        }
    }
}

impl<T: Encode<DagCbor>> Encode<DagCbor> for Arc<T> {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        self.deref().encode(c, w)
    }
}

impl Encode<DagCbor> for () {
    fn encode<W: Write>(&self, _c: DagCbor, w: &mut W) -> Result<()> {
        write_u8(w, MajorKind::Array, 0)?;
        Ok(())
    }
}

impl<A: Encode<DagCbor>> Encode<DagCbor> for (A,) {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        write_u8(w, MajorKind::Array, 1)?;
        self.0.encode(c, w)?;
        Ok(())
    }
}

impl<A: Encode<DagCbor>, B: Encode<DagCbor>> Encode<DagCbor> for (A, B) {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        write_u8(w, MajorKind::Array, 2)?;
        self.0.encode(c, w)?;
        self.1.encode(c, w)?;
        Ok(())
    }
}

impl<A: Encode<DagCbor>, B: Encode<DagCbor>, C: Encode<DagCbor>> Encode<DagCbor> for (A, B, C) {
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        write_u8(w, MajorKind::Array, 3)?;
        self.0.encode(c, w)?;
        self.1.encode(c, w)?;
        self.2.encode(c, w)?;
        Ok(())
    }
}

impl<A: Encode<DagCbor>, B: Encode<DagCbor>, C: Encode<DagCbor>, D: Encode<DagCbor>> Encode<DagCbor>
    for (A, B, C, D)
{
    fn encode<W: Write>(&self, c: DagCbor, w: &mut W) -> Result<()> {
        write_u8(w, MajorKind::Array, 4)?;
        self.0.encode(c, w)?;
        self.1.encode(c, w)?;
        self.2.encode(c, w)?;
        self.3.encode(c, w)?;
        Ok(())
    }
}
