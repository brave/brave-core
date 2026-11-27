use std::{ops::Range, str::FromStr};

#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub enum Type {
    BitField(Range<u8>),
    Bool,
    /// A single Unicode character
    Char,

    Debug,
    Display,
    FormatSequence,

    F32,
    F64,

    /// `{=?}` OR `{}`
    #[default] // when not specified in the format string, this type is assumed
    Format,
    FormatArray(usize), // FIXME: This `usize` is not the target's `usize`; use `u64` instead?
    /// `{=[?]}`
    FormatSlice,

    I8,
    I16,
    I32,
    I64,
    I128,
    Isize,

    /// Interned string index.
    IStr,
    /// String slice (i.e. passed directly; not as interned string indices).
    Str,

    U8,
    U16,
    U32,
    U64,
    U128,
    Usize,

    /// Byte slice `{=[u8]}`.
    U8Slice,
    U8Array(usize), // FIXME: This `usize` is not the target's `usize`; use `u64` instead?
}

// FIXME: either all or none of the type parsing should be done in here
impl FromStr for Type {
    type Err = ();

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Ok(match s {
            "u8" => Type::U8,
            "u16" => Type::U16,
            "u32" => Type::U32,
            "u64" => Type::U64,
            "u128" => Type::U128,
            "usize" => Type::Usize,
            "i8" => Type::I8,
            "i16" => Type::I16,
            "i32" => Type::I32,
            "i64" => Type::I64,
            "i128" => Type::I128,
            "isize" => Type::Isize,
            "f32" => Type::F32,
            "f64" => Type::F64,
            "bool" => Type::Bool,
            "str" => Type::Str,
            "istr" => Type::IStr,
            "__internal_Debug" => Type::Debug,
            "__internal_Display" => Type::Display,
            "__internal_FormatSequence" => Type::FormatSequence,
            "[u8]" => Type::U8Slice,
            "?" => Type::Format,
            "[?]" => Type::FormatSlice,
            "char" => Type::Char,
            _ => return Err(()),
        })
    }
}
