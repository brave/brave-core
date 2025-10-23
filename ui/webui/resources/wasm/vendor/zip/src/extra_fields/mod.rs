//! types for extra fields

/// marker trait to denote the place where this extra field has been stored
pub trait ExtraFieldVersion {}

/// use this to mark extra fields specified in a local header

#[derive(Debug, Clone)]
pub struct LocalHeaderVersion;

/// use this to mark extra fields specified in the central header

#[derive(Debug, Clone)]
pub struct CentralHeaderVersion;

impl ExtraFieldVersion for LocalHeaderVersion {}
impl ExtraFieldVersion for CentralHeaderVersion {}

mod extended_timestamp;

pub use extended_timestamp::*;

/// contains one extra field
#[derive(Debug, Clone)]
pub enum ExtraField {
    /// extended timestamp, as described in <https://libzip.org/specifications/extrafld.txt>
    ExtendedTimestamp(ExtendedTimestamp),
}
