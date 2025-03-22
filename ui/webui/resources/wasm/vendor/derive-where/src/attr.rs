//! [`Attribute`](syn::Attribute) parsing for items, variants and fields.

mod default;
mod field;
mod incomparable;
mod item;
mod skip;
mod variant;
#[cfg(feature = "zeroize")]
mod zeroize_fqs;

#[cfg(feature = "zeroize")]
pub use self::zeroize_fqs::ZeroizeFqs;
pub use self::{
	default::Default,
	field::FieldAttr,
	incomparable::Incomparable,
	item::{DeriveTrait, DeriveWhere, ItemAttr},
	skip::{Skip, SkipGroup},
	variant::VariantAttr,
};
