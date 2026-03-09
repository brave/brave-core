//! Attribute parsing for fields.

use syn::{spanned::Spanned, Attribute, Meta, Result};

use crate::{util::MetaListExt, DeriveWhere, Error, Skip, DERIVE_WHERE};
#[cfg(feature = "zeroize")]
use crate::{Trait, TraitImpl, ZeroizeFqs};

/// Attributes on field.
#[derive(Default)]
#[cfg_attr(test, derive(Debug))]
pub struct FieldAttr {
	/// [`Trait`](crate::Trait)s to skip this field for.
	#[cfg_attr(feature = "zeroize", allow(rustdoc::redundant_explicit_links))]
	pub skip: Skip,
	/// Use fully-qualified-syntax for the [`Zeroize`](https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html) implementation on this field.
	#[cfg(feature = "zeroize")]
	pub zeroize_fqs: ZeroizeFqs,
}

impl FieldAttr {
	/// Create [`FieldAttr`] from [`Attribute`]s.
	pub fn from_attrs(
		derive_wheres: &[DeriveWhere],
		skip_inner: &Skip,
		attrs: &[Attribute],
	) -> Result<Self> {
		let mut self_ = FieldAttr::default();

		for attr in attrs {
			if attr.path().is_ident(DERIVE_WHERE) {
				self_.add_meta(derive_wheres, skip_inner, &attr.meta)?
			}
		}

		Ok(self_)
	}

	/// Add [`Meta`] to [`FieldAttr`].
	fn add_meta(
		&mut self,
		derive_wheres: &[DeriveWhere],
		skip_inner: &Skip,
		meta: &Meta,
	) -> Result<()> {
		debug_assert!(meta.path().is_ident(DERIVE_WHERE));

		if let Meta::List(list) = meta {
			let nested = list.parse_non_empty_nested_metas()?;

			for meta in &nested {
				if meta.path().is_ident(Skip::SKIP) {
					self.skip
						.add_attribute(derive_wheres, Some(skip_inner), meta)?;
					continue;
				}

				#[cfg(feature = "zeroize")]
				{
					if meta.path().is_ident(Trait::Zeroize.as_str()) {
						self.zeroize_fqs.add_attribute(meta, derive_wheres)?;
						continue;
					}
				}

				return Err(Error::option(meta.path().span()));
			}

			Ok(())
		} else {
			Err(Error::option_syntax(meta.span()))
		}
	}
}
