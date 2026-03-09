//! [`Attribute`] parsing for items.

use std::{borrow::Cow, ops::Deref};

use proc_macro2::Span;
use syn::{
	parse::{discouraged::Speculative, Parse, ParseStream},
	punctuated::Punctuated,
	spanned::Spanned,
	Attribute, Data, Ident, Meta, Path, PredicateType, Result, Token, TraitBound,
	TraitBoundModifier, Type, TypeParamBound, TypePath, WhereClause, WherePredicate,
};

use crate::{
	util::{self, MetaListExt},
	Error, Incomparable, Item, Skip, SkipGroup, Trait, TraitImpl, DERIVE_WHERE,
};

/// Attributes on item.
#[derive(Default)]
pub struct ItemAttr {
	/// [`Trait`]s to skip all fields for.
	pub skip_inner: Skip,
	/// Comparing item will yield `false` for [`PartialEq`] and [`None`] for
	/// [`PartialOrd`].
	pub incomparable: Incomparable,
	/// [`DeriveWhere`]s on this item.
	pub derive_wheres: Vec<DeriveWhere>,
}

impl ItemAttr {
	/// Create [`ItemAttr`] from [`Attribute`]s.
	pub fn from_attrs(span: Span, data: &Data, attrs: &[Attribute]) -> Result<Self> {
		let mut self_ = ItemAttr::default();
		let mut skip_inners = Vec::new();
		let mut incomparables = Vec::new();

		for attr in attrs {
			if attr.path().is_ident(DERIVE_WHERE) {
				if let Meta::List(list) = &attr.meta {
					if let Ok(nested) =
						list.parse_args_with(Punctuated::<Meta, Token![,]>::parse_terminated)
					{
						match nested.len() {
							// Don't allow an empty list.
							0 => return Err(Error::empty(list.span())),
							// Check for `skip_inner` if list only has one item.
							1 => {
								let meta =
									nested.into_iter().next().expect("unexpected empty list");

								if meta.path().is_ident(Skip::SKIP_INNER) {
									// Don't allow `skip_inner` on the item level for enums.
									if let Data::Enum(_) = data {
										return Err(Error::option_enum_skip_inner(meta.span()));
									}

									// Don't parse `Skip` yet, because it needs access to all
									// `DeriveWhere`s.
									skip_inners.push(meta);
								} else if meta.path().is_ident(Incomparable::INCOMPARABLE) {
									// Needs to be parsed after all traits are known.
									incomparables.push(meta)
								} else if meta.path().is_ident("crate") {
									// Do nothing, we checked this before
									// already.
								}
								// The list can have one item but still not be the `skip_inner`
								// attribute, continue with parsing `DeriveWhere`.
								else {
									self_
										.derive_wheres
										.push(DeriveWhere::from_attr(span, data, attr)?);
								}
							}
							_ => self_
								.derive_wheres
								.push(DeriveWhere::from_attr(span, data, attr)?),
						}
					}
					// Anything list that isn't using `,` as separator, is because we expect
					// `A, B; C`.
					else {
						self_
							.derive_wheres
							.push(DeriveWhere::from_attr(span, data, attr)?)
					}
				} else {
					return Err(Error::option_syntax(attr.meta.span()));
				}
			}
		}

		// Check that we specified at least one `#[derive_where(..)]` with traits.
		if self_.derive_wheres.is_empty() {
			return Err(Error::none(span));
		}

		// Merge `DeriveWhere`s with the same bounds.
		self_
			.derive_wheres
			.dedup_by(|derive_where_1, derive_where_2| {
				if derive_where_1.generics == derive_where_2.generics {
					derive_where_2.spans.append(&mut derive_where_1.spans);
					derive_where_2.traits.append(&mut derive_where_1.traits);
					true
				} else {
					false
				}
			});

		// Check for duplicate traits in the same `derive_where` after merging with the
		// same bounds.
		for derive_where in &self_.derive_wheres {
			for (skip, trait_) in (1..).zip(&derive_where.traits) {
				if let Some((span, _)) = derive_where
					.spans
					.iter()
					.zip(&derive_where.traits)
					.skip(skip)
					.find(|(_, other_trait)| *other_trait == trait_)
				{
					return Err(Error::trait_duplicate(*span));
				}
			}
		}

		// Delayed parsing of `skip_inner` and `incomparable` to get access to all
		// traits to be implemented.
		for meta in skip_inners {
			self_
				.skip_inner
				.add_attribute(&self_.derive_wheres, None, &meta)?;
		}

		for meta in incomparables {
			self_
				.incomparable
				.add_attribute(&meta, &self_.derive_wheres)?;
		}

		Ok(self_)
	}
}

/// Holds parsed [generics](Generic) and [traits](crate::Trait).
pub struct DeriveWhere {
	/// [`Span`]s for each [trait](DeriveTrait).
	pub spans: Vec<Span>,
	/// [Traits](DeriveTrait) to implement.
	pub traits: Vec<DeriveTrait>,
	/// [Generics](Generic) for where clause.
	pub generics: Vec<Generic>,
}

impl DeriveWhere {
	/// Create [`DeriveWhere`] from [`Attribute`].
	fn from_attr(span: Span, data: &Data, attr: &Attribute) -> Result<Self> {
		attr.parse_args_with(|input: ParseStream| {
			// Parse the attribute input, this should either be:
			// - Comma separated traits.
			// - Comma separated traits `;` Comma separated generics.

			let mut spans = Vec::new();
			let mut traits = Vec::new();
			let mut generics = Vec::new();

			// Check for an empty list is already done in `ItemAttr::from_attrs`.
			assert!(!input.is_empty());

			while !input.is_empty() {
				// Start with parsing a trait.
				// Not checking for duplicates here, we do that after merging `derive_where`s
				// with the same bounds.
				let (span, trait_) = DeriveTrait::from_stream(span, data, input)?;
				spans.push(span);
				traits.push(trait_);

				if !input.is_empty() {
					let mut fork = input.fork();

					// Track `Span` of whatever was found instead of a delimiter. We parse the `,`
					// first because it's allowed to be followed by a `;`.
					let no_delimiter_found = match <Token![,]>::parse(&fork) {
						Ok(_) => {
							input.advance_to(&fork);
							None
						}
						Err(error) => {
							// Reset the fork if we didn't find a `,`.
							fork = input.fork();
							Some(error.span())
						}
					};

					if <Token![;]>::parse(&fork).is_ok() {
						input.advance_to(&fork);

						// If we found a semi-colon, start parsing generics.
						if !input.is_empty() {
							// `parse_terminated` parses everything left, which should end the
							// while-loop.
							// Not checking for duplicates here, as even Rust doesn't give a warning
							// for those: `where T: Clone, T: Clone` produces no error or warning.
							generics = Punctuated::<Generic, Token![,]>::parse_terminated(input)?
								.into_iter()
								.collect();
						}
					}
					// We are here because the input isn't empty, but we also found no delimiter,
					// something unexpected is here instead.
					else if let Some(span) = no_delimiter_found {
						return Err(Error::derive_where_delimiter(span));
					}
				}
			}

			Ok(Self {
				generics,
				spans,
				traits,
			})
		})
	}

	/// Returns `true` if [`Trait`] is present.
	pub fn contains(&self, trait_: Trait) -> bool {
		self.traits
			.iter()
			.any(|derive_trait| derive_trait == trait_)
	}

	/// Returns `true` if any [`CustomBound`](Generic::CustomBound) is present.
	pub fn any_custom_bound(&self) -> bool {
		self.generics.iter().any(|generic| match generic {
			Generic::CustomBound(_) => true,
			Generic::NoBound(_) => false,
		})
	}

	/// Returns `true` if the given generic type parameter if present.
	pub fn has_type_param(&self, type_param: &Ident) -> bool {
		self.generics.iter().any(|generic| match generic {
			Generic::NoBound(Type::Path(TypePath { qself: None, path })) => {
				if let Some(ident) = path.get_ident() {
					ident == type_param
				} else {
					false
				}
			}
			_ => false,
		})
	}

	/// Returns `true` if any [`Trait`] supports skipping.
	pub fn any_skip(&self) -> bool {
		self.traits
			.iter()
			.any(|trait_| SkipGroup::trait_supported(**trait_))
	}

	/// Create [`WhereClause`] for the given parameters.
	pub fn where_clause(
		&self,
		where_clause: &mut Option<Cow<WhereClause>>,
		trait_: &DeriveTrait,
		item: &Item,
	) {
		// Only create a where clause if required
		if !self.generics.is_empty() {
			// We use the existing where clause or create a new one if required.
			let where_clause = where_clause.get_or_insert(Cow::Owned(WhereClause {
				where_token: <Token![where]>::default(),
				predicates: Punctuated::default(),
			}));

			// Insert bounds into the `where` clause.
			for generic in &self.generics {
				where_clause
					.to_mut()
					.predicates
					.push(WherePredicate::Type(match generic {
						Generic::CustomBound(type_bound) => type_bound.clone(),
						Generic::NoBound(path) => PredicateType {
							lifetimes: None,
							bounded_ty: path.clone(),
							colon_token: <Token![:]>::default(),
							bounds: trait_.where_bounds(item),
						},
					}));
			}
		}
	}
}

/// Holds a single generic [type](Type) or [type with bound](PredicateType).
#[derive(Eq, PartialEq)]
pub enum Generic {
	/// Generic type with custom [specified bounds](PredicateType).
	CustomBound(PredicateType),
	/// Generic [type](Type) which will be bound to the [`DeriveTrait`].
	NoBound(Type),
}

impl Parse for Generic {
	fn parse(input: ParseStream) -> Result<Self> {
		let fork = input.fork();

		// Try to parse input as a `WherePredicate`. The problem is, both expressions
		// start with a Type, so starting with the `WherePredicate` is the easiest way
		// of differentiating them.
		if let Ok(where_predicate) = WherePredicate::parse(&fork) {
			input.advance_to(&fork);

			// Don't allow lifetimes, as it doesn't make sense in the context.
			if let WherePredicate::Type(path) = where_predicate {
				Ok(Generic::CustomBound(path))
			} else {
				Err(Error::generic(where_predicate.span()))
			}
		} else {
			match Type::parse(input) {
				Ok(type_) => Ok(Generic::NoBound(type_)),
				Err(error) => Err(Error::generic_syntax(error.span(), error)),
			}
		}
	}
}

/// Trait to implement.
#[derive(Eq, PartialEq)]
pub enum DeriveTrait {
	/// [`Clone`].
	Clone,
	/// [`Copy`].
	Copy,
	/// [`Debug`](std::fmt::Debug).
	Debug,
	/// [`Default`].
	Default,
	/// [`Eq`].
	Eq,
	/// [`Hash`](std::hash::Hash).
	Hash,
	/// [`Ord`].
	Ord,
	/// [`PartialEq`].
	PartialEq,
	/// [`PartialOrd`].
	PartialOrd,
	/// [`Zeroize`](https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html).
	#[cfg(feature = "zeroize")]
	Zeroize {
		/// [`Zeroize`](https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html) path.
		crate_: Option<Path>,
	},
	/// [`ZeroizeOnDrop`](https://docs.rs/zeroize/latest/zeroize/trait.ZeroizeOnDrop.html).
	#[cfg(feature = "zeroize")]
	ZeroizeOnDrop {
		/// [`ZeroizeOnDrop`](https://docs.rs/zeroize/latest/zeroize/trait.ZeroizeOnDrop.html) path.
		crate_: Option<Path>,
	},
}

impl Deref for DeriveTrait {
	type Target = Trait;

	fn deref(&self) -> &Self::Target {
		use DeriveTrait::*;

		match self {
			Clone => &Trait::Clone,
			Copy => &Trait::Copy,
			Debug => &Trait::Debug,
			Default => &Trait::Default,
			Eq => &Trait::Eq,
			Hash => &Trait::Hash,
			Ord => &Trait::Ord,
			PartialEq => &Trait::PartialEq,
			PartialOrd => &Trait::PartialOrd,
			#[cfg(feature = "zeroize")]
			Zeroize { .. } => &Trait::Zeroize,
			#[cfg(feature = "zeroize")]
			ZeroizeOnDrop { .. } => &Trait::ZeroizeOnDrop,
		}
	}
}

impl PartialEq<Trait> for &DeriveTrait {
	fn eq(&self, other: &Trait) -> bool {
		let trait_: &Trait = self;
		trait_ == other
	}
}

impl DeriveTrait {
	/// Returns fully qualified [`Path`] for this trait.
	pub fn path(&self) -> Path {
		use DeriveTrait::*;

		match self {
			Clone => util::path_from_root_and_strs(self.crate_(), &["clone", "Clone"]),
			Copy => util::path_from_root_and_strs(self.crate_(), &["marker", "Copy"]),
			Debug => util::path_from_root_and_strs(self.crate_(), &["fmt", "Debug"]),
			Default => util::path_from_root_and_strs(self.crate_(), &["default", "Default"]),
			Eq => util::path_from_root_and_strs(self.crate_(), &["cmp", "Eq"]),
			Hash => util::path_from_root_and_strs(self.crate_(), &["hash", "Hash"]),
			Ord => util::path_from_root_and_strs(self.crate_(), &["cmp", "Ord"]),
			PartialEq => util::path_from_root_and_strs(self.crate_(), &["cmp", "PartialEq"]),
			PartialOrd => util::path_from_root_and_strs(self.crate_(), &["cmp", "PartialOrd"]),
			#[cfg(feature = "zeroize")]
			Zeroize { .. } => util::path_from_root_and_strs(self.crate_(), &["Zeroize"]),
			#[cfg(feature = "zeroize")]
			ZeroizeOnDrop { .. } => util::path_from_root_and_strs(self.crate_(), &["ZeroizeOnDrop"]),
		}
	}

	/// Returns the path to the root crate for this trait.
	pub fn crate_(&self) -> Path {
		use DeriveTrait::*;

		match self {
			Clone => util::path_from_strs(&["core"]),
			Copy => util::path_from_strs(&["core"]),
			Debug => util::path_from_strs(&["core"]),
			Default => util::path_from_strs(&["core"]),
			Eq => util::path_from_strs(&["core"]),
			Hash => util::path_from_strs(&["core"]),
			Ord => util::path_from_strs(&["core"]),
			PartialEq => util::path_from_strs(&["core"]),
			PartialOrd => util::path_from_strs(&["core"]),
			#[cfg(feature = "zeroize")]
			Zeroize { crate_, .. } => {
				if let Some(crate_) = crate_ {
					crate_.clone()
				} else {
					util::path_from_strs(&["zeroize"])
				}
			}
			#[cfg(feature = "zeroize")]
			ZeroizeOnDrop { crate_, .. } => {
				if let Some(crate_) = crate_ {
					crate_.clone()
				} else {
					util::path_from_strs(&["zeroize"])
				}
			}
		}
	}

	/// Returns where-clause bounds for the trait in respect of the item type.
	fn where_bounds(&self, data: &Item) -> Punctuated<TypeParamBound, Token![+]> {
		let mut list = Punctuated::new();

		list.push(TypeParamBound::Trait(TraitBound {
			paren_token: None,
			modifier: TraitBoundModifier::None,
			lifetimes: None,
			path: self.path(),
		}));

		// Add bounds specific to the trait.
		if let Some(bound) = self.additional_where_bounds(data) {
			list.push(bound)
		}

		list
	}

	/// Create [`DeriveTrait`] from [`ParseStream`].
	fn from_stream(span: Span, data: &Data, input: ParseStream) -> Result<(Span, Self)> {
		match Meta::parse(input) {
			Ok(meta) => {
				let trait_ = Trait::from_path(meta.path())?;

				if let Data::Union(_) = data {
					// Make sure this `Trait` supports unions.
					if !trait_.supports_union() {
						return Err(Error::union(span));
					}
				}

				match &meta {
					Meta::Path(path) => Ok((path.span(), trait_.default_derive_trait())),
					Meta::List(list) => {
						let nested = list.parse_non_empty_nested_metas()?;

						// This will return an error if no options are supported.
						Ok((list.span(), trait_.parse_derive_trait(meta.span(), nested)?))
					}
					Meta::NameValue(name_value) => Err(Error::option_syntax(name_value.span())),
				}
			}
			Err(error) => Err(Error::trait_syntax(error.span())),
		}
	}
}
