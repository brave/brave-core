#![recursion_limit = "256"]
#![cfg(all(feature = "parse", feature = "display"))]

mod de_errors;
mod display;
mod display_tricky;
mod enum_external_deserialize;
mod float;
mod formatting;
mod macros;
mod pretty;
mod serde;
mod spanned;
mod spanned_impls;
mod tables_last;
