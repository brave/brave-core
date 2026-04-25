//! [`Clone`](trait@std::clone::Clone) implementation.

use proc_macro2::TokenStream;
use quote::quote;
use syn::{TraitBound, TraitBoundModifier, TypeParamBound};

use crate::{Data, DataType, DeriveTrait, Item, SimpleType, SplitGenerics, Trait, TraitImpl};

/// Dummy-struct implement [`Trait`] for [`Clone`](trait@std::clone::Clone).
pub struct Clone;

impl TraitImpl for Clone {
	fn as_str(&self) -> &'static str {
		"Clone"
	}

	fn default_derive_trait(&self) -> DeriveTrait {
		DeriveTrait::Clone
	}

	fn supports_union(&self) -> bool {
		true
	}

	fn additional_where_bounds(&self, data: &Item) -> Option<TypeParamBound> {
		// `Clone` for unions requires the `Copy` bound.
		if let Item::Item(Data {
			type_: DataType::Union(..),
			..
		}) = data
		{
			Some(TypeParamBound::Trait(TraitBound {
				paren_token: None,
				modifier: TraitBoundModifier::None,
				lifetimes: None,
				path: Trait::Copy.default_derive_trait().path(),
			}))
		} else {
			None
		}
	}

	fn build_signature(
		&self,
		any_bound: bool,
		item: &Item,
		_generics: &SplitGenerics<'_>,
		traits: &[DeriveTrait],
		_trait_: &DeriveTrait,
		body: &TokenStream,
	) -> TokenStream {
		// Special implementation for items also implementing `Copy`.
		if !any_bound && traits.iter().any(|trait_| trait_ == Trait::Copy) {
			return quote! {
				#[inline]
				fn clone(&self) -> Self { *self }
			};
		}

		// Special implementation for unions.
		if let Item::Item(Data {
			type_: DataType::Union(..),
			..
		}) = item
		{
			quote! {
				#[inline]
				fn clone(&self) -> Self {
					struct __AssertCopy<__T: ::core::marker::Copy + ?::core::marker::Sized>(::core::marker::PhantomData<__T>);
					let _: __AssertCopy<Self>;
					*self
				}
			}
		} else {
			quote! {
				#[inline]
				fn clone(&self) -> Self {
					match self {
						#body
					}
				}
			}
		}
	}

	fn build_body(
		&self,
		any_bound: bool,
		traits: &[DeriveTrait],
		trait_: &DeriveTrait,
		data: &Data,
	) -> TokenStream {
		if !any_bound && traits.iter().any(|trait_| trait_ == Trait::Copy) {
			return TokenStream::new();
		}

		match data.simple_type() {
			SimpleType::Struct(fields) => {
				let self_pattern = &fields.self_pattern;
				let item_path = &data.path;
				let self_ident = data.iter_self_ident(**trait_);
				let fields = data.iter_field_ident(**trait_);
				let trait_path = trait_.path();

				quote! {
					#self_pattern => #item_path { #(#fields: #trait_path::clone(#self_ident)),* },
				}
			}
			SimpleType::Tuple(fields) => {
				let self_pattern = &fields.self_pattern;
				let item_path = &data.path;
				let self_ident = data.iter_self_ident(**trait_);
				let trait_path = trait_.path();

				quote! {
					#self_pattern => #item_path(#(#trait_path::clone(#self_ident)),*),
				}
			}
			SimpleType::Unit(pattern) => {
				quote! { #pattern => #pattern, }
			}
			SimpleType::Union(_) => TokenStream::new(),
		}
	}
}
