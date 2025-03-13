//! Attribute parsing for the `Zeroize(fqs)` option.

use syn::{spanned::Spanned, Meta, Result};

use crate::{util::MetaListExt, DeriveWhere, Error, Trait, TraitImpl};

/// Stores if this field should use FQS to call [`Zeroize::zeroize`](https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html#tymethod.zeroize).
#[derive(Default)]
#[cfg_attr(test, derive(Debug))]
pub struct ZeroizeFqs(pub bool);

impl ZeroizeFqs {
	/// Token used for the `Zeroize(fqs)` option.
	const FQS: &'static str = "fqs";

	/// Adds a [`Meta`] to this [`ZeroizeFqs`].
	pub fn add_attribute(&mut self, meta: &Meta, derive_wheres: &[DeriveWhere]) -> Result<()> {
		debug_assert!(meta.path().is_ident(Trait::Zeroize.as_str()));

		if !derive_wheres
			.iter()
			.any(|derive_where| derive_where.contains(Trait::Zeroize))
		{
			return Err(Error::zeroize(meta.span()));
		}

		match meta {
			Meta::List(list) => {
				let nested = list.parse_non_empty_nested_metas()?;

				for meta in &nested {
					if let Meta::Path(path) = meta {
						if path.is_ident(Self::FQS) {
							if self.0 {
								return Err(Error::option_duplicate(path.span(), Self::FQS));
							} else {
								self.0 = true
							}
						} else {
							return Err(Error::option(path.span()));
						}
					} else {
						return Err(Error::option_syntax(meta.span()));
					}
				}

				Ok(())
			}
			Meta::Path(path) => Err(Error::option_required(path.span(), Trait::Zeroize.as_str())),
			_ => Err(Error::option_syntax(meta.span())),
		}
	}
}
