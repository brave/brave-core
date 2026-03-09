//! [`PartialEq`](trait@std::cmp::PartialEq) implementation.

use proc_macro2::TokenStream;
use quote::quote;

use super::common_ord::build_incomparable_pattern;
use crate::{Data, DeriveTrait, Item, SimpleType, SplitGenerics, TraitImpl};

/// Dummy-struct implement [`Trait`](crate::Trait) for
/// [`PartialEq`](trait@std::cmp::PartialEq).
pub struct PartialEq;

impl TraitImpl for PartialEq {
	fn as_str(&self) -> &'static str {
		"PartialEq"
	}

	fn default_derive_trait(&self) -> DeriveTrait {
		DeriveTrait::PartialEq
	}

	fn build_signature(
		&self,
		_any_bound: bool,
		item: &Item,
		_generics: &SplitGenerics<'_>,
		_traits: &[DeriveTrait],
		trait_: &DeriveTrait,
		body: &TokenStream,
	) -> TokenStream {
		let body = {
			match item {
				// If the whole item is incomparable return false
				item if item.is_incomparable() => {
					quote! { false }
				}
				// If there is more than one variant and not all variants are empty, check for
				// discriminant and match on variant data.
				Item::Enum { variants, .. } if variants.len() > 1 && !item.is_empty(**trait_) => {
					// Return `true` in the rest pattern if there are any empty variants
					// that are not incomparable.
					let rest = if variants
						.iter()
						.any(|variant| variant.is_empty(**trait_) && !variant.is_incomparable())
					{
						quote! { true }
					} else {
						#[cfg(not(feature = "safe"))]
						// This follows the standard implementation.
						quote! { unsafe { ::core::hint::unreachable_unchecked() } }
						#[cfg(feature = "safe")]
						quote! { ::core::unreachable!("comparing variants yielded unexpected results") }
					};

					// Return `false` for all incomparable variants
					let incomparable = build_incomparable_pattern(variants).into_iter();

					quote! {
						if ::core::mem::discriminant(self) == ::core::mem::discriminant(__other) {
							match (self, __other) {
								#body
								#((#incomparable, ..) => false,)*
								_ => #rest,
							}
						} else {
							false
						}
					}
				}
				// If there is more than one variant and all are empty, check for
				// discriminant and simply return `true` if it is not incomparable.
				Item::Enum { variants, .. } if variants.len() > 1 && item.is_empty(**trait_) => {
					let incomparable = build_incomparable_pattern(variants).into_iter();
					quote! {
						if ::core::mem::discriminant(self) == ::core::mem::discriminant(__other) {
							#(if ::core::matches!(self, #incomparable) {
								return false;
							})*
							true
						} else {
							false
						}
					}
				}
				// If there is only one variant and it's empty or if the struct is empty, simply
				// return `true`.
				item if item.is_empty(**trait_) => {
					quote! { true }
				}
				_ => {
					quote! {
						match (self, __other) {
							#body
						}
					}
				}
			}
		};

		quote! {
			#[inline]
			fn eq(&self, __other: &Self) -> bool {
				#body
			}
		}
	}

	fn build_body(
		&self,
		_any_bound: bool,
		_traits: &[DeriveTrait],
		trait_: &DeriveTrait,
		data: &Data,
	) -> TokenStream {
		if data.is_empty(**trait_) || data.is_incomparable() {
			TokenStream::new()
		} else {
			match data.simple_type() {
				SimpleType::Struct(fields) | SimpleType::Tuple(fields) => {
					let self_pattern = &fields.self_pattern;
					let other_pattern = &fields.other_pattern;
					let trait_path = trait_.path();
					let self_ident = data.iter_self_ident(**trait_);
					let other_ident = data.iter_other_ident(**trait_);

					quote! {
						(#self_pattern, #other_pattern) =>
							true #(&& #trait_path::eq(#self_ident, #other_ident))*,
					}
				}
				SimpleType::Unit(_) => TokenStream::new(),
				SimpleType::Union(_) => unreachable!("unexpected trait for union"),
			}
		}
	}
}
