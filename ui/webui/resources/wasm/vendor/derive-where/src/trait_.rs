//! Individual implementation for all traits.

mod clone;
mod common_ord;
mod copy;
mod debug;
mod default;
mod eq;
mod hash;
mod ord;
mod partial_eq;
mod partial_ord;
#[cfg(feature = "zeroize")]
mod zeroize;
#[cfg(feature = "zeroize")]
mod zeroize_on_drop;

use proc_macro2::{Span, TokenStream};
use syn::{punctuated::Punctuated, spanned::Spanned, Meta, Path, Result, Token, TypeParamBound};

use crate::{Data, DeriveTrait, Error, Item, SplitGenerics};

/// Type implementing [`TraitImpl`] for every trait.
#[derive(Clone, Copy, Eq, PartialEq)]
#[cfg_attr(test, derive(Debug))]
pub enum Trait {
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
	Zeroize,
	/// [`ZeroizeOnDrop`](https://docs.rs/zeroize/latest/zeroize/trait.ZeroizeOnDrop.html).
	#[cfg(feature = "zeroize")]
	ZeroizeOnDrop,
}

impl Trait {
	/// Return dummy-struct for the internal implementation.
	fn implementation(&self) -> &dyn TraitImpl {
		match self {
			Trait::Clone => &clone::Clone,
			Trait::Copy => &copy::Copy,
			Trait::Debug => &debug::Debug,
			Trait::Default => &default::Default,
			Trait::Eq => &eq::Eq,
			Trait::Hash => &hash::Hash,
			Trait::Ord => &ord::Ord,
			Trait::PartialEq => &partial_eq::PartialEq,
			Trait::PartialOrd => &partial_ord::PartialOrd,
			#[cfg(feature = "zeroize")]
			Trait::Zeroize => &zeroize::Zeroize,
			#[cfg(feature = "zeroize")]
			Trait::ZeroizeOnDrop => &zeroize_on_drop::ZeroizeOnDrop,
		}
	}

	/// Create [`Trait`] from [`Path`].
	pub fn from_path(path: &Path) -> Result<Self> {
		if let Some(ident) = path.get_ident() {
			use Trait::*;

			match ident.to_string().as_str() {
				"Clone" => Ok(Clone),
				"Copy" => Ok(Copy),
				"Debug" => Ok(Debug),
				"Default" => Ok(Default),
				"Eq" => Ok(Eq),
				"Hash" => Ok(Hash),
				"Ord" => Ok(Ord),
				"PartialEq" => Ok(PartialEq),
				"PartialOrd" => Ok(PartialOrd),
				#[cfg(feature = "zeroize")]
				"Zeroize" => Ok(Zeroize),
				#[cfg(feature = "zeroize")]
				"ZeroizeOnDrop" => Ok(ZeroizeOnDrop),
				"crate" => Err(Error::crate_(path.span())),
				_ => Err(Error::trait_(path.span())),
			}
		} else {
			Err(Error::trait_(path.span()))
		}
	}
}

impl TraitImpl for Trait {
	fn as_str(&self) -> &'static str {
		self.implementation().as_str()
	}

	fn default_derive_trait(&self) -> DeriveTrait {
		self.implementation().default_derive_trait()
	}

	fn parse_derive_trait(
		&self,
		span: Span,
		list: Punctuated<Meta, Token![,]>,
	) -> Result<DeriveTrait> {
		self.implementation().parse_derive_trait(span, list)
	}

	fn supports_union(&self) -> bool {
		self.implementation().supports_union()
	}

	fn additional_where_bounds(&self, data: &Item) -> Option<TypeParamBound> {
		self.implementation().additional_where_bounds(data)
	}

	fn additional_impl(&self, trait_: &DeriveTrait) -> Option<(Path, TokenStream)> {
		self.implementation().additional_impl(trait_)
	}

	fn impl_path(&self, trait_: &DeriveTrait) -> Path {
		self.implementation().impl_path(trait_)
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
		self.implementation()
			.build_signature(any_bound, item, generics, traits, trait_, body)
	}

	fn build_body(
		&self,
		any_bound: bool,
		traits: &[DeriveTrait],
		trait_: &DeriveTrait,
		data: &Data,
	) -> TokenStream {
		self.implementation()
			.build_body(any_bound, traits, trait_, data)
	}
}

/// Single trait implementation. Parses attributes and constructs `impl`s.
pub trait TraitImpl {
	/// [`str`] representation of this [`Trait`].
	/// Used to compare against [`Ident`](struct@syn::Ident)s and create error
	/// messages.
	fn as_str(&self) -> &'static str;

	/// Associated [`DeriveTrait`].
	fn default_derive_trait(&self) -> DeriveTrait;

	/// Parse a `derive_where` trait with it's options.
	fn parse_derive_trait(
		&self,
		span: Span,
		_list: Punctuated<Meta, Token![,]>,
	) -> Result<DeriveTrait> {
		Err(Error::options(span, self.as_str()))
	}

	/// Returns `true` if [`Trait`] supports unions.
	fn supports_union(&self) -> bool {
		false
	}

	/// Additional bounds to add to [`WhereClause`](syn::WhereClause).
	fn additional_where_bounds(&self, _data: &Item) -> Option<TypeParamBound> {
		None
	}

	/// Additional implementation to add for this [`Trait`].
	fn additional_impl(&self, _trait_: &DeriveTrait) -> Option<(Path, TokenStream)> {
		None
	}

	/// Trait to implement. Only used for [`ZeroizeOnDrop`](https://docs.rs/zeroize/latest/zeroize/trait.ZeroizeOnDrop.html)
	/// because it implements [`Drop`] and not itself.
	fn impl_path(&self, trait_: &DeriveTrait) -> Path {
		trait_.path()
	}

	/// Build method signature for this [`Trait`].
	fn build_signature(
		&self,
		_any_bound: bool,
		_item: &Item,
		_generics: &SplitGenerics<'_>,
		_traits: &[DeriveTrait],
		_trait_: &DeriveTrait,
		_body: &TokenStream,
	) -> TokenStream {
		TokenStream::new()
	}

	/// Build method body for this [`Trait`].
	fn build_body(
		&self,
		_any_bound: bool,
		_traits: &[DeriveTrait],
		_trait_: &DeriveTrait,
		_data: &Data,
	) -> TokenStream {
		TokenStream::new()
	}
}
