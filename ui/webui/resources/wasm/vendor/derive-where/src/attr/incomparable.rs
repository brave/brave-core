//! Attribute parsing for the `incomparable` option.

use proc_macro2::Span;
use syn::{spanned::Spanned, Meta, Result};

use crate::{attr::DeriveTrait, DeriveWhere, Error};

/// Stores if this variant should be incomparable when implementing
/// [`PartialEq`] or [`PartialOrd`].
#[derive(Clone, Copy, Default)]
#[cfg_attr(test, derive(Debug))]
pub struct Incomparable(pub Option<Span>);

impl Incomparable {
	/// Token used for the `incomparable` option.
	pub const INCOMPARABLE: &'static str = "incomparable";

	/// Adds a [`Meta`] to this [`Incomparable`].
	pub fn add_attribute(&mut self, meta: &Meta, derive_wheres: &[DeriveWhere]) -> Result<()> {
		debug_assert!(meta.path().is_ident(Self::INCOMPARABLE));

		if let Meta::Path(path) = meta {
			if self.0.is_some() {
				Err(Error::option_duplicate(path.span(), Self::INCOMPARABLE))
			} else {
				let mut impl_cmp = false;

				for trait_ in derive_wheres
					.iter()
					.flat_map(|derive_where| derive_where.traits.iter())
				{
					match trait_ {
						DeriveTrait::Eq | DeriveTrait::Ord => {
							return Err(Error::non_partial_incomparable(path.span()));
						}
						DeriveTrait::PartialEq | DeriveTrait::PartialOrd => impl_cmp = true,
						_ => {}
					}
				}

				if impl_cmp {
					self.0 = Some(path.span());
					Ok(())
				} else {
					Err(Error::incomparable(path.span()))
				}
			}
		} else {
			Err(Error::option_syntax(meta.span()))
		}
	}
}
