//! [`Default`](trait@std::default::Default) implementation.

use proc_macro2::TokenStream;
use quote::quote;

use crate::{Data, DeriveTrait, Item, SimpleType, SplitGenerics, TraitImpl};

/// Dummy-struct implement [`Trait`](crate::Trait) for
/// [`Default`](trait@std::default::Default).
pub struct Default;

impl TraitImpl for Default {
	fn as_str(&self) -> &'static str {
		"Default"
	}

	fn default_derive_trait(&self) -> DeriveTrait {
		DeriveTrait::Default
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
			fn default() -> Self {
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
		if data.is_default() {
			let path = &data.path;

			match data.simple_type() {
				SimpleType::Struct(_) => {
					let fields = data.iter_field_ident(**trait_);
					let trait_path = trait_.path();

					quote! { #path { #(#fields: #trait_path::default()),* } }
				}
				SimpleType::Tuple(_) => {
					let trait_path = trait_.path();
					let fields = data
						.iter_fields(**trait_)
						.map(|_| quote! { #trait_path::default() });

					quote! { #path(#(#fields),*) }
				}
				SimpleType::Unit(_) => {
					quote! { #path }
				}
				SimpleType::Union(_) => unreachable!("unexpected trait for union"),
			}
		}
		// Skip `Default` implementation if variant isn't marked with a `default` attribute.
		else {
			TokenStream::new()
		}
	}
}
