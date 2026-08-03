#![deny(unsafe_code)]
#![cfg_attr(
	feature = "nightly",
	feature(allow_internal_unstable),
	allow(internal_features)
)]
#![allow(clippy::tabs_in_doc_comments)]
#![warn(clippy::cargo, clippy::missing_docs_in_private_items)]
#![cfg_attr(feature = "nightly", allow(clippy::implied_bounds_in_impls))]
#![cfg_attr(doc, allow(unknown_lints), warn(rustdoc::all))]

//! # Description
//!
//! Attribute proc-macro to simplify deriving standard and other traits with
//! custom generic type bounds.
//!
//! # Usage
//!
//! The [`derive_where`](macro@derive_where) attribute can be used just like
//! std's `#[derive(...)]` statements:
//!
//! ```
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! #[derive_where(Clone, Debug)]
//! struct Example<T>(PhantomData<T>);
//! ```
//!
//! This will generate trait implementations for `Example` for any `T`,
//! as opposed to std's derives, which would only implement these traits with
//! `T: Trait` bound to the corresponding trait.
//!
//! Multiple [`derive_where`](macro@derive_where) attributes can be added to an
//! item, but only the first one must use any path qualifications.
//!
//! ```
//! # use std::marker::PhantomData;
//! #[derive_where::derive_where(Clone, Debug)]
//! #[derive_where(Eq, PartialEq)]
//! struct Example1<T>(PhantomData<T>);
//! ```
//!
//! If using a different package name, you must specify this:
//!
//! ```
//! # extern crate derive_where as derive_where_;
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! #[derive_where(crate = derive_where_)]
//! #[derive_where(Clone, Debug)]
//! struct Example<T>(PhantomData<T>);
//! ```
//!
//! In addition, the following convenience options are available:
//!
//! ## Generic type bounds
//!
//! Separated from the list of traits with a semi-colon, types to bind to can be
//! specified. This example will restrict the implementation for `Example` to
//! `T: Clone`:
//!
//! ```
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! #[derive_where(Clone, Debug; T)]
//! struct Example<T, U>(T, PhantomData<U>);
//! ```
//!
//! It is also possible to specify the bounds to be applied. This will
//! bind implementation for `Example` to `T: Super`:
//!
//! ```
//! # use std::fmt::Debug;
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! trait Super: Clone + Debug {}
//!
//! #[derive_where(Clone, Debug; T: Super)]
//! struct Example<T>(PhantomData<T>);
//! ```
//!
//! But more complex trait bounds are possible as well.
//! The example below will restrict the [`Clone`] implementation for `Example`
//! to `T::Type: Clone`:
//!
//! ```
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! trait Trait {
//! 	type Type;
//! }
//!
//! struct Impl;
//!
//! impl Trait for Impl {
//! 	type Type = i32;
//! }
//!
//! #[derive_where(Clone, Debug; T::Type)]
//! struct Example<T: Trait>(T::Type);
//! ```
//!
//! Any combination of options listed here can be used to satisfy a
//! specific constrain. It is also possible to use multiple separate
//! constrain specifications when required:
//!
//! ```
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! #[derive_where(Clone, Debug; T)]
//! #[derive_where(Eq, PartialEq; U)]
//! struct Example<T, U>(PhantomData<T>, PhantomData<U>);
//! ```
//!
//! ## Enum default
//!
//! Since Rust 1.62 deriving [`Default`] on an enum is possible with the
//! `#[default]` attribute. Derive-where allows this with a
//! `#[derive_where(default)]` attribute:
//!
//! ```
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! #[derive_where(Clone, Default)]
//! enum Example<T> {
//! 	#[derive_where(default)]
//! 	A(PhantomData<T>),
//! }
//! ```
//!
//! ## Skipping fields
//!
//! With a `skip` or `skip_inner` attribute fields can be skipped for traits
//! that allow it, which are: [`Debug`], [`Hash`], [`Ord`], [`PartialOrd`],
//! [`PartialEq`], [`Zeroize`] and [`ZeroizeOnDrop`].
//!
//! ```
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! #[derive_where(Debug, PartialEq; T)]
//! struct Example<T>(#[derive_where(skip)] T);
//!
//! assert_eq!(format!("{:?}", Example(42)), "Example");
//! assert_eq!(Example(42), Example(0));
//! ```
//!
//! It is also possible to skip all fields in an item or variant if desired:
//!
//! ```
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! #[derive_where(Debug, PartialEq)]
//! #[derive_where(skip_inner)]
//! struct StructExample<T>(T);
//!
//! assert_eq!(format!("{:?}", StructExample(42)), "StructExample");
//! assert_eq!(StructExample(42), StructExample(0));
//!
//! #[derive_where(Debug, PartialEq)]
//! enum EnumExample<T> {
//! 	#[derive_where(skip_inner)]
//! 	A(T),
//! }
//!
//! assert_eq!(format!("{:?}", EnumExample::A(42)), "A");
//! assert_eq!(EnumExample::A(42), EnumExample::A(0));
//! ```
//!
//! Selective skipping of fields for certain traits is also an option, both in
//! `skip` and `skip_inner`. To prevent breaking invariants defined for these
//! traits, some of them can only be skipped in groups. The following groups are
//! available:
//! - [`Debug`]
//! - `EqHashOrd`: Skips [`Eq`], [`Hash`], [`Ord`], [`PartialOrd`] and
//!   [`PartialEq`].
//! - [`Hash`]
//! - `Zeroize`: Skips [`Zeroize`] and [`ZeroizeOnDrop`].
//!
//! ```
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! #[derive_where(Debug, PartialEq)]
//! #[derive_where(skip_inner(Debug))]
//! struct Example<T>(i32, PhantomData<T>);
//!
//! assert_eq!(format!("{:?}", Example(42, PhantomData::<()>)), "Example");
//! assert_ne!(
//! 	Example(42, PhantomData::<()>),
//! 	Example(0, PhantomData::<()>)
//! );
//! ```
//!
//! ## Incomparable variants/items
//!
//! Similar to the `skip` attribute, `incomparable` can be used to skip variants
//! or items in [`PartialEq`] and [`PartialOrd`] trait implementations, meaning
//! they will always yield `false` for `eq` and `None` for `partial_cmp`. This
//! results in all comparisons but `!=`, i.e. `==`, `<`, `<=`, `>=` and `>`,
//! with the marked variant or struct evaluating to `false`.
//!
//! ```
//! # use derive_where::derive_where;
//! #[derive(Debug)]
//! #[derive_where(PartialEq, PartialOrd)]
//! enum EnumExample {
//! 	#[derive_where(incomparable)]
//! 	Incomparable,
//! 	Comparable,
//! }
//! assert_eq!(EnumExample::Comparable, EnumExample::Comparable);
//! assert_ne!(EnumExample::Incomparable, EnumExample::Incomparable);
//! assert!(!(EnumExample::Comparable >= EnumExample::Incomparable));
//! assert!(!(EnumExample::Comparable <= EnumExample::Incomparable));
//! assert!(!(EnumExample::Incomparable >= EnumExample::Incomparable));
//! assert!(!(EnumExample::Incomparable <= EnumExample::Incomparable));
//!
//! #[derive(Debug)]
//! #[derive_where(PartialEq, PartialOrd)]
//! #[derive_where(incomparable)]
//! struct StructExample;
//!
//! assert_ne!(StructExample, StructExample);
//! assert!(!(StructExample >= StructExample));
//! assert!(!(StructExample <= StructExample));
//! ```
//!
//! Note that it is not possible to use `incomparable` with [`Eq`] or [`Ord`] as
//! that would break their invariants.
//!
//! ## `Zeroize` options
//!
//! `Zeroize` has two options:
//! - `crate`: an item-level option which specifies a path to the [`zeroize`]
//!   crate in case of a re-export or rename.
//! - `fqs`: a field-level option which will use fully-qualified-syntax instead
//!   of calling the [`zeroize`][method@zeroize] method on `self` directly. This
//!   is to avoid ambiguity between another method also called `zeroize`.
//!
//! ```
//! # #[cfg(feature = "zeroize")]
//! # {
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! # use zeroize_::Zeroize;
//! #[derive_where(Zeroize(crate = zeroize_))]
//! struct Example(#[derive_where(Zeroize(fqs))] i32);
//!
//! impl Example {
//! 	// If we didn't specify the `fqs` option, this would lead to a compile
//! 	// error because of method ambiguity.
//! 	fn zeroize(&mut self) {
//! 		self.0 = 1;
//! 	}
//! }
//!
//! let mut test = Example(42);
//!
//! // Will call the struct method.
//! test.zeroize();
//! assert_eq!(test.0, 1);
//!
//! // WIll call the `Zeroize::zeroize` method.
//! Zeroize::zeroize(&mut test);
//! assert_eq!(test.0, 0);
//! # }
//! ```
//!
//! ## `ZeroizeOnDrop` options
//!
//! If the `zeroize-on-drop` feature is enabled, it implements [`ZeroizeOnDrop`]
//! and can be implemented without [`Zeroize`], otherwise it only implements
//! [`Drop`] and requires [`Zeroize`] to be implemented.
//!
//! [`ZeroizeOnDrop`] has one option:
//! - `crate`: an item-level option which specifies a path to the [`zeroize`]
//!   crate in case of a re-export or rename.
//!
//! ```
//! # #[cfg(feature = "zeroize-on-drop")]
//! # {
//! # use std::marker::PhantomData;
//! # use derive_where::derive_where;
//! #[derive_where(ZeroizeOnDrop(crate = zeroize_))]
//! struct Example(i32);
//!
//! assert!(core::mem::needs_drop::<Example>());
//! # }
//! ```
//!
//! ## Supported traits
//!
//! The following traits can be derived with derive-where:
//! - [`Clone`]
//! - [`Copy`]
//! - [`Debug`]
//! - [`Default`]
//! - [`Eq`]
//! - [`Hash`]
//! - [`Ord`]
//! - [`PartialEq`]
//! - [`PartialOrd`]
//! - [`Zeroize`]: Only available with the `zeroize` crate feature.
//! - [`ZeroizeOnDrop`]: Only available with the `zeroize` crate feature. If the
//!   `zeroize-on-drop` feature is enabled, it implements [`ZeroizeOnDrop`],
//!   otherwise it only implements [`Drop`].
//!
//! ## Supported items
//!
//! Structs, tuple structs, unions and enums are supported. Derive-where tries
//! it's best to discourage usage that could be covered by std's `derive`. For
//! example unit structs and enums only containing unit variants aren't
//! supported.
//!
//! Unions only support [`Clone`] and [`Copy`].
//!
//! [`PartialOrd`] and [`Ord`] need to determine the discriminant type to
//! function correctly. To protect against a potential future change to the
//! default discriminant type, some compile-time validation is inserted to
//! ascertain that the type remains `isize`.
//!
//! ## `no_std` support
//!
//! `no_std` support is provided by default.
//!
//! # Crate features
//!
//! - `nightly`: Implements [`Ord`] and [`PartialOrd`] with the help of
//!   [`core::intrinsics::discriminant_value`], which is what Rust does by
//!   default too. This requires a nightly version of the Rust compiler.
//! - `safe`: `safe`: Uses only safe ways to access the discriminant of the enum
//!   for [`Ord`] and [`PartialOrd`]. It also replaces all cases of
//!   [`core::hint::unreachable_unchecked`] in [`Ord`], [`PartialEq`] and
//!   [`PartialOrd`], which is what std uses, with [`unreachable`].
//! - `zeroize`: Allows deriving [`Zeroize`] and [`zeroize`][method@zeroize] on
//!   [`Drop`].
//! - `zeroize-on-drop`: Allows deriving [`Zeroize`] and [`ZeroizeOnDrop`] and
//!   requires [`zeroize`] v1.5.
//!
//! # MSRV
//!
//! The current MSRV is 1.57 and is being checked by the CI. A change will be
//! accompanied by a minor version bump. If MSRV is important to you, use
//! `derive-where = "~1.x"` to pin a specific minor version to your crate.
//!
//! # Alternatives
//!
//! - [derivative](https://crates.io/crates/derivative) [![Crates.io](https://img.shields.io/crates/v/derivative.svg)](https://crates.io/crates/derivative)
//!   is a great alternative with many options. Notably it doesn't support
//!   `no_std` and requires an extra `#[derive(Derivative)]` to use.
//! - [derive_bounded](https://crates.io/crates/derive_bounded) [![Crates.io](https://img.shields.io/crates/v/derive_bounded.svg)](https://crates.io/crates/derive_bounded)
//!   is a new alternative still in development.
//!
//! # Changelog
//!
//! See the [CHANGELOG] file for details.
//!
//! # License
//!
//! Licensed under either of
//!
//! - Apache License, Version 2.0 ([LICENSE-APACHE] or <http://www.apache.org/licenses/LICENSE-2.0>)
//! - MIT license ([LICENSE-MIT] or <http://opensource.org/licenses/MIT>)
//!
//! at your option.
//!
//! ## Contribution
//!
//! Unless you explicitly state otherwise, any contribution intentionally
//! submitted for inclusion in the work by you, as defined in the Apache-2.0
//! license, shall be dual licensed as above, without any additional terms or
//! conditions.
//!
//! [CHANGELOG]: https://github.com/ModProg/derive-where/blob/main/CHANGELOG.md
//! [LICENSE-MIT]: https://github.com/ModProg/derive-where/blob/main/LICENSE-MIT
//! [LICENSE-APACHE]: https://github.com/ModProg/derive-where/blob/main/LICENSE-APACHE
//! [`Debug`]: core::fmt::Debug
//! [`Default`]: core::default::Default
//! [`Eq`]: core::cmp::Eq
//! [`Hash`]: core::hash::Hash
//! [`Ord`]: core::cmp::Ord
//! [`PartialEq`]: core::cmp::PartialEq
//! [`PartialOrd`]: core::cmp::PartialOrd
//! [`zeroize`]: https://docs.rs/zeroize
//! [`Zeroize`]: https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html
//! [`ZeroizeOnDrop`]: https://docs.rs/zeroize/latest/zeroize/trait.ZeroizeOnDrop.html
//! [method@zeroize]: https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html#tymethod.zeroize

mod attr;
mod data;
mod error;
mod input;
mod item;
#[cfg(test)]
mod test;
mod trait_;
mod util;

use std::{borrow::Cow, iter};

use input::SplitGenerics;
use proc_macro2::TokenStream;
use quote::{quote, quote_spanned, ToTokens};
use syn::{
	spanned::Spanned, Attribute, DataEnum, DataStruct, DataUnion, DeriveInput, Expr, ExprLit,
	ExprPath, Fields, FieldsNamed, FieldsUnnamed, Lit, Meta, Path, Result, Variant,
};
use util::MetaListExt;

#[cfg(feature = "zeroize")]
use self::attr::ZeroizeFqs;
use self::{
	attr::{
		Default, DeriveTrait, DeriveWhere, FieldAttr, Incomparable, ItemAttr, Skip, SkipGroup,
		VariantAttr,
	},
	data::{Data, DataType, Field, SimpleType},
	error::Error,
	input::Input,
	item::{Discriminant, Item},
	trait_::{Trait, TraitImpl},
	util::Either,
};

/// Name of the `derive_where` attribute proc-macro.
const DERIVE_WHERE: &str = "derive_where";
/// Name of the `DeriveWhere` derive proc-macro.
const DERIVE_WHERE_FORWARD: &str = "DeriveWhere";
/// Name of the `derive_where_visited` proc-macro.
const DERIVE_WHERE_VISITED: &str = "derive_where_visited";

/// Item-level options:
/// - `#[derive_where(crate = path)]`: Specify path to the `derive_where` crate.
/// - `#[derive_where(Clone, ..; T, ..)]`: Specify traits to implement and
///   optionally bounds.
///   - `#[derive_where(Zeroize(crate = path))]`: Specify path to [`Zeroize`]
///     trait.
///   - `#[derive_where(ZeroizeOnDrop(crate = path))]`: Specify path to
///     [`ZeroizeOnDrop`] trait.
/// - `#[derive_where(skip_inner(EqHashOrd, ..))]`: Skip all fields in the item.
///   Optionally specify trait groups to constrain skipping fields. Only works
///   for structs, for enums use this on the variant-level.
///
/// Variant-level options:
/// - `#[derive_where(default)]`: Uses this variant as the default for the
///   [`Default`](trait@core::default::Default) implementation.
/// - `#[derive_where(skip_inner(EqHashOrd, ..))]`: Skip all fields in this
///   variant. Optionally specify trait groups to constrain skipping fields.
///
/// Field-level options:
/// - `#[derive_where(skip(EqHashOrd, ...))]`: Skip field. Optionally specify
///   trait groups to constrain skipping field.
/// - `#[derive_where(Zeroize(fqs))]`: Use fully-qualified-syntax when
///   implementing [`Zeroize`].
///
/// See the [crate] level description for more details.
///
/// [`Zeroize`]: https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html
/// [`ZeroizeOnDrop`]: https://docs.rs/zeroize/latest/zeroize/trait.ZeroizeOnDrop.html
#[proc_macro_attribute]
pub fn derive_where(
	attr: proc_macro::TokenStream,
	original_input: proc_macro::TokenStream,
) -> proc_macro::TokenStream {
	let attr = TokenStream::from(attr);
	let mut original_input = TokenStream::from(original_input);
	let mut input = quote_spanned! { attr.span()=> #[derive_where(#attr)] };
	input.extend(original_input.clone());

	match syn::parse2::<DeriveInput>(input) {
		Ok(input) => match derive_where_internal(input.clone()) {
			Ok(item) => item.into(),
			Err(error) => {
				let mut clean_input =
					input_without_derive_where_attributes(input).into_token_stream();
				clean_input.extend(error.into_compile_error());
				clean_input.into()
			}
		},
		Err(error) => {
			original_input.extend(error.into_compile_error());
			original_input.into()
		}
	}
}

/// Convenient way to deal with [`Result`] for [`derive_where()`].
fn derive_where_internal(mut item: DeriveInput) -> Result<TokenStream> {
	let mut crate_ = None;

	// Search for `crate` option.
	for attr in &item.attrs {
		if attr.path().is_ident(DERIVE_WHERE) {
			if let Meta::List(list) = &attr.meta {
				if let Ok(nested) = list.parse_non_empty_nested_metas() {
					if nested.len() == 1 {
						let meta = nested.into_iter().next().expect("unexpected empty list");

						if meta.path().is_ident("crate") {
							if let Meta::NameValue(name_value) = meta {
								let path = match &name_value.value {
									Expr::Lit(ExprLit {
										lit: Lit::Str(lit_str),
										..
									}) => match lit_str.parse::<Path>() {
										Ok(path) => path,
										Err(error) => {
											return Err(Error::path(lit_str.span(), error))
										}
									},
									Expr::Path(ExprPath { path, .. }) => path.clone(),
									_ => return Err(Error::option_syntax(name_value.value.span())),
								};

								if path == util::path_from_strs(&[DERIVE_WHERE]) {
									return Err(Error::path_unnecessary(
										path.span(),
										&format!("::{}", DERIVE_WHERE),
									));
								}

								match crate_ {
									Some(_) => {
										return Err(Error::option_duplicate(
											name_value.span(),
											"crate",
										))
									}
									None => crate_ = Some(path),
								}
							} else {
								return Err(Error::option_syntax(meta.span()));
							}
						}
					}
				}
			}
		}
	}

	// Build [`Path`] to crate.
	let crate_ = crate_.unwrap_or_else(|| util::path_from_strs(&[DERIVE_WHERE]));

	// Build `derive_where_visited` path.
	let derive_where_visited =
		util::path_from_root_and_strs(crate_.clone(), &[DERIVE_WHERE_VISITED]);

	// Check if we already parsed this item before.
	for attr in &item.attrs {
		if attr.path() == &derive_where_visited {
			return Err(Error::visited(attr.span()));
		}
	}

	// Mark this as visited to prevent duplicate `derive_where` attributes.
	item.attrs
		.push(syn::parse_quote! { #[#derive_where_visited] });

	// Build `DeriveWhere` path.
	let derive_where = util::path_from_root_and_strs(crate_, &[DERIVE_WHERE_FORWARD]);

	// Let the `derive` proc-macro parse this.
	let mut output = quote! { #[derive(#derive_where)] };
	output.extend(item.into_token_stream());
	Ok(output)
}

#[doc(hidden)]
#[proc_macro_derive(DeriveWhere, attributes(derive_where))]
#[cfg_attr(feature = "nightly", allow_internal_unstable(core_intrinsics))]
pub fn derive_where_actual(input: proc_macro::TokenStream) -> proc_macro::TokenStream {
	let input = TokenStream::from(input);
	let item = match syn::parse2::<DeriveInput>(input) {
		Ok(item) => item,
		Err(error) => {
			return error.into_compile_error().into();
		}
	};

	let span = {
		let clean_item = DeriveInput {
			attrs: Vec::new(),
			vis: item.vis.clone(),
			ident: item.ident.clone(),
			generics: item.generics.clone(),
			data: item.data.clone(),
		};

		clean_item.span()
	};

	match { Input::from_input(span, &item) } {
		Ok(Input {
			derive_wheres,
			generics,
			item,
		}) => derive_wheres
			.iter()
			.flat_map(|derive_where| iter::repeat(derive_where).zip(&derive_where.traits))
			.map(|(derive_where, trait_)| generate_impl(derive_where, trait_, &item, &generics))
			.collect::<TokenStream>()
			.into(),
		Err(error) => error.into_compile_error().into(),
	}
}

/// Marker attribute signifying that this item was already processed by a
/// `derive_where` attribute before. This should prevent users to wrongly use a
/// qualified path for a `derive_where` attribute except the first one.
///
/// MSRV: This currently prevents an MSRV down to 1.34, as proc-macro derives
/// are not allowed to come before a proc-macro attribute. But the logic of this
/// proc-macro attribute is circumvented if it isn't inserted at the end, after
/// the proc-macro derive.
#[doc(hidden)]
#[proc_macro_attribute]
pub fn derive_where_visited(
	_attr: proc_macro::TokenStream,
	input: proc_macro::TokenStream,
) -> proc_macro::TokenStream {
	// No-op, just here to mark the item as visited.
	input
}

/// Generate implementation for a [`Trait`].
fn generate_impl(
	derive_where: &DeriveWhere,
	trait_: &DeriveTrait,
	item: &Item,
	generics: &SplitGenerics,
) -> TokenStream {
	let SplitGenerics {
		imp,
		ty,
		where_clause,
	} = generics;
	let mut where_clause = where_clause.map(Cow::Borrowed);
	derive_where.where_clause(&mut where_clause, trait_, item);

	let body = generate_body(derive_where, &derive_where.traits, trait_, item, generics);

	let ident = item.ident();
	let path = trait_.impl_path(trait_);
	let mut output = quote! {
		#[automatically_derived]
		impl #imp #path for #ident #ty
		#where_clause
		{
			#body
		}
	};

	if let Some((path, body)) = trait_.additional_impl(trait_) {
		output.extend(quote! {
			#[automatically_derived]
			impl #imp #path for #ident #ty
			#where_clause
			{
				#body
			}
		})
	}

	output
}

/// Generate implementation method body for a [`Trait`].
fn generate_body(
	derive_where: &DeriveWhere,
	traits: &[DeriveTrait],
	trait_: &DeriveTrait,
	item: &Item,
	generics: &SplitGenerics<'_>,
) -> TokenStream {
	let any_bound = !derive_where.generics.is_empty();

	match &item {
		Item::Item(data) => {
			let body = trait_.build_body(any_bound, traits, trait_, data);
			trait_.build_signature(any_bound, item, generics, traits, trait_, &body)
		}
		Item::Enum { variants, .. } => {
			let body: TokenStream = variants
				.iter()
				.map(|data| trait_.build_body(any_bound, traits, trait_, data))
				.collect();

			trait_.build_signature(any_bound, item, generics, traits, trait_, &body)
		}
	}
}

/// Removes `derive_where` attributes from the item and all fields and variants.
///
/// This is necessary because Rust currently does not support helper attributes
/// for attribute proc-macros and therefore doesn't automatically remove them.
fn input_without_derive_where_attributes(mut input: DeriveInput) -> DeriveInput {
	use syn::Data;

	let DeriveInput { data, attrs, .. } = &mut input;

	/// Remove all `derive_where` attributes.
	fn remove_derive_where(attrs: &mut Vec<Attribute>) {
		attrs.retain(|attr| !attr.path().is_ident(DERIVE_WHERE))
	}

	/// Remove all `derive_where` attributes from [`FieldsNamed`].
	fn remove_derive_where_from_fields_named(fields: &mut FieldsNamed) {
		let FieldsNamed { named, .. } = fields;
		named
			.iter_mut()
			.for_each(|field| remove_derive_where(&mut field.attrs))
	}

	/// Remove all `derive_where` attributes from [`Fields`].
	fn remove_derive_where_from_fields(fields: &mut Fields) {
		match fields {
			Fields::Named(fields) => remove_derive_where_from_fields_named(fields),
			Fields::Unnamed(FieldsUnnamed { unnamed, .. }) => unnamed
				.iter_mut()
				.for_each(|field| remove_derive_where(&mut field.attrs)),
			Fields::Unit => (),
		}
	}

	// Remove `derive_where` attributes from the item.
	remove_derive_where(attrs);

	// Remove `derive_where` attributes from variants or fields.
	match data {
		Data::Struct(DataStruct { fields, .. }) => remove_derive_where_from_fields(fields),
		Data::Enum(DataEnum { variants, .. }) => {
			variants
				.iter_mut()
				.for_each(|Variant { attrs, fields, .. }| {
					remove_derive_where(attrs);
					remove_derive_where_from_fields(fields)
				})
		}
		Data::Union(DataUnion { fields, .. }) => remove_derive_where_from_fields_named(fields),
	}

	input
}
