//! [`ZeroizeOnDrop`](https://docs.rs/zeroize/latest/zeroize/trait.ZeroizeOnDrop.html) implementation.

use proc_macro2::{Span, TokenStream};
use quote::quote;
use syn::{
	punctuated::Punctuated, spanned::Spanned, Expr, ExprLit, ExprPath, Lit, Meta, Path, Result,
	Token,
};

use crate::{util, Data, DeriveTrait, Error, Item, SimpleType, SplitGenerics, TraitImpl};

/// Dummy-struct implement [`Trait`](crate::Trait) for [`ZeroizeOnDrop`](https://docs.rs/zeroize/latest/zeroize/trait.ZeroizeOnDrop.html) .
pub struct ZeroizeOnDrop;

impl TraitImpl for ZeroizeOnDrop {
	fn as_str(&self) -> &'static str {
		"ZeroizeOnDrop"
	}

	fn default_derive_trait(&self) -> DeriveTrait {
		DeriveTrait::ZeroizeOnDrop { crate_: None }
	}

	fn parse_derive_trait(
		&self,
		_span: Span,
		list: Punctuated<Meta, Token![,]>,
	) -> Result<DeriveTrait> {
		// This is already checked in `DeriveTrait::from_stream`.
		debug_assert!(!list.is_empty());

		let mut crate_ = None;

		for meta in list {
			match &meta {
				Meta::Path(path) => return Err(Error::option_trait(path.span(), self.as_str())),
				Meta::NameValue(name_value) => {
					if name_value.path.is_ident("crate") {
						// Check for duplicate `crate` option.
						if crate_.is_none() {
							let path = match &name_value.value {
								Expr::Lit(ExprLit {
									lit: Lit::Str(lit_str),
									..
								}) => match lit_str.parse::<Path>() {
									Ok(path) => path,
									Err(error) => return Err(Error::path(lit_str.span(), error)),
								},
								Expr::Path(ExprPath { path, .. }) => path.clone(),
								_ => return Err(Error::option_syntax(name_value.value.span())),
							};

							if path == util::path_from_strs(&["zeroize"]) {
								return Err(Error::path_unnecessary(path.span(), "::zeroize"));
							}

							crate_ = Some(path);
						} else {
							return Err(Error::option_duplicate(name_value.span(), "crate"));
						}
					} else {
						return Err(Error::option_trait(name_value.path.span(), self.as_str()));
					}
				}
				_ => {
					return Err(Error::option_syntax(meta.span()));
				}
			}
		}

		Ok(DeriveTrait::ZeroizeOnDrop { crate_ })
	}

	#[allow(unused_variables)]
	fn additional_impl(&self, trait_: &DeriveTrait) -> Option<(Path, TokenStream)> {
		#[cfg(feature = "zeroize-on-drop")]
		return Some((trait_.path(), quote! {}));
		#[cfg(not(feature = "zeroize-on-drop"))]
		None
	}

	fn impl_path(&self, _trait_: &DeriveTrait) -> Path {
		util::path_from_strs(&["core", "ops", "Drop"])
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
		match item {
			Item::Item(data) if data.is_empty(**trait_) => quote! {
				fn drop(&mut self) { }
			},
			_ => {
				#[cfg(feature = "zeroize-on-drop")]
				{
					let crate_ = trait_.crate_();
					let internal = util::path_segment("__internal");

					let mut assert_zeroize = crate_.clone();
					assert_zeroize
						.segments
						.extend([internal.clone(), util::path_segment("AssertZeroize")]);

					let mut assert_zeroize_on_drop = crate_;
					assert_zeroize_on_drop
						.segments
						.extend([internal, util::path_segment("AssertZeroizeOnDrop")]);

					quote! {
						fn drop(&mut self) {
							use #assert_zeroize;
							use #assert_zeroize_on_drop;

							match self {
								#body
							}
						}
					}
				}
				#[cfg(not(feature = "zeroize-on-drop"))]
				quote! {
					fn drop(&mut self) {
						#body
					}
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
		if data.is_empty(**trait_) {
			TokenStream::new()
		} else {
			match data.simple_type() {
				SimpleType::Struct(fields) | SimpleType::Tuple(fields) => {
					#[cfg(feature = "zeroize-on-drop")]
					{
						let self_pattern = fields.self_pattern_mut();
						let self_ident = data.iter_self_ident(**trait_);

						quote! {
							#self_pattern => {
								#(#self_ident.zeroize_or_on_drop();)*
							}
						}
					}
					#[cfg(not(feature = "zeroize-on-drop"))]
					{
						// Use unused variables.
						let _ = fields;

						let path = util::path_from_root_and_strs(trait_.crate_(), &["Zeroize"]);

						quote! {
							#path::zeroize(self);
						}
					}
				}
				SimpleType::Unit(_) => TokenStream::new(),
				SimpleType::Union(_) => unreachable!("unexpected trait for union"),
			}
		}
	}
}
