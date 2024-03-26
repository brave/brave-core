//! CBOR error types.
use std::any::type_name;
use thiserror::Error;

/// Number larger than u64.
#[derive(Debug, Error)]
#[error("Number larger than {ty}.")]
pub struct NumberOutOfRange {
    /// Type.
    pub ty: &'static str,
}

impl NumberOutOfRange {
    /// Creates a new `NumberOutOfRange` error.
    pub fn new<T>() -> Self {
        Self {
            ty: type_name::<T>(),
        }
    }
}

/// Number is not minimally encoded.
#[derive(Debug, Error)]
#[error("Number not minimally encoded.")]
pub struct NumberNotMinimal;

/// Length larger than usize or too small, for example zero length cid field.
#[derive(Debug, Error)]
#[error("Length out of range when decoding {ty}.")]
pub struct LengthOutOfRange {
    /// Type.
    pub ty: &'static str,
}

impl LengthOutOfRange {
    /// Creates a new `LengthOutOfRange` error.
    pub fn new<T>() -> Self {
        Self {
            ty: type_name::<T>(),
        }
    }
}

/// Unexpected cbor code.
#[derive(Debug, Error)]
#[error("Unexpected cbor code `0x{code:x}` when decoding `{ty}`.")]
pub struct UnexpectedCode {
    /// Code.
    pub code: u8,
    /// Type.
    pub ty: &'static str,
}

impl UnexpectedCode {
    /// Creates a new `UnexpectedCode` error.
    pub fn new<T>(code: u8) -> Self {
        Self {
            code,
            ty: type_name::<T>(),
        }
    }
}

/// Unexpected key.
#[derive(Debug, Error)]
#[error("Unexpected key `{key}` when decoding `{ty}`.")]
pub struct UnexpectedKey {
    /// Key.
    pub key: String,
    /// Type.
    pub ty: &'static str,
}

impl UnexpectedKey {
    /// Creates a new `UnexpectedKey` error.
    pub fn new<T>(key: String) -> Self {
        Self {
            key,
            ty: type_name::<T>(),
        }
    }
}

/// Missing key.
#[derive(Debug, Error)]
#[error("Missing key `{key}` for decoding `{ty}`.")]
pub struct MissingKey {
    /// Key.
    pub key: &'static str,
    /// Type.
    pub ty: &'static str,
}

impl MissingKey {
    /// Creates a new `MissingKey` error.
    pub fn new<T>(key: &'static str) -> Self {
        Self {
            key,
            ty: type_name::<T>(),
        }
    }
}

/// Unknown cbor tag.
#[derive(Debug, Error)]
#[error("Unknown cbor tag `{0}`.")]
pub struct UnknownTag(pub u64);

/// Unexpected eof.
#[derive(Debug, Error)]
#[error("Unexpected end of file.")]
pub struct UnexpectedEof;

/// The byte before Cid was not multibase identity prefix.
#[derive(Debug, Error)]
#[error("Invalid Cid prefix: {0}")]
pub struct InvalidCidPrefix(pub u8);

/// A duplicate key within a map.
#[derive(Debug, Error)]
#[error("Duplicate map key.")]
pub struct DuplicateKey;
