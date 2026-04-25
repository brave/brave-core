//! Types holding data of items.

mod field;
mod fields;

use proc_macro2::Span;
use syn::{Expr, FieldsNamed, Ident, Pat, PatPath, Path, Result, Variant};

pub use self::{
	field::{Field, Member},
	fields::Fields,
};
use crate::{util, Default, DeriveWhere, Either, Error, Incomparable, Skip, Trait, VariantAttr};

/// Holds all relevant data of a struct, union or variant.
#[cfg_attr(test, derive(Debug))]
pub struct Data<'a> {
	/// [`Skip`] attribute of this struct, union or variant.
	skip_inner: Skip,
	/// [`Incomparable`] attribute of this struct, union or variant.
	pub incomparable: Incomparable,
	/// [`struct@Ident`] of this struct, union or variant, used for implementing
	/// [`Debug`](std::fmt::Debug).
	pub ident: &'a Ident,
	/// [`Path`] of this struct, union or variant, used to construct new
	/// instances of that item, for example when implementing [`Clone`].
	pub path: Path,
	/// [Type](DataType) of this struct, union or variant.
	pub type_: DataType<'a>,
	/// Discriminant of this variant.
	pub discriminant: Option<&'a Expr>,
}

/// Type of this data.
#[cfg_attr(test, derive(Debug))]
pub enum DataType<'a> {
	/// Struct.
	Struct(Fields<'a>),
	/// Tuple.
	Tuple(Fields<'a>),
	/// Union.
	Union(Fields<'a>),
	/// Variant.
	Variant {
		/// [`struct@Default`] attribute of this variant.
		default: Default,
		/// [Type](VariantType) of this variant.
		type_: VariantType<'a>,
	},
	/// Unit.
	Unit(Pat),
}

/// Type of [`Data`].
#[cfg_attr(test, derive(Debug))]
pub enum VariantType<'a> {
	/// Struct variant.
	Struct(Fields<'a>),
	/// Tuple variant.
	Tuple(Fields<'a>),
	/// Unit variant.
	Unit(Pat),
}

/// Type to enable simplified matching.
pub enum SimpleType<'a> {
	/// Struct, struct variant.
	Struct(&'a Fields<'a>),
	/// Tuple struct or tuple variant.
	Tuple(&'a Fields<'a>),
	/// Union.
	Union(&'a Fields<'a>),
	/// Unit variant.
	Unit(&'a Pat),
}

impl<'a> Data<'a> {
	/// Create [`Data`]s from [`syn::Fields`] of a struct.
	pub fn from_struct(
		span: Span,
		derive_wheres: &[DeriveWhere],
		skip_inner: Skip,
		incomparable: Incomparable,
		ident: &'a Ident,
		fields: &'a syn::Fields,
	) -> Result<Self> {
		let path = util::path_from_idents(&[ident]);

		match fields {
			syn::Fields::Named(fields) => {
				if fields.named.is_empty() && incomparable.0.is_none() {
					Err(Error::item_empty(span))
				} else {
					let fields =
						Fields::from_named(derive_wheres, &skip_inner, path.clone(), fields)?;

					Ok(Self {
						skip_inner,
						incomparable,
						ident,
						path,
						type_: DataType::Struct(fields),
						discriminant: None,
					})
				}
			}
			syn::Fields::Unnamed(fields) => {
				if fields.unnamed.is_empty() && incomparable.0.is_none() {
					Err(Error::item_empty(span))
				} else {
					let fields =
						Fields::from_unnamed(derive_wheres, &skip_inner, path.clone(), fields)?;

					Ok(Self {
						skip_inner,
						incomparable,
						ident,
						path,
						type_: DataType::Tuple(fields),
						discriminant: None,
					})
				}
			}
			syn::Fields::Unit if incomparable.0.is_some() => Ok(Self {
				skip_inner,
				incomparable,
				ident,
				path: path.clone(),
				type_: DataType::Unit(Pat::Path(PatPath {
					attrs: Vec::new(),
					qself: None,
					path,
				})),
				discriminant: None,
			}),
			syn::Fields::Unit => Err(Error::item_empty(span)),
		}
	}

	/// Create [`Data`]s from [`FieldsNamed`] of an union.
	pub fn from_union(
		span: Span,
		derive_wheres: &[DeriveWhere],
		skip_inner: Skip,
		incomparable: Incomparable,
		ident: &'a Ident,
		fields: &'a FieldsNamed,
	) -> Result<Self> {
		if fields.named.is_empty() && incomparable.0.is_none() {
			Err(Error::item_empty(span))
		} else {
			let path = util::path_from_idents(&[ident]);
			let fields = Fields::from_named(derive_wheres, &skip_inner, path.clone(), fields)?;

			Ok(Self {
				skip_inner,
				incomparable,
				ident,
				path,
				type_: DataType::Union(fields),
				discriminant: None,
			})
		}
	}

	/// Create [`Data`]s from [`syn::Fields`] of a variant.
	pub fn from_variant(
		item_ident: &'a Ident,
		derive_wheres: &[DeriveWhere],
		variant: &'a Variant,
	) -> Result<Self> {
		// Parse `Attribute`s on variant.
		let VariantAttr {
			default,
			skip_inner,
			incomparable,
		} = VariantAttr::from_attrs(&variant.attrs, derive_wheres, variant)?;

		let path = util::path_from_idents(&[item_ident, &variant.ident]);

		match &variant.fields {
			syn::Fields::Named(fields) => {
				let fields = Fields::from_named(derive_wheres, &skip_inner, path.clone(), fields)?;

				Ok(Self {
					skip_inner,
					incomparable,
					ident: &variant.ident,
					path,
					type_: DataType::Variant {
						default,
						type_: VariantType::Struct(fields),
					},
					discriminant: variant.discriminant.as_ref().map(|(_, expr)| expr),
				})
			}
			syn::Fields::Unnamed(fields) => {
				let fields =
					Fields::from_unnamed(derive_wheres, &skip_inner, path.clone(), fields)?;

				Ok(Self {
					skip_inner,
					incomparable,
					ident: &variant.ident,
					path,
					type_: DataType::Variant {
						default,
						type_: VariantType::Tuple(fields),
					},
					discriminant: variant.discriminant.as_ref().map(|(_, expr)| expr),
				})
			}
			syn::Fields::Unit => {
				let pattern = Pat::Path(PatPath {
					attrs: Vec::new(),
					qself: None,
					path: path.clone(),
				});

				Ok(Self {
					skip_inner,
					incomparable,
					ident: &variant.ident,
					path,
					type_: DataType::Variant {
						default,
						type_: VariantType::Unit(pattern),
					},
					discriminant: variant.discriminant.as_ref().map(|(_, expr)| expr),
				})
			}
		}
	}

	/// Returns the [`Fields`] of this [`Data`]. If [`Data`] is a unit variant
	/// or struct returns [`Pat`] instead.
	pub fn fields(&self) -> Either<&Fields, &Pat> {
		match &self.type_ {
			DataType::Struct(fields)
			| DataType::Tuple(fields)
			| DataType::Union(fields)
			| DataType::Variant {
				type_: VariantType::Struct(fields),
				..
			}
			| DataType::Variant {
				type_: VariantType::Tuple(fields),
				..
			} => Either::Left(fields),
			DataType::Unit(pattern)
			| DataType::Variant {
				type_: VariantType::Unit(pattern),
				..
			} => Either::Right(pattern),
		}
	}

	/// Returns the destructuring `self` pattern of this [`Data`].
	pub fn self_pattern(&self) -> &Pat {
		match self.fields() {
			Either::Left(fields) => &fields.self_pattern,
			Either::Right(pattern) => pattern,
		}
	}

	/// Returns `true` if this variant is marked as the [`struct@Default`]. If
	/// not a variant, always returns `true`.
	pub fn is_default(&self) -> bool {
		match self.type_ {
			DataType::Variant { default, .. } => default.0.is_some(),
			_ => true,
		}
	}

	/// Returns `true` if this item or variant is marked as [`Incomparable`].
	pub fn is_incomparable(&self) -> bool {
		self.incomparable.0.is_some()
	}

	/// Returns [`Some`] if this variant has a [`struct@Default`]. If
	/// not a variant, always returns [`None`].
	pub fn default_span(&self) -> Option<Span> {
		match &self.type_ {
			DataType::Variant { default, .. } => default.0,
			_ => None,
		}
	}

	/// Returns `true` if this [`Data`] has no [`Fields`].
	pub fn is_empty(&self, trait_: Trait) -> bool {
		self.iter_fields(trait_).count() == 0
	}

	/// Returns `true` if a field is skipped with that [`Trait`].
	pub fn any_skip_trait(&self, trait_: Trait) -> bool {
		self.skip_inner.trait_skipped(trait_)
			|| match self.fields() {
				Either::Left(fields) => fields.any_skip_trait(trait_),
				Either::Right(_) => false,
			}
	}

	/// Returns `true` if all fields are skipped with that [`Trait`].
	fn skip(&self, trait_: Trait) -> bool {
		self.skip_inner.trait_skipped(trait_)
			|| match self.fields() {
				Either::Left(fields) => fields.skip(trait_),
				Either::Right(_) => false,
			}
	}

	/// Return a [`SimpleType`].
	pub fn simple_type(&self) -> SimpleType {
		match &self.type_ {
			DataType::Struct(fields)
			| DataType::Variant {
				type_: VariantType::Struct(fields),
				..
			} => SimpleType::Struct(fields),
			DataType::Tuple(fields)
			| DataType::Variant {
				type_: VariantType::Tuple(fields),
				..
			} => SimpleType::Tuple(fields),
			DataType::Unit(pattern)
			| DataType::Variant {
				type_: VariantType::Unit(pattern),
				..
			} => SimpleType::Unit(pattern),
			DataType::Union(fields) => SimpleType::Union(fields),
		}
	}

	/// Returns an [`Iterator`] over [`Field`]s.
	pub fn iter_fields(
		&self,
		trait_: Trait,
	) -> impl '_ + Iterator<Item = &'_ Field> + DoubleEndedIterator {
		if self.skip(trait_) {
			[].iter()
		} else {
			match self.fields() {
				Either::Left(fields) => fields.fields.iter(),
				Either::Right(_) => [].iter(),
			}
		}
		.filter(move |field| !field.skip(trait_))
	}

	/// Returns an [`Iterator`] over [`Member`]s.
	pub fn iter_field_ident(&self, trait_: Trait) -> impl '_ + Iterator<Item = &'_ Member> {
		self.iter_fields(trait_).map(|field| &field.member)
	}

	/// Returns an [`Iterator`] over [`struct@Ident`]s used as temporary
	/// variables for destructuring `self`.
	pub fn iter_self_ident(
		&self,
		trait_: Trait,
	) -> impl Iterator<Item = &'_ Ident> + DoubleEndedIterator {
		self.iter_fields(trait_).map(|field| &field.self_ident)
	}

	/// Returns an [`Iterator`] over [`struct@Ident`]s used as temporary
	/// variables for destructuring `other`.
	pub fn iter_other_ident(
		&self,
		trait_: Trait,
	) -> impl Iterator<Item = &'_ Ident> + DoubleEndedIterator {
		self.iter_fields(trait_).map(|field| &field.other_ident)
	}
}
