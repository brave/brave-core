//! CBOR helper types for encoding and decoding.
use std::convert::TryFrom;

use crate::error::UnexpectedCode;
use libipld_core::ipld::Ipld;

/// Represents a major "byte". This includes both the major bits and the additional info.
#[repr(transparent)]
#[derive(Clone, Copy, Eq, PartialEq)]
pub struct Major(u8);

/// The constant TRUE.
pub const FALSE: Major = Major::new(MajorKind::Other, 20);
/// The constant FALSE.
pub const TRUE: Major = Major::new(MajorKind::Other, 21);
/// The constant NULL.
pub const NULL: Major = Major::new(MajorKind::Other, 22);
/// The major "byte" indicating that a 16 bit float follows.
pub const F16: Major = Major::new(MajorKind::Other, 25);
/// The major "byte" indicating that a 32 bit float follows.
pub const F32: Major = Major::new(MajorKind::Other, 26);
/// The major "byte" indicating that a 64 bit float follows.
pub const F64: Major = Major::new(MajorKind::Other, 27);

impl Major {
    const fn new(kind: MajorKind, info: u8) -> Self {
        Major(((kind as u8) << 5) | info)
    }

    /// Returns the major type.
    #[inline(always)]
    pub const fn kind(self) -> MajorKind {
        // This is a 3 bit value, so value 0-7 are covered.
        unsafe { std::mem::transmute(self.0 >> 5) }
    }

    /// Returns the additional info.
    #[inline(always)]
    pub const fn info(self) -> u8 {
        self.0 & 0x1f
    }

    /// Interprets the additioanl info as a number of additional bytes that should be consumed.
    #[inline(always)]
    #[allow(clippy::len_without_is_empty)]
    pub const fn len(self) -> u8 {
        // All major types follow the same rules for "additioanl bytes".
        // 24 -> 1, 25 -> 2, 26 -> 4, 27 -> 8
        match self.info() {
            info @ 24..=27 => 1 << (info - 24),
            _ => 0,
        }
    }
}

impl From<Major> for u8 {
    fn from(m: Major) -> u8 {
        m.0
    }
}

// This is the core of the validation logic. Every major type passes through here giving us a chance
// to determine if it's something we allow.
impl TryFrom<u8> for Major {
    type Error = UnexpectedCode;
    fn try_from(value: u8) -> Result<Self, Self::Error> {
        // We don't allow any major types with additional info 28-31 inclusive.
        // Or the bitmask 0b00011100 = 28.
        if value & 28 == 28 {
            return Err(UnexpectedCode::new::<Ipld>(value));
        } else if (value >> 5) == MajorKind::Other as u8 {
            match value & 0x1f {
                // False, True, Null. TODO: Allow undefined?
                20 | 21 | 22 => (),
                // Floats. TODO: forbid f16 & f32?
                25 | 26 | 27 => (),
                // Everything is forbidden.
                _ => {
                    return Err(UnexpectedCode::new::<Ipld>(value));
                }
            }
        }
        Ok(Major(value))
    }
}

/// The type
#[repr(u8)]
#[derive(Clone, Copy, Eq, PartialEq)]
#[allow(dead_code)]
pub enum MajorKind {
    /// Non-negative integer (major type 0).
    UnsignedInt = 0,
    /// Negative integer (major type 1).
    NegativeInt = 1,
    /// Byte string (major type 2).
    ByteString = 2,
    /// Unicode text string (major type 3).
    TextString = 3,
    /// Array (major type 4).
    Array = 4,
    /// Map (major type 5).
    Map = 5,
    /// Tag (major type 6).
    Tag = 6,
    /// Other (major type 7).
    Other = 7,
}
