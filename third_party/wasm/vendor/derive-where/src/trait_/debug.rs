//! [`Debug`](trait@std::fmt::Debug) implementation.

use proc_macro2::TokenStream;
use quote::quote;

use crate::{Data, DeriveTrait, Item, SimpleType, SplitGenerics, TraitImpl};

/// Dummy-struct implement [`Trait`](crate::Trait) for
/// [`Debug`](trait@std::fmt::Debug).
pub struct Debug;

impl TraitImpl for Debug {
	fn as_str(&self) -> &'static str {
		"Debug"
	}

	fn default_derive_trait(&self) -> DeriveTrait {
		DeriveTrait::Debug
	}

	fn build_signature(
		&self,
		_any_bound: bool,
		_item: &Item,
		_generics: &SplitGenerics<'_>,
		_traits: &[DeriveTrait],
		_trait_: &DeriveTrait,
		body: &TokenStream,
	) -> TokenStream {
		quote! {
			fn fmt(&self, __f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
				match self {
					#body
				}
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
		let self_pattern = &data.self_pattern();
		let debug_name = data.ident.to_string();

		match data.simple_type() {
			SimpleType::Struct(_) => {
				let self_ident = data.iter_self_ident(**trait_);
				let debug_fields = data
					.iter_field_ident(**trait_)
					.map(|field| field.to_string());

				let finish = if data.any_skip_trait(**trait_) {
					quote! { finish_non_exhaustive }
				} else {
					quote! { finish }
				};

				quote! {
					#self_pattern => {
						let mut __builder = ::core::fmt::Formatter::debug_struct(__f, #debug_name);
						#(::core::fmt::DebugStruct::field(&mut __builder, #debug_fields, #self_ident);)*
						::core::fmt::DebugStruct::#finish(&mut __builder)
					}
				}
			}
			SimpleType::Tuple(_) => {
				let self_ident = data.iter_self_ident(**trait_);

				quote! {
					#self_pattern => {
						let mut __builder = ::core::fmt::Formatter::debug_tuple(__f, #debug_name);
						#(::core::fmt::DebugTuple::field(&mut __builder, #self_ident);)*
						::core::fmt::DebugTuple::finish(&mut __builder)
					}
				}
			}
			SimpleType::Unit(_) => {
				quote! { #self_pattern => ::core::fmt::Formatter::write_str(__f, #debug_name), }
			}
			SimpleType::Union(_) => unreachable!("unexpected trait for union"),
		}
	}
}
