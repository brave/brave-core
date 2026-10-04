//! Package source identifiers.
//!
//! Adapted from Cargo's `source_id.rs`:
//!
//! <https://github.com/rust-lang/cargo/blob/master/src/cargo/core/source/source_id.rs>
//!
//! Copyright (c) 2014 The Rust Project Developers
//! Licensed under the same terms as the `cargo-lock` crate: Apache 2.0 + MIT

use crate::error::{Error, Result};
use serde::{Deserialize, Serialize, de, ser};
use std::{
    cmp::{Ord, Ordering},
    fmt,
    hash::Hash,
    str::FromStr,
};
use url::Url;

#[cfg(any(unix, windows))]
use std::path::Path;

/// Location of the crates.io index
pub const CRATES_IO_INDEX: &str = "https://github.com/rust-lang/crates.io-index";
/// Location of the crates.io sparse HTTP index
pub const CRATES_IO_SPARSE_INDEX: &str = "sparse+https://index.crates.io/";

/// Unique identifier for a source of packages.
#[derive(Clone, Debug)]
pub struct SourceId {
    /// The source URL.
    url: Url,

    /// The source kind.
    kind: SourceKind,

    /// For example, the exact Git revision of the specified branch for a Git Source.
    precise: Option<String>,

    /// Name of the registry source for alternative registries
    name: Option<String>,
}

impl SourceId {
    /// Creates a `SourceId` object from the kind and URL.
    fn new(kind: SourceKind, url: Url) -> Result<Self> {
        Ok(Self {
            kind,
            url,
            precise: None,
            name: None,
        })
    }

    /// Parses a source URL and returns the corresponding ID.
    ///
    /// ## Example
    ///
    /// ```
    /// use cargo_lock::SourceId;
    /// SourceId::from_url("git+https://github.com/alexcrichton/\
    ///                     libssh2-static-sys#80e71a3021618eb05\
    ///                     656c58fb7c5ef5f12bc747f");
    /// ```
    pub fn from_url(string: &str) -> Result<Self> {
        let mut parts = string.splitn(2, '+');
        let kind = parts.next().unwrap();
        let url = parts
            .next()
            .ok_or_else(|| Error::Parse(format!("invalid source `{string}`")))?;

        match kind {
            "git" => {
                let mut url = url.into_url()?;
                let mut reference = GitReference::DefaultBranch;
                for (k, v) in url.query_pairs() {
                    match &k[..] {
                        // Map older 'ref' to branch.
                        "branch" | "ref" => reference = GitReference::Branch(v.into_owned()),

                        "rev" => reference = GitReference::Rev(v.into_owned()),
                        "tag" => reference = GitReference::Tag(v.into_owned()),
                        _ => {}
                    }
                }
                let precise = url.fragment().map(|s| s.to_owned());
                url.set_fragment(None);
                url.set_query(None);
                Ok(Self::for_git(&url, reference)?.with_precise(precise))
            }
            "registry" => {
                let url = url.into_url()?;
                Ok(SourceId::new(SourceKind::Registry, url)?
                    .with_precise(Some("locked".to_string())))
            }
            "sparse" => {
                let url = url.into_url()?;
                Ok(SourceId::new(SourceKind::SparseRegistry, url)?
                    .with_precise(Some("locked".to_string())))
            }
            "path" => Self::new(SourceKind::Path, url.into_url()?),
            kind => Err(Error::Parse(format!(
                "unsupported source protocol: `{kind}` from `{string}`"
            ))),
        }
    }

    /// Creates a `SourceId` from a filesystem path.
    ///
    /// `path`: an absolute path.
    #[cfg(any(unix, windows))]
    pub fn for_path(path: &Path) -> Result<Self> {
        Self::new(SourceKind::Path, path.into_url()?)
    }

    /// Creates a `SourceId` from a Git reference.
    pub fn for_git(url: &Url, reference: GitReference) -> Result<Self> {
        Self::new(SourceKind::Git(reference), url.clone())
    }

    /// Creates a SourceId from a remote registry URL.
    pub fn for_registry(url: &Url) -> Result<Self> {
        Self::new(SourceKind::Registry, url.clone())
    }

    /// Creates a SourceId from a local registry path.
    #[cfg(any(unix, windows))]
    pub fn for_local_registry(path: &Path) -> Result<Self> {
        Self::new(SourceKind::LocalRegistry, path.into_url()?)
    }

    /// Creates a `SourceId` from a directory path.
    #[cfg(any(unix, windows))]
    pub fn for_directory(path: &Path) -> Result<Self> {
        Self::new(SourceKind::Directory, path.into_url()?)
    }

    /// Gets this source URL.
    pub fn url(&self) -> &Url {
        &self.url
    }

    /// Get the kind of source.
    pub fn kind(&self) -> &SourceKind {
        &self.kind
    }

    /// Human-friendly description of an index
    pub fn display_index(&self) -> String {
        if self.is_default_registry() {
            "crates.io index".to_string()
        } else {
            format!("`{}` index", self.url())
        }
    }

    /// Human-friendly description of a registry name
    pub fn display_registry_name(&self) -> String {
        if self.is_default_registry() {
            "crates.io".to_string()
        } else if let Some(name) = &self.name {
            name.clone()
        } else {
            self.url().to_string()
        }
    }

    /// Returns `true` if this source is from a filesystem path.
    pub fn is_path(&self) -> bool {
        self.kind == SourceKind::Path
    }

    /// Returns `true` if this source is from a registry (either local or not).
    pub fn is_registry(&self) -> bool {
        matches!(
            self.kind,
            SourceKind::Registry | SourceKind::SparseRegistry | SourceKind::LocalRegistry
        )
    }

    /// Returns `true` if this source is a "remote" registry.
    ///
    /// "remote" may also mean a file URL to a git index, so it is not
    /// necessarily "remote". This just means it is not `local-registry`.
    pub fn is_remote_registry(&self) -> bool {
        matches!(self.kind, SourceKind::Registry | SourceKind::SparseRegistry)
    }

    /// Returns `true` if this source from a Git repository.
    pub fn is_git(&self) -> bool {
        matches!(self.kind, SourceKind::Git(_))
    }

    /// Gets the value of the precise field.
    pub fn precise(&self) -> Option<&str> {
        self.precise.as_ref().map(AsRef::as_ref)
    }

    /// Gets the Git reference if this is a git source, otherwise `None`.
    pub fn git_reference(&self) -> Option<&GitReference> {
        if let SourceKind::Git(s) = &self.kind {
            Some(s)
        } else {
            None
        }
    }

    /// Creates a new `SourceId` from this source with the given `precise`.
    pub fn with_precise(&self, v: Option<String>) -> Self {
        Self {
            precise: v,
            ..self.clone()
        }
    }

    /// Returns `true` if the remote registry is the standard <https://crates.io>.
    pub fn is_default_registry(&self) -> bool {
        self.kind == SourceKind::Registry && self.url.as_str() == CRATES_IO_INDEX
            || self.kind == SourceKind::SparseRegistry
                && self.url.as_str() == &CRATES_IO_SPARSE_INDEX[7..]
    }

    /// A view of the [`SourceId`] that can be `Display`ed as a URL.
    pub(crate) fn as_url(&self, encoded: bool) -> SourceIdAsUrl<'_> {
        SourceIdAsUrl { id: self, encoded }
    }
}

/// We've seen a number of subtle ways that dependency references (in `package.dependencies`)
/// can differ from the corresponding `package.source` field for git dependencies.
/// This `Ord` impl (which is used when storing `SourceId`s in a `BTreeMap`) tries to
/// account for these differences and treat them as equal.
///
/// The `package.source` field for a git dependency includes both the `tag`, `branch` or `rev`
/// (in a query string) used to fetch the dependency, as well as the full commit hash (in the
/// fragment), but the `package.dependencies` entry does not include the full commit hash.
///
/// Additionally, when the `rev` is specified for a dependency using a longer hash, the `rev`
/// used in the `package.source` may be an abbreviated hash.
impl Ord for SourceId {
    fn cmp(&self, other: &Self) -> Ordering {
        match self.url.cmp(&other.url) {
            Ordering::Equal => {}
            non_eq => return non_eq,
        }

        match self.name.cmp(&other.name) {
            Ordering::Equal => {}
            non_eq => return non_eq,
        }

        // Some special handling for git sources follows...
        match (&self.kind, &other.kind) {
            (SourceKind::Git(s), SourceKind::Git(o)) => (s, o),
            (a, b) => return a.cmp(b),
        };

        if let (Some(s), Some(o)) = (&self.precise, &other.precise) {
            // If the git hash is the same, we consider the sources equal
            return s.cmp(o);
        }

        Ordering::Equal
    }
}

impl PartialOrd for SourceId {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}

impl Hash for SourceId {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.url.hash(state);
        self.kind.hash(state);
        self.precise.hash(state);
        self.name.hash(state);
    }
}

impl PartialEq for SourceId {
    fn eq(&self, other: &Self) -> bool {
        self.cmp(other) == Ordering::Equal
    }
}

impl Eq for SourceId {}

impl Serialize for SourceId {
    fn serialize<S: ser::Serializer>(&self, s: S) -> std::result::Result<S::Ok, S::Error> {
        if self.is_path() {
            None::<String>.serialize(s)
        } else {
            s.collect_str(&self.to_string())
        }
    }
}

impl<'de> Deserialize<'de> for SourceId {
    fn deserialize<D: de::Deserializer<'de>>(d: D) -> std::result::Result<Self, D::Error> {
        let string = String::deserialize(d)?;
        SourceId::from_url(&string).map_err(de::Error::custom)
    }
}

impl FromStr for SourceId {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        Self::from_url(s)
    }
}

impl fmt::Display for SourceId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.as_url(false).fmt(f)
    }
}

impl Default for SourceId {
    fn default() -> SourceId {
        SourceId::for_registry(&CRATES_IO_INDEX.into_url().unwrap()).unwrap()
    }
}

/// The possible kinds of code source.
#[derive(Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[non_exhaustive]
pub enum SourceKind {
    /// A git repository.
    Git(GitReference),

    /// A local path..
    Path,

    /// A remote registry.
    Registry,

    /// A sparse registry.
    SparseRegistry,

    /// A local filesystem-based registry.
    LocalRegistry,

    /// A directory-based registry.
    #[cfg(any(unix, windows))]
    Directory,
}

/// A `Display`able view into a `SourceId` that will write it as a url
pub(crate) struct SourceIdAsUrl<'a> {
    id: &'a SourceId,
    encoded: bool,
}

impl fmt::Display for SourceIdAsUrl<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self.id {
            SourceId {
                kind: SourceKind::Path,
                url,
                ..
            } => write!(f, "path+{url}"),
            SourceId {
                kind: SourceKind::Git(reference),
                url,
                precise,
                ..
            } => {
                write!(f, "git+{url}")?;
                // TODO: set it to true when the default is lockfile v4,
                if let Some(pretty) = reference.pretty_ref(self.encoded) {
                    write!(f, "?{pretty}")?;
                }
                if let Some(precise) = precise.as_ref() {
                    write!(f, "#{precise}")?;
                }
                Ok(())
            }
            SourceId {
                kind: SourceKind::Registry,
                url,
                ..
            } => write!(f, "registry+{url}"),
            SourceId {
                kind: SourceKind::SparseRegistry,
                url,
                ..
            } => write!(f, "sparse+{url}"),
            SourceId {
                kind: SourceKind::LocalRegistry,
                url,
                ..
            } => write!(f, "local-registry+{url}"),
            #[cfg(any(unix, windows))]
            SourceId {
                kind: SourceKind::Directory,
                url,
                ..
            } => write!(f, "directory+{url}"),
        }
    }
}

/// Information to find a specific commit in a Git repository.
#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum GitReference {
    /// The default branch of the repository, the reference named `HEAD`.
    DefaultBranch,

    /// From a tag.
    Tag(String),

    /// From the HEAD of a branch.
    Branch(String),

    /// From a specific revision.
    Rev(String),
}

impl GitReference {
    /// Returns a `Display`able view of this git reference, or None if using
    /// the head of the default branch
    pub fn pretty_ref(&self, url_encoded: bool) -> Option<PrettyRef<'_>> {
        match self {
            Self::DefaultBranch => None,
            _ => Some(PrettyRef {
                inner: self,
                url_encoded,
            }),
        }
    }
}

/// A git reference that can be `Display`ed
pub struct PrettyRef<'a> {
    inner: &'a GitReference,
    url_encoded: bool,
}

impl fmt::Display for PrettyRef<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let value: &str = match self.inner {
            GitReference::DefaultBranch => return Ok(()),
            GitReference::Branch(s) => {
                write!(f, "branch=")?;
                s
            }
            GitReference::Tag(s) => {
                write!(f, "tag=")?;
                s
            }
            GitReference::Rev(s) => {
                write!(f, "rev=")?;
                s
            }
        };
        if self.url_encoded {
            for value in url::form_urlencoded::byte_serialize(value.as_bytes()) {
                write!(f, "{value}")?;
            }
        } else {
            write!(f, "{value}")?;
        }
        Ok(())
    }
}

/// A type that can be converted to a Url
trait IntoUrl {
    /// Performs the conversion
    fn into_url(self) -> Result<Url>;
}

impl IntoUrl for &str {
    fn into_url(self) -> Result<Url> {
        Url::parse(self).map_err(|s| Error::Parse(format!("invalid url `{self}`: {s}")))
    }
}

#[cfg(any(unix, windows))]
impl IntoUrl for &Path {
    fn into_url(self) -> Result<Url> {
        Url::from_file_path(self)
            .map_err(|_| Error::Parse(format!("invalid path url `{}`", self.display())))
    }
}

#[cfg(test)]
mod tests {
    use super::SourceId;

    #[test]
    fn identifies_crates_io() {
        assert!(SourceId::default().is_default_registry());
        assert!(
            SourceId::from_url(super::CRATES_IO_SPARSE_INDEX)
                .expect("failed to parse sparse URL")
                .is_default_registry()
        );
    }
}
