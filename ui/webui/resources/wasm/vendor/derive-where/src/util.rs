//! Utility functions.

use proc_macro2::Span;
use syn::{
	punctuated::Punctuated, spanned::Spanned, Ident, Meta, MetaList, Path, PathArguments,
	PathSegment, Result, Token,
};

use crate::error::Error;

/// Convenience type to return two possible values.
pub enum Either<L, R> {
	/// `L` return value.
	Left(L),
	/// `R` return value.
	Right(R),
}

/// Create [`PathSegment`] from [`str`].
pub fn path_segment(ident: &str) -> PathSegment {
	PathSegment {
		ident: Ident::new(ident, Span::call_site()),
		arguments: PathArguments::None,
	}
}

/// Create [`Path`] from `[&str]`s.
pub fn path_from_strs(segments: &[&str]) -> Path {
	Path {
		leading_colon: Some(<Token![::]>::default()),
		segments: Punctuated::from_iter(segments.iter().map(|segment| path_segment(segment))),
	}
}

/// Create [`Path`] from `[&Ident]`s.
pub fn path_from_idents(segments: &[&Ident]) -> Path {
	Path {
		leading_colon: None,
		segments: Punctuated::from_iter(segments.iter().map(|ident| PathSegment {
			ident: (*ident).clone(),
			arguments: PathArguments::None,
		})),
	}
}

/// Create [`Path`] from a root [`Path`] and `[&str]`s.
pub fn path_from_root_and_strs(root: Path, segments: &[&str]) -> Path {
	Path {
		leading_colon: root.leading_colon,
		segments: root
			.segments
			.into_iter()
			.chain(segments.iter().map(|segment| path_segment(segment)))
			.collect(),
	}
}

/// Extension for [`MetaList`].
pub trait MetaListExt {
	/// Shorthand for parsing a [`MetaList`] into a list of [`Meta`]s.
	fn parse_non_empty_nested_metas(&self) -> Result<Punctuated<Meta, Token![,]>>;
}

impl MetaListExt for MetaList {
	fn parse_non_empty_nested_metas(&self) -> Result<Punctuated<Meta, Token![,]>> {
		let list = self.parse_args_with(Punctuated::<Meta, Token![,]>::parse_terminated)?;

		if list.is_empty() {
			return Err(Error::option_empty(self.span()));
		}

		Ok(list)
	}
}
