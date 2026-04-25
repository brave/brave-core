//! [`PartialOrd`](trait@std::cmp::PartialOrd) implementation.

use proc_macro2::TokenStream;
use quote::quote;

use super::common_ord;
use crate::{Data, DeriveTrait, Item, SimpleType, SplitGenerics, Trait, TraitImpl};

/// Dummy-struct implement [`Trait`] for
/// [`PartialOrd`](trait@std::cmp::PartialOrd).
pub struct PartialOrd;

impl TraitImpl for PartialOrd {
	fn as_str(&self) -> &'static str {
		"PartialOrd"
	}

	fn default_derive_trait(&self) -> DeriveTrait {
		DeriveTrait::PartialOrd
	}

	fn build_signature(
		&self,
		any_bound: bool,
		item: &Item,
		generics: &SplitGenerics<'_>,
		traits: &[DeriveTrait],
		trait_: &DeriveTrait,
		body: &TokenStream,
	) -> TokenStream {
		let body = if !any_bound && traits.iter().any(|trait_| trait_ == Trait::Ord) {
			quote! {
				::core::option::Option::Some(::core::cmp::Ord::cmp(self, __other))
			}
		} else {
			common_ord::build_ord_signature(item, generics, traits, trait_, body)
		};

		quote! {
			#[inline]
			fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
				#body
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
		if data.is_empty(**trait_)
			|| data.is_incomparable()
			|| (!any_bound && traits.iter().any(|trait_| trait_ == Trait::Ord))
		{
			TokenStream::new()
		} else {
			match data.simple_type() {
				SimpleType::Struct(fields) | SimpleType::Tuple(fields) => {
					let self_pattern = &fields.self_pattern;
					let other_pattern = &fields.other_pattern;
					let body = common_ord::build_ord_body(trait_, data);

					quote! {
						(#self_pattern, #other_pattern) => #body,
					}
				}
				SimpleType::Unit(_) => TokenStream::new(),
				SimpleType::Union(_) => unreachable!("unexpected trait for union"),
			}
		}
	}
}
