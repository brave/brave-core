//! Description of how types should be formatted and parsed.
//!
//! The formatted value will be output to the provided writer. Format descriptions can be
//! [well-known](crate::format_description::well_known) or obtained by using the
//! [`format_description!`](crate::macros::format_description) macro or a function listed below.

mod borrowed_format_item;
mod component;
pub mod modifier;
#[cfg(feature = "alloc")]
mod owned_format_item;
#[cfg(feature = "alloc")]
mod parse;

pub use borrowed_format_item::BorrowedFormatItem as FormatItem;
#[cfg(feature = "alloc")]
pub use owned_format_item::OwnedFormatItem;

pub use self::component::Component;
#[cfg(feature = "alloc")]
pub use self::parse::{parse, parse_borrowed, parse_owned};

/// Well-known formats, typically standards.
pub mod well_known {
    pub mod iso8601;
    mod rfc2822;
    mod rfc3339;

    #[doc(inline)]
    pub use iso8601::Iso8601;
    pub use rfc2822::Rfc2822;
    pub use rfc3339::Rfc3339;
}
