//! # Feature flags
//!
//! This crate exposes a number of features. These can be enabled or disabled as shown
//! [in Cargo's documentation](https://doc.rust-lang.org/cargo/reference/features.html). Features
//! are _disabled_ by default unless otherwise noted.
//!
//! Reliance on a given feature is always indicated alongside the item definition.
//!
//! - `std` (_enabled by default, implicitly enables `alloc`_)
//!
//!   This enables a number of features that depend on the standard library.
//!
//! - `alloc` (_enabled by default via `std`_)
//!
//!   Enables a number of features that require the ability to dynamically allocate memory.
//!
//! - `macros`
//!
//!   Enables macros that provide compile-time verification of values and intuitive syntax.
//!
//! - `formatting` (_implicitly enables `std`_)
//!
//!   Enables formatting of most structs.
//!
//! - `parsing`
//!
//!   Enables parsing of most structs.
//!
//! - `local-offset` (_implicitly enables `std`_)
//!
//!   This feature enables a number of methods that allow obtaining the system's UTC offset.
//!
//! - `large-dates`
//!
//!   By default, only years within the ±9999 range (inclusive) are supported. If you need support
//!   for years outside this range, consider enabling this feature; the supported range will be
//!   increased to ±999,999.
//!
//!   Note that enabling this feature has some costs, as it means forgoing some optimizations.
//!   Ambiguities may be introduced when parsing that would not otherwise exist.
//!
//! - `serde`
//!
//!   Enables [`serde`](https://docs.rs/serde) support for all types.
//!
//! - `serde-human-readable` (_implicitly enables `serde`, `formatting`, and `parsing`_)
//!
//!   Allows `serde` representations to use a human-readable format. This is determined by the
//!   serializer, not the user. If this feature is not enabled or if the serializer requests a
//!   non-human-readable format, a format optimized for binary representation will be used.
//!
//!   Libraries should never enable this feature, as the decision of what format to use should be up
//!   to the user.
//!
//! - `rand` (_implicitly enables `rand08`, `rand09`, and `rand010`_)
//!
//!   Previously, this would enable support for `rand` 0.8. Since the release of `rand` 0.9, the
//!   feature has been split into `rand08` and `rand09` to allow support for both versions. `rand`
//!   0.10 has since been added. For backwards compatibility and simplicity, this feature enables
//!   support for _all_ series.
//!
//!   It is strongly recommended to enable the version you need directly, as enabling `rand` will
//!   needlessly pull in all versions.
//!
//! - `rand08`, `rand09`, `rand010`
//!
//!   Enables [`rand` 0.8](https://docs.rs/rand/0.8), [`rand` 0.9](https://docs.rs/rand/0.9), and
//!   [`rand` 0.10](https://docs.rs/rand/0.10) support for all types, respectively.
//!
//! - `quickcheck` (_implicitly enables `alloc`_)
//!
//!   Enables [quickcheck](https://docs.rs/quickcheck) support for all types.
//!
//! - `wasm-bindgen`
//!
//!   Enables [`wasm-bindgen`](https://github.com/rustwasm/wasm-bindgen) support for converting
//!   [JavaScript dates](https://rustwasm.github.io/wasm-bindgen/api/js_sys/struct.Date.html), as
//!   well as obtaining the UTC offset from JavaScript.

#![doc(html_playground_url = "https://play.rust-lang.org")]
#![cfg_attr(docsrs, feature(doc_cfg, doc_notable_trait))]
#![no_std]
#![doc(html_favicon_url = "https://avatars0.githubusercontent.com/u/55999857")]
#![doc(html_logo_url = "https://avatars0.githubusercontent.com/u/55999857")]
#![doc(test(attr(deny(warnings))))]

#[allow(unused_extern_crates)]
#[cfg(feature = "alloc")]
extern crate alloc;

#[cfg(feature = "std")]
extern crate std;

/// Deprecated module for units of time.
#[deprecated(note = "import from `time::unit` instead")]
pub mod convert {
    pub use time_core::unit::*;
}
mod date;
pub mod error;
pub mod ext;
#[cfg(any(feature = "formatting", feature = "parsing"))]
pub mod format_description;
#[cfg(feature = "formatting")]
pub mod formatting;
mod hint;
#[cfg(feature = "std")]
mod instant;
mod internal_macros;
mod interop;
pub mod iter;
#[cfg(feature = "macros")]
pub mod macros;
mod month;
mod num_fmt;
mod offset_date_time;
#[cfg(feature = "parsing")]
pub mod parsing;
mod plain_date_time;
#[cfg(feature = "quickcheck")]
mod quickcheck;
#[cfg(feature = "rand010")]
mod rand010;
#[cfg(feature = "rand08")]
mod rand08;
#[cfg(feature = "rand09")]
mod rand09;
#[cfg(feature = "serde")]
pub mod serde;
mod signed_duration;
mod sys;
#[cfg(test)]
mod tests;
mod time;
mod timestamp;
mod utc_date_time;
mod utc_offset;
pub mod util;
mod weekday;

pub use time_core::unit;

pub use crate::date::Date;
pub use crate::error::Error;
#[doc(hidden)]
#[cfg(feature = "std")]
#[expect(deprecated)]
pub use crate::instant::Instant;
pub use crate::month::Month;
pub use crate::offset_date_time::OffsetDateTime;
pub use crate::plain_date_time::PlainDateTime;
pub use crate::signed_duration::SignedDuration;
pub use crate::time::Time;
pub use crate::timestamp::Timestamp;
pub use crate::utc_date_time::UtcDateTime;
pub use crate::utc_offset::UtcOffset;
pub use crate::weekday::Weekday;

/// An alias for [`std::result::Result`] with a generic error from the time crate.
pub type Result<T> = core::result::Result<T, Error>;
/// A [`PlainDateTime`] under its original name.
///
/// This type is not currently deprecated, but it likely will be in the future. Use
/// [`PlainDateTime`] when writing new code. In the next breaking release, this alias will be
/// removed.
pub type PrimitiveDateTime = PlainDateTime;
/// A [`SignedDuration`] under its original name.
///
/// This type is not currently deprecated, but it likely will be in the future. Use
/// [`SignedDuration`] when writing new code. In the next breaking release, this alias will be
/// removed.
pub type Duration = SignedDuration;

/// A private type used to restrict the ability to call methods that are not intended to be called
/// by downstream users.
#[cfg(any(feature = "formatting", feature = "parsing"))]
#[derive(Debug)]
struct PrivateMethod;
