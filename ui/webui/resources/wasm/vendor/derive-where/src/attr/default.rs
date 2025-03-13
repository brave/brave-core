//! Attribute parsing for the `default` option.

use proc_macro2::Span;
use syn::{spanned::Spanned, Meta, Result};

use crate::{DeriveWhere, Error, Trait};

/// Stores if this variant should be the default when implementing
/// [`Default`](trait@std::default::Default).
#[derive(Clone, Copy, Default)]
#[cfg_attr(test, derive(Debug))]
pub struct Default(pub Option<Span>);

impl Default {
	/// Token used for the `default` option.
	pub const DEFAULT: &'static str = "default";

	/// Adds a [`Meta`] to this [`Default`](Self).
	pub fn add_attribute(&mut self, meta: &Meta, derive_wheres: &[DeriveWhere]) -> Result<()> {
		debug_assert!(meta.path().is_ident(Self::DEFAULT));

		if let Meta::Path(path) = meta {
			if self.0.is_some() {
				Err(Error::option_duplicate(path.span(), Self::DEFAULT))
			} else {
				let mut impl_default = false;

				for derive_where in derive_wheres {
					if derive_where.contains(Trait::Default) {
						impl_default = true;
						break;
					}
				}

				if impl_default {
					self.0 = Some(path.span());
					Ok(())
				} else {
					Err(Error::default(path.span()))
				}
			}
		} else {
			Err(Error::option_syntax(meta.span()))
		}
	}
}
