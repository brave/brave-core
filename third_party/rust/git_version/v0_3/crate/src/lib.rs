#![no_std]

//! Embed git information in your code at compile-time.
//!
//! ```
//! use git_version::git_version;
//! const GIT_VERSION: &str = git_version!();
//! ```
//!
//! The version number will have a `-modified` suffix if your git worktree had
//! untracked or changed files.
//!
//! These macros do not depend on libgit, but simply uses the `git` binary directly.
//! So you must have `git` installed somewhere in your `PATH`.

use proc_macro_hack::proc_macro_hack;

/// Get the git version for the source code.
///
/// The following (named) arguments can be given:
///
/// - `args`: The arguments to call `git describe` with.
///   Default: `args = ["--always", "--dirty=-modified"]`
///
/// - `prefix`, `suffix`:
///   The git version will be prefixed/suffexed by these strings.
///
/// - `cargo_prefix`, `cargo_suffix`:
///   If either is given, Cargo's version (given by the CARGO_PKG_VERSION
///   environment variable) will be used if git fails instead of giving an
///   error. It will be prefixed/suffixed by the given strings.
///
/// - `fallback`:
///   If all else fails, this string will be given instead of reporting an
///   error.
///
/// # Examples
///
/// ```ignore
/// const VERSION: &str = git_version!();
/// ```
///
/// ```ignore
/// const VERSION: &str = git_version!(args = ["--abbrev=40", "--always"]);
/// ```
///
/// ```
/// # use git_version::git_version;
/// const VERSION: &str = git_version!(prefix = "git:", cargo_prefix = "cargo:", fallback = "unknown");
/// ```
#[proc_macro_hack]
pub use git_version_macro::git_version;

/// Run `git describe` at compile time with custom flags.
///
/// This is just a short-hand for `git_version!(args = [...])`,
/// to be backwards compatible with earlier versions of this crate.
#[macro_export]
macro_rules! git_describe {
	($($args:tt)*) => {
		$crate::git_version!(args = [$($args)*])
	};
}
