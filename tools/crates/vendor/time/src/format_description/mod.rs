//! Description of how types should be formatted and parsed.
//!
//! The formatted value will be output to the provided writer. Format descriptions can be
//! [well-known](crate::format_description::well_known) or obtained by using the
//! [`format_description!`](crate::macros::format_description) macro or a function listed below.
//!
//! For examples, see the implementors of [Formattable](crate::formatting::Formattable),
//! e.g. [`well_known::Rfc3339`].

mod borrowed_format_item;
mod component;
pub(crate) mod format_description_v3;
pub mod modifier;
#[cfg(feature = "alloc")]
mod owned_format_item;
#[cfg(feature = "alloc")]
mod parse;

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

/// Re-exports of internal types for use in macros.
///
/// Do not rely on the existence of this module for any reason. It, in its entirety, is not
/// considered public API, is not subject to semantic versioning, and may be changed or removed at
/// any point.
#[doc(hidden)]
pub mod __private {
    pub use super::format_description_v3::{Component, FormatDescriptionV3Inner};
}

pub use borrowed_format_item::BorrowedFormatItem;
#[doc(hidden)]
#[deprecated(since = "0.3.37", note = "use `BorrowedFormatItem` for clarity")]
pub use borrowed_format_item::BorrowedFormatItem as FormatItem;
#[cfg(feature = "alloc")]
pub use owned_format_item::OwnedFormatItem;

pub use self::component::Component;
pub use self::format_description_v3::FormatDescriptionV3;
#[cfg(feature = "alloc")]
#[expect(deprecated, reason = "backwards compatibility")]
pub use self::parse::parse;
#[cfg(feature = "alloc")]
pub use self::parse::{parse_borrowed, parse_owned, parse_strftime_borrowed, parse_strftime_owned};

/// The type output by the [`format_description!`](crate::macros::format_description) macro.
pub type StaticFormatDescription = &'static [BorrowedFormatItem<'static>];

/// Indicate whether the hour is "am" or "pm".
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum Period {
    #[allow(clippy::missing_docs_in_private_items)]
    Am,
    #[allow(clippy::missing_docs_in_private_items)]
    Pm,
}
