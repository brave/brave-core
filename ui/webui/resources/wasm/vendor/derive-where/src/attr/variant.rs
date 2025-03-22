//! Attribute parsing for variants.

use syn::{spanned::Spanned, Attribute, Fields, Meta, Result, Variant};

use crate::{util::MetaListExt, Default, DeriveWhere, Error, Incomparable, Skip, DERIVE_WHERE};

/// Attributes on variant.
#[derive(Default)]
pub struct VariantAttr {
	/// Default variant.
	pub default: Default,
	/// [`Trait`](crate::Trait)s to skip all fields for.
	pub skip_inner: Skip,
	/// Comparing variant will yield `false` for [`PartialEq`] and [`None`] for
	/// [`PartialOrd`].
	pub incomparable: Incomparable,
}

impl VariantAttr {
	/// Create [`VariantAttr`] from [`Attribute`]s.
	pub fn from_attrs(
		attrs: &[Attribute],
		derive_wheres: &[DeriveWhere],
		variant: &Variant,
	) -> Result<Self> {
		let mut self_ = VariantAttr::default();

		for attr in attrs {
			if attr.path().is_ident(DERIVE_WHERE) {
				self_.add_meta(&attr.meta, derive_wheres, variant)?
			}
		}

		Ok(self_)
	}

	/// Add [`Meta`] to [`VariantAttr`].
	fn add_meta(
		&mut self,
		meta: &Meta,
		derive_wheres: &[DeriveWhere],
		variant: &Variant,
	) -> Result<()> {
		debug_assert!(meta.path().is_ident(DERIVE_WHERE));

		if let Meta::List(list) = meta {
			let nested = list.parse_non_empty_nested_metas()?;

			if nested.is_empty() {
				return Err(Error::empty(list.span()));
			}

			for meta in &nested {
				if meta.path().is_ident(Skip::SKIP_INNER) {
					// Don't allow `skip_inner` on empty variants.
					match &variant.fields {
						Fields::Named(fields) if fields.named.is_empty() => {
							return Err(Error::option_skip_empty(variant.span()))
						}
						Fields::Unnamed(fields) if fields.unnamed.is_empty() => {
							return Err(Error::option_skip_empty(variant.span()))
						}
						Fields::Unit => return Err(Error::option_skip_empty(variant.span())),
						_ => self.skip_inner.add_attribute(derive_wheres, None, meta)?,
					}
				} else if meta.path().is_ident(Default::DEFAULT) {
					self.default.add_attribute(meta, derive_wheres)?;
				} else if meta.path().is_ident(Incomparable::INCOMPARABLE) {
					self.incomparable.add_attribute(meta, derive_wheres)?;
				} else {
					return Err(Error::option(meta.path().span()));
				}
			}

			Ok(())
		} else {
			Err(Error::option_syntax(meta.span()))
		}
	}
}
