//! [`Copy`](trait@std::marker::Copy) implementation.

use crate::{DeriveTrait, TraitImpl};

/// Dummy-struct implement [`Trait`](crate::Trait) for
/// [`Copy`](trait@std::marker::Copy).
pub struct Copy;

impl TraitImpl for Copy {
	fn as_str(&self) -> &'static str {
		"Copy"
	}

	fn default_derive_trait(&self) -> DeriveTrait {
		DeriveTrait::Copy
	}

	fn supports_union(&self) -> bool {
		true
	}
}
