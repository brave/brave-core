//! Field parsing.

use std::fmt::{self, Display, Formatter};

use proc_macro2::{Span, TokenStream};
use quote::{format_ident, IdentFragment, ToTokens};
use syn::{ext::IdentExt, Attribute, FieldsNamed, FieldsUnnamed, Ident, Index, Result, Type};

use crate::{DeriveWhere, FieldAttr, Skip, Trait};

/// Struct, union, struct variant or tuple variant field.
#[cfg_attr(test, derive(Debug))]
pub struct Field<'a> {
	/// Attributes.
	pub attr: FieldAttr,
	/// [`struct@Ident`] or [`Index`] for this field.
	pub member: Member<'a>,
	/// [`struct@Ident`] used as a Temporary variable for destructuring `self`.
	pub self_ident: Ident,
	/// [`struct@Ident`] used as a Temporary variable for destructuring `other`.
	pub other_ident: Ident,
	/// [`Type`] used for asserting traits on fields for [`Eq`].
	pub type_: &'a Type,
}

/// Borrowed version of [`syn::Member`], to avoid unnecessary allocations.
#[cfg_attr(test, derive(Debug))]
pub enum Member<'a> {
	/// Named field.
	Named(&'a Ident),
	/// Unnamed field.
	Unnamed(Index),
}

impl IdentFragment for Member<'_> {
	fn fmt(&self, f: &mut Formatter) -> fmt::Result {
		Display::fmt(&self, f)
	}
}

impl ToTokens for Member<'_> {
	fn to_tokens(&self, tokens: &mut TokenStream) {
		match self {
			Member::Named(ident) => ident.to_tokens(tokens),
			Member::Unnamed(index) => index.to_tokens(tokens),
		}
	}
}

impl Display for Member<'_> {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		match self {
			Member::Named(ident) => write!(f, "{}", ident.unraw()),
			Member::Unnamed(index) => write!(f, "{}", index.index),
		}
	}
}

impl<'a> Field<'a> {
	/// Create [`Field`]s from [`syn::FieldsNamed`].
	pub fn from_named(
		derive_wheres: &[DeriveWhere],
		skip_inner: &Skip,
		fields: &'a FieldsNamed,
	) -> Result<Vec<Self>> {
		fields
			.named
			.iter()
			.map(|field| {
				Field::from_field(
					derive_wheres,
					skip_inner,
					&field.attrs,
					Member::Named(field.ident.as_ref().expect("unexpected unnamed field")),
					&field.ty,
				)
			})
			.collect()
	}

	/// Create [`Field`]s from [`syn::FieldsUnnamed`].
	pub fn from_unnamed(
		derive_wheres: &[DeriveWhere],
		skip_inner: &Skip,
		fields: &'a FieldsUnnamed,
	) -> Result<Vec<Self>> {
		(0_u32..)
			.zip(&fields.unnamed)
			.map(|(index, field)| {
				Field::from_field(
					derive_wheres,
					skip_inner,
					&field.attrs,
					Member::Unnamed(Index {
						index,
						span: Span::call_site(),
					}),
					&field.ty,
				)
			})
			.collect()
	}

	/// Create [`Field`] from [`syn::Field`].
	fn from_field(
		derive_wheres: &[DeriveWhere],
		skip_inner: &Skip,
		attrs: &[Attribute],
		member: Member<'a>,
		type_: &'a Type,
	) -> Result<Self> {
		let attr = FieldAttr::from_attrs(derive_wheres, skip_inner, attrs)?;
		let self_ident = format_ident!("__field_{}", member);
		let other_ident = format_ident!("__other_field_{}", member);

		Ok(Self {
			attr,
			member,
			self_ident,
			other_ident,
			type_,
		})
	}

	/// Convert to [`syn::Member`].
	pub fn to_member(&self) -> syn::Member {
		match self.member {
			Member::Named(ident) => syn::Member::Named(ident.clone()),
			Member::Unnamed(ref index) => syn::Member::Unnamed(index.clone()),
		}
	}

	/// Returns `true` if this field is skipped with the given [`Trait`].
	pub fn skip(&self, trait_: Trait) -> bool {
		self.attr.skip.trait_skipped(trait_)
	}
}
