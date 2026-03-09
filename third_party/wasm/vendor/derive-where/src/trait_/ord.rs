//! [`Ord`](trait@std::cmp::Ord) implementation.

use proc_macro2::TokenStream;
use quote::quote;

use super::common_ord;
use crate::{Data, DeriveTrait, Item, SimpleType, SplitGenerics, TraitImpl};

/// Dummy-struct implement [`Trait`](crate::Trait) for
/// [`Ord`](trait@std::cmp::Ord).
pub struct Ord;

impl TraitImpl for Ord {
	fn as_str(&self) -> &'static str {
		"Ord"
	}

	fn default_derive_trait(&self) -> DeriveTrait {
		DeriveTrait::Ord
	}

	fn build_signature(
		&self,
		_any_bound: bool,
		item: &Item,
		generics: &SplitGenerics<'_>,
		traits: &[DeriveTrait],
		trait_: &DeriveTrait,
		body: &TokenStream,
	) -> TokenStream {
		let body = common_ord::build_ord_signature(item, generics, traits, trait_, body);

		quote! {
			#[inline]
			fn cmp(&self, __other: &Self) -> ::core::cmp::Ordering {
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
		if data.is_empty(**trait_) {
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
