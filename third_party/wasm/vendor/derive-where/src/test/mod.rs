mod basic;
mod bound;
mod discriminant;
mod enum_;
#[cfg(not(any(feature = "nightly", feature = "safe")))]
mod incomparable;
mod misc;
mod partial_ord;
mod skip;
mod use_case;
#[cfg(feature = "zeroize")]
mod zeroize;

use std::iter;

use pretty_assertions::assert_eq;
use proc_macro2::TokenStream;
use quote::quote;
use syn::{spanned::Spanned, DeriveInput, Result};

use crate::{generate_impl, input::Input};

fn test_derive(input: TokenStream, expected: TokenStream) -> Result<()> {
	let left = derive_where_internal(input)?.to_string();
	let right = quote! {#expected}.to_string();

	assert_eq!(left, right);
	Ok(())
}

fn compiles(input: TokenStream) -> Result<()> {
	derive_where_internal(input).map(|_| ())
}

/// Internal derive function for testing
fn derive_where_internal(input: TokenStream) -> Result<TokenStream> {
	// Save `Span` before we consume `input` when parsing it.
	let span = input.span();
	let item = syn::parse2::<DeriveInput>(input).expect("derive on unparsable item");

	let Input {
		derive_wheres,
		generics,
		item,
		..
	} = Input::from_input(span, &item)?;

	Ok(derive_wheres
		.iter()
		.flat_map(|derive_where| iter::repeat(derive_where).zip(&derive_where.traits))
		.map(|(derive_where, trait_)| generate_impl(derive_where, trait_, &item, &generics))
		.collect())
}
