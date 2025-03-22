//! Error type.

use proc_macro2::Span;

/// Easy API to create all [`syn::Error`] messages in this crate.
pub struct Error;

impl Error {
	/// `derive_where` was already applied on this item before.
	pub fn visited(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"`#[derive_where(..)` was already applied to this item before, this occurs when using \
			 a qualified path for any `#[derive_where(..)`s except the first",
		)
	}

	/// Unnecessary `crate` option because it is equal to the default.
	pub fn path_unnecessary(span: Span, default: &str) -> syn::Error {
		syn::Error::new(
			span,
			format!(
				"unnecessary path qualification, `{}` is used by default",
				default
			),
		)
	}

	/// The `crate` option was defined together with traits.
	pub fn crate_(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"the `crate` option has to be defined in it's own `#[derive_where(..)` attribute",
		)
	}

	/// No `derive_where` with [`Trait`](crate::Trait) found.
	pub fn none(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"no traits found to implement, use `#[derive_where(..)` to specify some",
		)
	}

	/// Unsupported empty `derive_where` on item.
	pub fn empty(span: Span) -> syn::Error {
		syn::Error::new(span, "empty `derive_where` found")
	}

	/// Item has no use-case because it's covered by standard `#[derive(..)]`.
	pub fn use_case(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"this can be handled by standard `#[derive(..)]`, use a `skip` or `incomparable` \
			 attribute, implement `Default` on an enum, or different generic type parameters",
		)
	}

	/// Unsupported empty item.
	pub fn item_empty(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"derive-where doesn't support empty items, as this can already be handled by standard \
			 `#[derive(..)]`",
		)
	}

	/// Unsupported trait for union.
	pub fn union(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"traits other then `Clone` and `Copy` aren't supported by unions",
		)
	}

	/// Unsupported option in attribute.
	#[cfg(feature = "zeroize")]
	pub fn option_trait(span: Span, attribute: &str) -> syn::Error {
		syn::Error::new(span, format!("`{}` doesn't support this option", attribute))
	}

	/// Unsupported option in attribute.
	pub fn option(span: Span) -> syn::Error {
		syn::Error::new(span, "unknown option")
	}

	/// Unsupported options in attribute.
	pub fn options(span: Span, trait_: &str) -> syn::Error {
		syn::Error::new(span, format!("`{}` doesn't support any options", trait_))
	}

	/// Invalid syntax for an option in attribute.
	pub fn option_syntax(span: Span) -> syn::Error {
		syn::Error::new(span, "unexpected option syntax")
	}

	/// Unsupported empty attribute option.
	pub fn option_empty(span: Span) -> syn::Error {
		syn::Error::new(span, "empty attribute option found")
	}

	/// Missing sub-option for an option.
	#[cfg(feature = "zeroize")]
	pub fn option_required(span: Span, option: &str) -> syn::Error {
		syn::Error::new(span, format!("`{}` requires an option", option))
	}

	/// Duplicate option in attribute.
	pub fn option_duplicate(span: Span, option: &str) -> syn::Error {
		syn::Error::new(span, format!("duplicate `{}` option", option))
	}

	/// Unsupported `skip_inner` on an enum.
	pub fn option_enum_skip_inner(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"enums don't support `skip_inner`, use it on a variant instead",
		)
	}

	/// Unexpected `skip` on a field when `skip_inner` is already used on the
	/// item or variant with this [`Trait`](crate::Trait).
	pub fn option_skip_inner(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"unexpected `skip` on a field when parent already uses `skip_inner` with this trait",
		)
	}

	/// Unsupported `skip_inner` on empty variant.
	pub fn option_skip_empty(span: Span) -> syn::Error {
		syn::Error::new(span, "no fields to skip")
	}

	/// Unexpected constrained field skipping when configured to skip all traits
	/// anyway.
	pub fn option_skip_all(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"unexpected constraint on `skip` when unconstrained `skip` already used",
		)
	}

	/// Duplicate trait constraint on `skip`.
	pub fn option_skip_duplicate(span: Span, trait_: &str) -> syn::Error {
		syn::Error::new(span, format!("duplicate `{}` constraint on `skip`", trait_))
	}

	/// No trait that can be skipped is being implemented.
	pub fn option_skip_no_trait(span: Span) -> syn::Error {
		syn::Error::new(span, "no trait that can be skipped is being implemented")
	}

	/// Trait to be skipped isn't being implemented
	pub fn option_skip_trait(span: Span) -> syn::Error {
		syn::Error::new(span, "trait to be skipped isn't being implemented")
	}

	/// Unsupported [`SkipGroup`](crate::SkipGroup).
	pub fn skip_group(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			format!(
				"unsupported skip group, expected one of {}",
				Self::skip_group_list()
			),
		)
	}

	/// Invalid value for the `derive_where` or `Zeroize` `crate` option.
	pub fn path(span: Span, parse_error: syn::Error) -> syn::Error {
		syn::Error::new(span, format!("expected path, {}", parse_error))
	}

	/// Unsupported [`Trait`](crate::Trait).
	pub fn trait_(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			format!("unsupported trait, expected one of {}", Self::trait_list()),
		)
	}

	/// Invalid syntax for a [`Trait`](crate::Trait).
	pub fn trait_syntax(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			format!(
				"unsupported trait syntax, expected one of {}",
				Self::trait_list()
			),
		)
	}

	/// Invalid delimiter in `derive_where` attribute for
	/// [`Trait`](crate::Trait)s.
	pub fn derive_where_delimiter(span: Span) -> syn::Error {
		syn::Error::new(span, "expected `;` or `,")
	}

	/// Unsupported predicate type in `derive_where` attribute for where clause.
	pub fn generic(span: Span) -> syn::Error {
		syn::Error::new(span, "only type predicates are supported")
	}

	/// Invalid syntax in `derive_where` attribute for generics.
	pub fn generic_syntax(span: Span, parse_error: syn::Error) -> syn::Error {
		syn::Error::new(span, format!("expected type to bind to, {}", parse_error))
	}

	/// Duplicate trait with the same bound.
	pub fn trait_duplicate(span: Span) -> syn::Error {
		syn::Error::new(span, "duplicate trait with the same bound")
	}

	/// Unknown `repr`.
	pub fn repr_unknown(span: Span) -> syn::Error {
		syn::Error::new(span, "found unknown representation")
	}

	/// Invalid enum with non-empty variants and custom discriminants without an
	/// integer representation.
	pub fn repr_discriminant_invalid(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"enums with non-empty variants and custom discriminants require a integer \
			 representation",
		)
	}

	/// Unsupported default option if [`Default`] isn't implemented.
	pub fn default(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"`default` is only supported if `Default` is being implemented",
		)
	}

	/// Missing `default` option on a variant when [`Default`] is implemented
	/// for an enum.
	pub fn default_missing(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"required `default` option on a variant if `Default` is being implemented",
		)
	}

	/// Duplicate `default` option on a variant.
	pub fn default_duplicate(span: Span) -> syn::Error {
		syn::Error::new(span, "multiple `default` options in enum")
	}

	/// Unsupported `incomparable` option if [`PartialEq`] or [`PartialOrd`]
	/// isn't implemented.
	pub fn incomparable(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"`incomparable` is only supported if `PartialEq` or `PartialOrd` is being implemented",
		)
	}

	/// Unsupported `incomparable` option if [`Eq`] or [`Ord`] is implemented.
	pub fn non_partial_incomparable(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"`incomparable` is not supported if `Eq` or `Ord` is being implemented",
		)
	}

	/// Unsupported `incomparable` option on both enum and variant.
	pub fn incomparable_on_item_and_variant(item: Span, variant: Span) -> syn::Error {
		syn::Error::new(
			// This will only produce joint spans on nightly.
			item.join(variant).unwrap_or(item),
			"`incomparable` cannot be specified on both item and variant",
		)
	}

	/// List of available [`Trait`](crate::Trait)s.
	fn trait_list() -> String {
		[
			"Clone",
			"Copy",
			"Debug",
			"Default",
			"Eq",
			"Hash",
			"Ord",
			"PartialEq",
			"PartialOrd",
			#[cfg(feature = "zeroize")]
			"Zeroize",
			#[cfg(feature = "zeroize")]
			"ZeroizeOnDrop",
		]
		.join(", ")
	}

	/// List of available [`SkipGroup`](crate::SkipGroup)s.
	fn skip_group_list() -> String {
		[
			"Debug",
			"EqHashOrd",
			"Hash",
			#[cfg(feature = "zeroize")]
			"Zeroize",
		]
		.join(", ")
	}

	/// Unsupported `Zeroize` option if [`Zeroize`](https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html) isn't implemented.
	#[cfg(feature = "zeroize")]
	pub fn zeroize(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"`Zeroize` option is only supported if `Zeroize` is being implemented",
		)
	}

	/// Deprecated use of `Zeroize(drop)`.
	#[cfg(feature = "zeroize")]
	pub fn deprecated_zeroize_drop(span: Span) -> syn::Error {
		syn::Error::new(
			span,
			"`Zeroize(drop)` is deprecated, use `ZeroizeOnDrop` instead",
		)
	}
}
