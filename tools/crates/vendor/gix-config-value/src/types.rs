use crate::{color, integer};

/// Any value that may contain a foreground color, background color, a
/// collection of color (text) modifiers, or a combination of any of the
/// aforementioned values, like `red` or `brightgreen`.
///
/// Note that `git-config` allows color values to simply be a collection of
/// [`color::Attribute`]s, and does not require a [`color::Name`] for either the
/// foreground or background color.
/// Conversion errors expose invalid `input` bytes as [metadata](gix_error::Error::metadata()).
#[derive(Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Debug, Default)]
pub struct Color {
    /// A provided foreground color
    pub foreground: Option<color::Name>,
    /// A provided background color
    pub background: Option<color::Name>,
    /// A potentially empty set of text attributes
    pub attributes: color::Attribute,
}

/// Any value that can be interpreted as an integer.
///
/// Use [`Integer::from_bytes()`] to parse raw input, apply any suffix multiplier, and convert
/// to a signed or unsigned integer in one step, with classified errors for invalid or overflowing values.
///
/// Converting to this type with [`TryFrom`] instead preserves the suffix separately from the value,
/// which must fit in an [`i64`] before applying the suffix. Use [`Integer::to_decimal()`] to obtain
/// the multiplied value of an already-parsed integer.
/// Conversion errors expose invalid `input` bytes as [metadata](gix_error::Error::metadata()).
#[derive(Default, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Debug)]
pub struct Integer {
    /// The value, without any suffix modification
    pub value: i64,
    /// A provided suffix, if any.
    pub suffix: Option<integer::Suffix>,
}

/// Any value that can be interpreted as a boolean.
/// Conversion errors expose invalid `input` bytes as [metadata](gix_error::Error::metadata()).
#[derive(Default, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Debug)]
pub struct Boolean(
    /// The interpreted boolean value.
    pub bool,
);

/// Any value that can be interpreted as a path to a resource on disk.
///
/// Git represents file paths as byte arrays, modeled here as an owned byte sequence.
///
/// ## Optional Paths
///
/// Paths can be marked as optional by prefixing them with `:(optional)` in the configuration.
/// This indicates that it's acceptable if the file doesn't exist, which is useful for
/// configuration values like `blame.ignoreRevsFile` that may only exist in some repositories.
///
/// ```
/// use gix_config_value::Path;
/// use bstr::ByteSlice;
///
/// // Regular path - file is expected to exist
/// let path = Path::from("/etc/gitconfig");
/// assert!(!path.is_optional);
///
/// // Optional path - it's okay if the file doesn't exist
/// let path = Path::from(":(optional)~/.gitignore");
/// assert!(path.is_optional);
/// assert_eq!(path.value.as_bstr(), "~/.gitignore"); // prefix is stripped
/// ```
#[derive(Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Debug)]
pub struct Path {
    /// The path string, un-interpolated
    pub value: bstr::BString,
    /// Whether this path was prefixed with `:(optional)`, indicating it's acceptable if the file doesn't exist.
    ///
    /// Optional paths indicate that it's acceptable if the file doesn't exist.
    /// This is typically used for configuration like `blame.ignorerevsfile` where
    /// the file might not exist in all repositories.
    pub is_optional: bool,
}
