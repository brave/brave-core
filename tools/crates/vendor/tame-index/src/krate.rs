//! Provides types for the structured metadata stored in a registry index

mod dedupe;

use crate::Error;
use dedupe::DedupeContext;
use semver::Version;
use serde::{Deserialize, Serialize};
use smol_str::SmolStr;
use std::{collections::BTreeMap, sync::Arc};

/// A mapping of feature name to the features it enables
pub type FeatureMap = BTreeMap<String, Vec<String>>;

/// A single version of a crate (package) published to the index
#[derive(Serialize, Deserialize, Clone, Debug, Eq, PartialEq)]
pub struct IndexVersion {
    /// [Name](https://doc.rust-lang.org/cargo/reference/manifest.html#the-name-field)
    pub name: SmolStr,
    /// [Version](https://doc.rust-lang.org/cargo/reference/manifest.html#the-version-field)
    #[serde(rename = "vers")]
    pub version: SmolStr,
    /// [Dependencies](https://doc.rust-lang.org/cargo/reference/specifying-dependencies.html)
    pub deps: Arc<[IndexDependency]>,
    /// The SHA-256 for this crate version's tarball
    #[serde(rename = "cksum")]
    pub checksum: Chksum,
    /// [Features](https://doc.rust-lang.org/cargo/reference/features.html)
    features: Arc<FeatureMap>,
    /// Version 2 of the index includes this field
    /// <https://rust-lang.github.io/rfcs/3143-cargo-weak-namespaced-features.html#index-changes>
    #[serde(default, skip_serializing_if = "Option::is_none")]
    features2: Option<Arc<FeatureMap>>,
    /// Whether the crate is yanked from the remote index or not
    #[serde(default)]
    pub yanked: bool,
    /// [Links](https://doc.rust-lang.org/cargo/reference/manifest.html#the-links-field)
    #[serde(skip_serializing_if = "Option::is_none")]
    pub links: Option<Box<SmolStr>>,
    /// [Rust Version](https://doc.rust-lang.org/cargo/reference/manifest.html#the-rust-version-field)
    #[serde(skip_serializing_if = "Option::is_none")]
    pub rust_version: Option<SmolStr>,
    /// The index version, 1 if not set, v2 indicates presence of feature2 field
    #[serde(skip_serializing_if = "Option::is_none")]
    v: Option<u32>,
}

impl IndexVersion {
    /// Test functionality
    #[doc(hidden)]
    pub fn fake(name: &str, version: impl Into<SmolStr>) -> Self {
        Self {
            name: name.into(),
            version: version.into(),
            deps: Arc::new([]),
            features: Arc::default(),
            features2: None,
            links: None,
            rust_version: None,
            checksum: Chksum(Default::default()),
            yanked: false,
            v: None,
        }
    }

    /// Dependencies for this version
    #[inline]
    pub fn dependencies(&self) -> &[IndexDependency] {
        &self.deps
    }

    /// Checksum of the package for this version
    ///
    /// SHA256 of the .crate file
    #[inline]
    pub fn checksum(&self) -> &[u8; 32] {
        &self.checksum.0
    }

    /// Explicit feature set for this crate.
    ///
    /// This list is not exhaustive, because any optional dependency becomes a
    /// feature automatically.
    ///
    /// `default` is a special feature name for implicitly enabled features.
    #[inline]
    pub fn features(&self) -> impl Iterator<Item = (&String, &Vec<String>)> {
        self.features.iter().chain(
            self.features2
                .as_ref()
                .map(|f| f.iter())
                .into_iter()
                .flatten(),
        )
    }

    /// Exclusivity flag. If this is a sys crate, it informs it
    /// conflicts with any other crate with the same links string.
    ///
    /// It does not involve linker or libraries in any way.
    #[inline]
    pub fn links(&self) -> Option<&str> {
        self.links.as_ref().map(|s| s.as_str())
    }

    /// Whether this version was [yanked](http://doc.crates.io/crates-io.html#cargo-yank) from the
    /// index
    #[inline]
    pub fn is_yanked(&self) -> bool {
        self.yanked
    }

    /// Required version of rust
    ///
    /// Corresponds to `package.rust-version`.
    ///
    /// Added in 2023 (see <https://github.com/rust-lang/crates.io/pull/6267>),
    /// can be `None` if published before then or if not set in the manifest.
    #[inline]
    pub fn rust_version(&self) -> Option<&str> {
        self.rust_version.as_deref()
    }

    /// Retrieves the URL this crate version's tarball can be downloaded from
    #[inline]
    pub fn download_url(&self, index: &crate::index::IndexConfig) -> Option<String> {
        Some(index.download_url(self.name.as_str().try_into().ok()?, self.version.as_ref()))
    }
}

/// A single dependency of a specific crate version
#[derive(Serialize, Deserialize, Clone, Debug, Eq, PartialEq, Hash)]
pub struct IndexDependency {
    /// Dependency's arbitrary nickname (it may be an alias). Use [`Self::crate_name`] for actual crate name.
    pub name: SmolStr,
    /// The version requirement, as a string
    pub req: SmolStr,
    /// Double indirection to remove size from this struct, since the features are rarely set
    pub features: Box<Box<[String]>>,
    /// If it is an optional dependency
    pub optional: bool,
    /// True if the default features are enabled
    pub default_features: bool,
    /// Cfg expression applied to the dependency
    pub target: Option<Box<SmolStr>>,
    /// The kind of the dependency
    #[serde(skip_serializing_if = "Option::is_none")]
    pub kind: Option<DependencyKind>,
    /// The name of the actual crate, if it was renamed in the crate's manifest
    #[serde(skip_serializing_if = "Option::is_none")]
    pub package: Option<Box<SmolStr>>,
}

impl IndexDependency {
    /// Gets the version requirement for the dependency as a [`semver::VersionReq`]
    #[inline]
    pub fn version_requirement(&self) -> semver::VersionReq {
        self.req.parse().unwrap()
    }

    /// Features unconditionally enabled when using this dependency, in addition
    /// to [`Self::has_default_features`] and features enabled through the
    /// parent crate's feature list.
    #[inline]
    pub fn features(&self) -> &[String] {
        &self.features
    }

    /// If it's optional, it implies a feature of its [`Self::name`], and
    /// can be enabled through the parent crate's features.
    #[inline]
    pub fn is_optional(&self) -> bool {
        self.optional
    }

    /// If `true` (default), enable `default` feature of this dependency
    #[inline]
    pub fn has_default_features(&self) -> bool {
        self.default_features
    }

    /// This dependency is only used when compiling for this `cfg` expression
    #[inline]
    pub fn target(&self) -> Option<&str> {
        self.target.as_ref().map(|s| s.as_str())
    }

    /// The kind of the dependency
    #[inline]
    pub fn kind(&self) -> DependencyKind {
        self.kind.unwrap_or_default()
    }

    /// Set if dependency's crate name is different from the `name` (alias)
    #[inline]
    pub fn package(&self) -> Option<&str> {
        self.package.as_ref().map(|s| s.as_str())
    }

    /// Returns the name of the crate providing the dependency.
    /// This is equivalent to `name()` unless `self.package()`
    /// is not `None`, in which case it's equal to `self.package()`.
    ///
    /// Basically, you can define a dependency in your `Cargo.toml`
    /// like this:
    ///
    /// ```toml
    /// serde_lib = { version = "1", package = "serde" }
    /// ```
    ///
    /// ...which means that it uses the crate `serde` but imports
    /// it under the name `serde_lib`.
    #[inline]
    pub fn crate_name(&self) -> &str {
        match &self.package {
            Some(s) => s,
            None => &self.name,
        }
    }
}

/// Section in which this dependency was defined
#[derive(Debug, Copy, Clone, Serialize, Deserialize, Eq, PartialEq, Hash, Default)]
#[serde(rename_all = "lowercase")]
pub enum DependencyKind {
    /// Used at run time
    #[default]
    Normal,
    /// Not fetched and not used, except for when used direclty in a workspace
    Dev,
    /// Used at build time, not available at run time
    Build,
}

/// A whole crate with all its versions
#[derive(Serialize, Deserialize, Clone, Debug, Eq, PartialEq)]
pub struct IndexKrate {
    /// All versions of the crate, sorted chronologically by when it was published
    pub versions: Vec<IndexVersion>,
}

impl IndexKrate {
    /// The highest version as per semantic versioning specification
    ///
    /// Note this may be a pre-release or yanked, use [`Self::highest_normal_version`]
    /// to filter to the highest version that is not one of those
    #[inline]
    pub fn highest_version(&self) -> &IndexVersion {
        self.versions
            .iter()
            .max_by_key(|v| Version::parse(&v.version).ok())
            // SAFETY: Versions inside the index will always adhere to
            // semantic versioning. If a crate is inside the index, at
            // least one version is available.
            .unwrap()
    }

    /// Returns crate version with the highest version number according to semver,
    /// but excludes pre-release and yanked versions.
    ///
    /// 0.x.y versions are included.
    ///
    /// May return `None` if the crate has only pre-release or yanked versions.
    #[inline]
    pub fn highest_normal_version(&self) -> Option<&IndexVersion> {
        self.versions
            .iter()
            .filter_map(|v| {
                if v.is_yanked() {
                    return None;
                }

                v.version
                    .parse::<Version>()
                    .ok()
                    .filter(|v| v.pre.is_empty())
                    .map(|vs| (v, vs))
            })
            .max_by(|a, b| a.1.cmp(&b.1))
            .map(|(v, _vs)| v)
    }

    /// The crate's unique registry name. Case-sensitive, mostly.
    #[inline]
    pub fn name(&self) -> &str {
        &self.versions[0].name
    }

    /// The last release by date, even if it's yanked or less than highest version.
    ///
    /// See [`Self::highest_normal_version`]
    #[inline]
    pub fn most_recent_version(&self) -> &IndexVersion {
        &self.versions[self.versions.len() - 1]
    }

    /// First version ever published. May be yanked.
    ///
    /// It is not guaranteed to be the lowest version number.
    #[inline]
    pub fn earliest_version(&self) -> &IndexVersion {
        &self.versions[0]
    }
}

impl IndexKrate {
    /// Parse an index file with all of crate's versions.
    ///
    /// The file must contain at least one version.
    #[inline]
    pub fn new(index_path: impl AsRef<crate::Path>) -> Result<Self, Error> {
        let lines = std::fs::read(index_path.as_ref())?;
        Self::from_slice(&lines)
    }

    /// Parse a crate from in-memory JSON-lines data
    #[inline]
    pub fn from_slice(bytes: &[u8]) -> Result<Self, Error> {
        let mut dedupe = DedupeContext::default();
        Self::from_slice_with_context(bytes, &mut dedupe)
    }

    /// Parse a [`Self`] file from in-memory JSON data
    pub(crate) fn from_slice_with_context(
        mut bytes: &[u8],
        dedupe: &mut DedupeContext,
    ) -> Result<Self, Error> {
        use crate::index::cache::split;
        // Trim last newline(s) so we don't need to special case the split
        while bytes.last() == Some(&b'\n') {
            bytes = &bytes[..bytes.len() - 1];
        }

        let num_versions = split(bytes, b'\n').count();
        let mut versions = Vec::with_capacity(num_versions);
        for line in split(bytes, b'\n') {
            let mut version: IndexVersion = serde_json::from_slice(line)?;

            // Many versions have identical dependencies and features
            dedupe.deps(&mut version.deps);
            dedupe.features(&mut version.features);

            if let Some(features2) = &mut version.features2 {
                dedupe.features(features2);
            }

            versions.push(version);
        }

        if versions.is_empty() {
            return Err(Error::NoCrateVersions);
        }

        Ok(Self { versions })
    }

    /// Writes this crate into a JSON-lines formatted buffer
    ///
    /// Note this creates its own internal [`std::io::BufWriter`], there is no
    /// need to wrap it in your own
    pub fn write_json_lines<W: std::io::Write>(&self, writer: &mut W) -> Result<(), Error> {
        use std::io::{BufWriter, Write};

        let mut w = BufWriter::new(writer);
        for iv in &self.versions {
            serde_json::to_writer(&mut w, &iv)?;
            w.write_all(b"\n")?;
        }

        Ok(w.flush()?)
    }
}

/// A SHA-256 checksum, this is used by cargo to verify the contents of a crate's
/// tarball
#[derive(Clone, Eq, PartialEq)]
pub struct Chksum(pub [u8; 32]);

use std::fmt;

impl fmt::Debug for Chksum {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut hex = [0; 64];
        let hs = crate::utils::encode_hex(&self.0, &mut hex);

        f.debug_struct("Chksum").field("sha-256", &hs).finish()
    }
}

impl fmt::Display for Chksum {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut hex = [0; 64];
        let hs = crate::utils::encode_hex(&self.0, &mut hex);

        f.write_str(hs)
    }
}

/// Errors that can occur parsing a sha-256 hex string
#[derive(Debug)]
pub enum ChksumParseError {
    /// The checksum string had an invalid length
    InvalidLength(usize),
    /// The checksum string contained a non-hex character
    InvalidValue(char),
}

impl std::error::Error for ChksumParseError {}

impl fmt::Display for ChksumParseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::InvalidLength(len) => {
                write!(f, "expected string with length 64 but got length {len}")
            }
            Self::InvalidValue(c) => write!(f, "encountered non-hex character '{c}'"),
        }
    }
}

impl std::str::FromStr for Chksum {
    type Err = ChksumParseError;

    fn from_str(data: &str) -> Result<Self, Self::Err> {
        if data.len() != 64 {
            return Err(ChksumParseError::InvalidLength(data.len()));
        }

        let mut array = [0u8; 32];

        for (ind, chunk) in data.as_bytes().chunks(2).enumerate() {
            #[inline]
            fn parse_hex(b: u8) -> Result<u8, ChksumParseError> {
                Ok(match b {
                    b'A'..=b'F' => b - b'A' + 10,
                    b'a'..=b'f' => b - b'a' + 10,
                    b'0'..=b'9' => b - b'0',
                    c => {
                        return Err(ChksumParseError::InvalidValue(c as char));
                    }
                })
            }

            let mut cur = parse_hex(chunk[0])?;
            cur <<= 4;
            cur |= parse_hex(chunk[1])?;

            array[ind] = cur;
        }

        Ok(Self(array))
    }
}

impl<'de> Deserialize<'de> for Chksum {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        use serde::de::Error;
        struct HexStrVisitor;

        impl<'de> serde::de::Visitor<'de> for HexStrVisitor {
            type Value = Chksum;

            fn expecting(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                write!(f, "a hex encoded string")
            }

            fn visit_str<E: Error>(self, data: &str) -> Result<Self::Value, E> {
                data.parse().map_err(|err| match err {
                    ChksumParseError::InvalidLength(len) => {
                        serde::de::Error::invalid_length(len, &"a string with 64 characters")
                    }
                    ChksumParseError::InvalidValue(c) => serde::de::Error::invalid_value(
                        serde::de::Unexpected::Char(c),
                        &"a hexadecimal character",
                    ),
                })
            }

            fn visit_borrowed_str<E: Error>(self, data: &'de str) -> Result<Self::Value, E> {
                self.visit_str(data)
            }
        }

        deserializer.deserialize_str(HexStrVisitor)
    }
}

impl Serialize for Chksum {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        let mut raw = [0u8; 64];
        let s = crate::utils::encode_hex(&self.0, &mut raw);
        serializer.serialize_str(s)
    }
}

#[cfg(test)]
mod test {
    #[test]
    fn krate_versions() {
        use super::IndexVersion as iv;
        let ik = super::IndexKrate {
            versions: vec![
                iv::fake("vers", "0.1.0"),
                iv::fake("vers", "0.1.1"),
                iv::fake("vers", "0.1.0"),
                iv::fake("vers", "0.2.0"),
                iv::fake("vers", "0.3.0"),
                // These are ordered this way to actually test the methods correctly
                iv::fake("vers", "0.4.0"),
                iv::fake("vers", "0.4.0-alpha.00"),
                {
                    let mut iv = iv::fake("vers", "0.5.0");
                    iv.yanked = true;
                    iv
                },
            ],
        };

        assert_eq!(ik.earliest_version().version, "0.1.0");
        assert_eq!(ik.most_recent_version().version, "0.5.0");
        assert_eq!(ik.highest_version().version, "0.5.0");
        assert_eq!(ik.highest_normal_version().unwrap().version, "0.4.0");
    }
}
