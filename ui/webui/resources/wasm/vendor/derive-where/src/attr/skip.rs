//! Attribute parsing for the `skip` and `skip_inner` options.

use std::default::Default;

use syn::{spanned::Spanned, Meta, Path, Result};

use crate::{util::MetaListExt, DeriveWhere, Error, Trait};

/// Stores what [`Trait`]s to skip this field or variant for.
#[cfg_attr(test, derive(Debug))]
pub enum Skip {
	/// Field skipped for no [`Trait`].
	None,
	/// Field skipped for all [`Trait`]s that support it.
	All,
	/// Field skipped for the [`Trait`]s listed.
	Traits(Vec<SkipGroup>),
}

impl Default for Skip {
	fn default() -> Self {
		Skip::None
	}
}

impl Skip {
	/// Token used for the `skip` option.
	pub const SKIP: &'static str = "skip";
	/// Token used for the `skip_inner` option.
	pub const SKIP_INNER: &'static str = "skip_inner";

	/// Returns `true` if variant is [`Skip::None`].
	pub fn is_none(&self) -> bool {
		matches!(self, Skip::None)
	}

	/// Adds a [`Meta`] to this [`Skip`].
	pub fn add_attribute(
		&mut self,
		derive_wheres: &[DeriveWhere],
		skip_inner: Option<&Skip>,
		meta: &Meta,
	) -> Result<()> {
		debug_assert!(meta.path().is_ident(Self::SKIP) || meta.path().is_ident(Self::SKIP_INNER));

		match meta {
			Meta::Path(path) => {
				// Check for duplicates.
				if self.is_none() {
					// Check against parent `skip_inner`.
					match skip_inner {
						// Allow `Skip::All` on field if parent has a tighter constraint.
						Some(Skip::None) | Some(Skip::Traits(..)) | None => {
							// Don't allow to skip all traits if no trait to be implemented supports
							// skipping.
							if derive_wheres
								.iter()
								.any(|derive_where| derive_where.any_skip())
							{
								*self = Skip::All;
								Ok(())
							} else {
								Err(Error::option_skip_no_trait(path.span()))
							}
						}
						// Don't allow `Skip::All` on field if parent already covers it.
						Some(Skip::All) => Err(Error::option_skip_inner(path.span())),
					}
				} else {
					Err(Error::option_duplicate(
						path.span(),
						&meta
							.path()
							.get_ident()
							.expect("unexpected skip syntax")
							.to_string(),
					))
				}
			}
			Meta::List(list) => {
				let nested = list.parse_non_empty_nested_metas()?;

				// Get traits already set to be skipped.
				let traits = match self {
					// If no traits are set, change to empty `Skip::Traits` and return that.
					Skip::None => {
						*self = Skip::Traits(Vec::new());

						if let Skip::Traits(traits) = self {
							traits
						} else {
							unreachable!("unexpected variant")
						}
					}
					// If we are already skipping all traits, we can't skip again with constraints.
					Skip::All => return Err(Error::option_skip_all(list.span())),
					Skip::Traits(traits) => traits,
				};

				for nested_meta in &nested {
					if let Meta::Path(path) = nested_meta {
						let skip_group = SkipGroup::from_path(path)?;

						// Don't allow to skip the same trait twice.
						if traits.contains(&skip_group) {
							return Err(Error::option_skip_duplicate(
								path.span(),
								skip_group.as_str(),
							));
						} else {
							// Don't allow to skip a trait already set to be skipped in the
							// parent.
							match skip_inner {
								Some(skip_inner) if skip_inner.group_skipped(skip_group) => {
									return Err(Error::option_skip_inner(path.span()))
								}
								_ => {
									// Don't allow to skip trait that isn't being implemented.
									if derive_wheres.iter().any(|derive_where| {
										skip_group
											.traits()
											.any(|trait_| derive_where.contains(trait_))
									}) {
										traits.push(skip_group)
									} else {
										return Err(Error::option_skip_trait(path.span()));
									}
								}
							}
						}
					} else {
						return Err(Error::option_syntax(nested_meta.span()));
					}
				}

				Ok(())
			}
			_ => Err(Error::option_syntax(meta.span())),
		}
	}

	/// Returns `true` if this item, variant or field is skipped with the given
	/// [`Trait`].
	pub fn trait_skipped(&self, trait_: Trait) -> bool {
		match self {
			Skip::None => false,
			Skip::All => SkipGroup::trait_supported(trait_),
			Skip::Traits(skip_groups) => skip_groups
				.iter()
				.any(|skip_group| skip_group.traits().any(|this_trait| this_trait == trait_)),
		}
	}

	/// Returns `true` if this item, variant or field is skipped with the given
	/// [`SkipGroup`].
	pub fn group_skipped(&self, group: SkipGroup) -> bool {
		match self {
			Skip::None => false,
			Skip::All => true,
			Skip::Traits(groups) => groups.iter().any(|this_group| *this_group == group),
		}
	}
}

/// Available groups of [`Trait`]s to skip.
#[derive(Clone, Copy, Eq, PartialEq)]
#[cfg_attr(test, derive(Debug))]
pub enum SkipGroup {
	/// [`Debug`].
	Debug,
	/// [`Eq`], [`Hash`], [`Ord`], [`PartialEq`] and [`PartialOrd`].
	EqHashOrd,
	/// [`Hash`].
	Hash,
	/// [`Zeroize`](https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html) and
	/// [`ZeroizeOnDrop`](https://docs.rs/zeroize/latest/zeroize/trait.ZeroizeOnDrop.html).
	#[cfg(feature = "zeroize")]
	Zeroize,
}

impl SkipGroup {
	/// Create [`SkipGroup`] from [`Path`].
	fn from_path(path: &Path) -> Result<Self> {
		if let Some(ident) = path.get_ident() {
			use SkipGroup::*;

			match ident.to_string().as_str() {
				"Debug" => Ok(Debug),
				"EqHashOrd" => Ok(EqHashOrd),
				"Hash" => Ok(Hash),
				#[cfg(feature = "zeroize")]
				"Zeroize" => Ok(Zeroize),
				_ => Err(Error::skip_group(path.span())),
			}
		} else {
			Err(Error::skip_group(path.span()))
		}
	}

	/// [`str`] representation of this [`Trait`].
	/// Used to compare against [`Ident`](struct@syn::Ident)s and create error
	/// messages.
	const fn as_str(self) -> &'static str {
		match self {
			Self::Debug => "Debug",
			Self::EqHashOrd => "EqHashOrd",
			Self::Hash => "Hash",
			#[cfg(feature = "zeroize")]
			Self::Zeroize => "Zeroize",
		}
	}

	/// [`Trait`]s supported by this group.
	fn traits(self) -> impl Iterator<Item = Trait> {
		match self {
			Self::Debug => [Some(Trait::Debug), None, None, None, None]
				.into_iter()
				.flatten(),
			Self::EqHashOrd => [
				Some(Trait::Eq),
				Some(Trait::Hash),
				Some(Trait::Ord),
				Some(Trait::PartialEq),
				Some(Trait::PartialOrd),
			]
			.into_iter()
			.flatten(),
			Self::Hash => [Some(Trait::Hash), None, None, None, None]
				.into_iter()
				.flatten(),
			#[cfg(feature = "zeroize")]
			Self::Zeroize => [
				Some(Trait::Zeroize),
				Some(Trait::ZeroizeOnDrop),
				None,
				None,
				None,
			]
			.into_iter()
			.flatten(),
		}
	}

	/// Returns `true` if [`Trait`] is supported by any group.
	pub fn trait_supported(trait_: Trait) -> bool {
		match trait_ {
			Trait::Clone | Trait::Copy | Trait::Default => false,
			Trait::Debug
			| Trait::Eq
			| Trait::Hash
			| Trait::Ord
			| Trait::PartialEq
			| Trait::PartialOrd => true,
			#[cfg(feature = "zeroize")]
			Trait::Zeroize | Trait::ZeroizeOnDrop => true,
		}
	}
}
