//! serde-based `Cargo.lock` parser/serializer
//!
//! Customized to allow pre/postprocessing to detect and serialize both
//! the V1 vs V2 formats and ensure the end-user is supplied a consistent
//! representation regardless of which version is in use.
//!
//! Parts adapted from upstream Cargo.
//! Cargo is primarily distributed under the terms of both the MIT license and
//! the Apache License (Version 2.0).

use super::{Lockfile, ResolveVersion};
use crate::{
    metadata, Checksum, Dependency, Error, Metadata, Name, Package, Patch, Result, SourceId,
    Version,
};
use serde::{de, ser, Deserialize, Serialize};
use std::{collections::HashMap, collections::HashSet, fmt, fmt::Write, str::FromStr};

impl<'de> Deserialize<'de> for Lockfile {
    fn deserialize<D: de::Deserializer<'de>>(
        deserializer: D,
    ) -> std::result::Result<Self, D::Error> {
        EncodableLockfile::deserialize(deserializer)?
            .try_into()
            .map_err(de::Error::custom)
    }
}

impl Serialize for Lockfile {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> std::result::Result<S::Ok, S::Error> {
        EncodableLockfile::from(self).serialize(serializer)
    }
}

/// Serialization-oriented equivalent to [`Lockfile`]
#[derive(Debug, Deserialize, Serialize)]
pub(super) struct EncodableLockfile {
    /// Lockfile version
    pub(super) version: Option<u32>,

    /// Packages in the lockfile
    #[serde(default)]
    pub(super) package: Vec<EncodablePackage>,

    /// Legacy root package (preserved for compatibility)
    pub(super) root: Option<EncodablePackage>,

    /// Metadata fields
    #[serde(default, skip_serializing_if = "Metadata::is_empty")]
    pub(super) metadata: Metadata,

    /// Patch section
    #[serde(default, skip_serializing_if = "Patch::is_empty")]
    pub(super) patch: Patch,
}

impl EncodableLockfile {
    /// Attempt to find a checksum for a package in a V1 lockfile
    pub fn find_checksum(&self, package: &Package) -> Option<Checksum> {
        for (key, value) in &self.metadata {
            if let Ok(dep) = key.checksum_dependency() {
                if dep.name == package.name && dep.version == package.version {
                    return value.checksum().ok();
                }
            }
        }

        None
    }
}

impl TryFrom<EncodableLockfile> for Lockfile {
    type Error = Error;

    fn try_from(raw_lockfile: EncodableLockfile) -> Result<Lockfile> {
        let version = match raw_lockfile.version {
            Some(n) => n.try_into()?,
            None => ResolveVersion::detect(&raw_lockfile.package, &raw_lockfile.metadata)?,
        };

        let mut packages = Vec::with_capacity(raw_lockfile.package.len());

        for raw_package in &raw_lockfile.package {
            packages.push(if version == ResolveVersion::V1 {
                // In the V1 format, all dependencies are fully qualified with
                // their versions, but their checksums are stored in metadata.
                let mut pkg = Package::try_from(raw_package)?;
                pkg.checksum = raw_lockfile.find_checksum(&pkg);
                pkg
            } else {
                // In newer versions, we may need to look up dependency versions
                // from the other packages listed in the lockfile
                raw_package.resolve(&raw_lockfile.package)?
            })
        }

        Ok(Lockfile {
            version,
            packages,
            root: raw_lockfile
                .root
                .as_ref()
                .map(|root| root.try_into())
                .transpose()?,
            metadata: raw_lockfile.metadata,
            patch: raw_lockfile.patch,
        })
    }
}

impl From<&Lockfile> for EncodableLockfile {
    fn from(lockfile: &Lockfile) -> EncodableLockfile {
        let mut packages = Vec::with_capacity(lockfile.packages.len());
        let mut metadata = lockfile.metadata.clone();

        let mut package_to_registries: HashMap<_, HashSet<&SourceId>> = HashMap::new();
        for package in &lockfile.packages {
            if let Some(source) = package.source.as_ref() {
                if source.is_registry() {
                    package_to_registries
                        .entry(&package.name)
                        .or_default()
                        .insert(source);
                }
            }
        }

        for package in &lockfile.packages {
            let mut raw_pkg = EncodablePackage::from_package(package, lockfile.version);
            let checksum_key = metadata::MetadataKey::for_checksum(&Dependency::from(package));

            if lockfile.version == ResolveVersion::V1 {
                // In the V1 format, we need to remove the checksum from
                // packages and add it to metadata
                if let Some(checksum) = raw_pkg.checksum.take() {
                    let value = checksum
                        .to_string()
                        .parse::<metadata::MetadataValue>()
                        .unwrap();
                    metadata.insert(checksum_key, value);
                }
            } else {
                // In newer versions, we need to remove the version/source from
                // unambiguous dependencies, and remove checksums from the
                // metadata table if present
                raw_pkg.v2_deps(&lockfile.packages);
                metadata.remove(&checksum_key);

                // If there's only one registry, Cargo does not encode this information so
                // it should be omitted from encodable packages
                for dep in raw_pkg.dependencies.iter_mut() {
                    if let Some(registries) = package_to_registries.get(&dep.name) {
                        if registries.len() == 1 {
                            dep.source = None;
                        }
                    }
                }
            }

            packages.push(raw_pkg);
        }

        let version = if lockfile.version.is_explicit() {
            Some(lockfile.version.into())
        } else {
            None
        };

        EncodableLockfile {
            version,
            package: packages,
            root: lockfile
                .root
                .as_ref()
                .map(|root| EncodablePackage::from_package(root, lockfile.version)),
            metadata,
            patch: lockfile.patch.clone(),
        }
    }
}

#[allow(clippy::to_string_trait_impl)]
impl ToString for EncodableLockfile {
    /// Adapted from `serialize_resolve` in upstream Cargo:
    /// <https://github.com/rust-lang/cargo/blob/0c70319/src/cargo/ops/lockfile.rs#L103-L174>
    fn to_string(&self) -> String {
        let toml = toml::Value::try_from(self).unwrap();
        let mut out = String::new();

        // At the start of the file we notify the reader that the file is generated.
        // Specifically Phabricator ignores files containing "@generated", so we use that.
        let marker_line = "# This file is automatically @generated by Cargo.";
        let extra_line = "# It is not intended for manual editing.";
        out.push_str(marker_line);
        out.push('\n');
        out.push_str(extra_line);
        out.push('\n');

        if let Some(value) = toml.get("version") {
            if let Some(version) = value.as_integer() {
                if version >= 3 {
                    writeln!(out, "version = {version}").unwrap();
                }
            }
        }

        out.push('\n');

        let deps = toml["package"].as_array().unwrap();
        for dep in deps {
            let dep = dep.as_table().unwrap();

            out.push_str("[[package]]\n");
            emit_package(dep, &mut out);
        }

        if let Some(patch) = toml.get("patch") {
            let list = patch["unused"].as_array().unwrap();
            for entry in list {
                out.push_str("[[patch.unused]]\n");
                emit_package(entry.as_table().unwrap(), &mut out);
                out.push('\n');
            }
        }

        if let Some(meta) = toml.get("metadata") {
            out.push_str("[metadata]\n");
            out.push_str(&toml::to_string_pretty(&meta).unwrap());
        }

        // Trim redundant newlines
        if out.ends_with("\n\n") {
            out.pop();
        }

        out
    }
}

/// Emit a single package from a lockfile.
///
/// This method is adapted from the same-named method in upstream Cargo:
/// <https://github.com/rust-lang/cargo/blob/0c70319/src/cargo/ops/lockfile.rs#L194-L221>
fn emit_package(dep: &toml::value::Table, out: &mut String) {
    writeln!(out, "name = {}", &dep["name"]).unwrap();
    writeln!(out, "version = {}", &dep["version"]).unwrap();

    if dep.contains_key("source") {
        writeln!(out, "source = {}", &dep["source"]).unwrap();
    }
    if dep.contains_key("checksum") {
        writeln!(out, "checksum = {}", &dep["checksum"]).unwrap();
    }

    if let Some(s) = dep.get("dependencies") {
        let slice = s.as_array().unwrap();

        if !slice.is_empty() {
            out.push_str("dependencies = [\n");

            for child in slice.iter() {
                writeln!(out, " {child},").unwrap();
            }

            out.push_str("]\n");
        }
    } else if dep.contains_key("replace") {
        writeln!(out, "replace = {}", &dep["replace"]).unwrap();
    }

    out.push('\n');
}

/// Serialization-oriented equivalent to [`Package`]
#[derive(Debug, Deserialize, Serialize)]
pub(crate) struct EncodablePackage {
    /// Package name
    pub(super) name: Name,

    /// Package version
    pub(super) version: Version,

    /// Source of a package
    pub(super) source: Option<EncodableSourceId>,

    /// Package checksum
    pub(super) checksum: Option<Checksum>,

    /// Package dependencies
    #[serde(default, skip_serializing_if = "Vec::is_empty")]
    pub(super) dependencies: Vec<EncodableDependency>,

    /// Replace directive
    pub(super) replace: Option<EncodableDependency>,
}

impl EncodablePackage {
    /// Resolve all of the dependencies of a package, which in the V2 format
    /// may be abbreviated to prevent merge conflicts
    fn resolve(&self, packages: &[EncodablePackage]) -> Result<Package> {
        let mut dependencies = Vec::with_capacity(self.dependencies.len());

        for dep in &self.dependencies {
            dependencies.push(dep.resolve(packages)?);
        }

        Ok(Package {
            name: self.name.clone(),
            version: self.version.clone(),
            source: self.source.as_ref().map(|s| s.inner.clone()),
            checksum: self.checksum.clone(),
            dependencies,
            replace: self
                .replace
                .as_ref()
                .map(|rep| rep.try_into())
                .transpose()?,
        })
    }

    /// Prepare `ResolveVersion::V2` dependencies by removing ones which are unambiguous
    fn v2_deps(&mut self, packages: &[Package]) {
        for dependency in &mut self.dependencies {
            dependency.v2(packages);
        }
    }

    fn from_package(package: &Package, version: ResolveVersion) -> EncodablePackage {
        EncodablePackage {
            name: package.name.clone(),
            version: package.version.clone(),
            source: package
                .source
                .clone()
                .and_then(|id| encodable_source_id(id, version)),
            checksum: package.checksum.clone(),
            dependencies: package
                .dependencies
                .iter()
                .map(|dep| EncodableDependency::from_dependency(dep, version))
                .collect::<Vec<_>>(),
            replace: package
                .replace
                .as_ref()
                .map(|rep| EncodableDependency::from_dependency(rep, version)),
        }
    }
}

fn encodable_source_id(id: SourceId, version: ResolveVersion) -> Option<EncodableSourceId> {
    if id.is_path() {
        None
    } else {
        Some(if version >= ResolveVersion::V4 {
            EncodableSourceId::new(id)
        } else {
            EncodableSourceId::without_url_encoded(id)
        })
    }
}

/// Note: this only works for `ResolveVersion::V1` dependencies.
impl TryFrom<&EncodablePackage> for Package {
    type Error = Error;

    fn try_from(raw_package: &EncodablePackage) -> Result<Package> {
        raw_package.resolve(&[])
    }
}

/// Package dependencies
#[derive(Clone, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
pub(crate) struct EncodableDependency {
    /// Name of the dependency
    pub(super) name: Name,

    /// Version of the dependency
    pub(super) version: Option<Version>,

    /// Source for the dependency
    pub(super) source: Option<EncodableSourceId>,
}

impl EncodableDependency {
    /// Resolve this dependency, which in the V2 format may be abbreviated to
    /// prevent merge conflicts
    pub fn resolve(&self, packages: &[EncodablePackage]) -> Result<Dependency> {
        for pkg in packages {
            if pkg.name == self.name
                && (self.version.is_none() || self.version.as_ref() == Some(&pkg.version))
                && self.source.is_none()
            {
                return Ok(Dependency {
                    name: pkg.name.clone(),
                    version: pkg.version.clone(),
                    source: pkg
                        .source
                        .clone()
                        .map(|x| x.normalize_git_source_for_dependency()),
                });
            }
        }

        let version = self
            .version
            .clone()
            .ok_or_else(|| Error::Parse(format!("couldn't resolve dependency: {}", self.name)))?;

        Ok(Dependency {
            name: self.name.clone(),
            version,
            source: self
                .source
                .clone()
                .map(|x| x.normalize_git_source_for_dependency()),
        })
    }

    /// Prepare `ResolveVersion::V2` dependencies by removing ones which are unambiguous
    pub fn v2(&mut self, packages: &[Package]) {
        let mut matching = vec![];

        for package in packages {
            if package.name == self.name {
                matching.push(package);
            }
        }

        // TODO(tarcieri): better handle other cases?
        if matching.len() == 1 {
            self.version = None;
            self.source = None;
        }
    }

    pub fn from_dependency(dep: &Dependency, version: ResolveVersion) -> EncodableDependency {
        EncodableDependency {
            name: dep.name.clone(),
            version: Some(dep.version.clone()),
            source: dep
                .source
                .clone()
                .and_then(|id| encodable_source_id(id, version)),
        }
    }
}

/// Note: this only works for `ResolveVersion::V1` dependencies.
impl FromStr for EncodableDependency {
    type Err = Error;

    fn from_str(s: &str) -> Result<Self> {
        let mut parts = s.split_whitespace();

        let name = parts
            .next()
            .ok_or_else(|| Error::Parse("empty dependency string".to_owned()))?
            .parse()?;

        let version = parts.next().map(FromStr::from_str).transpose()?;

        let source = parts
            .next()
            .map(|s| {
                if s.len() < 2 || !s.starts_with('(') || !s.ends_with(')') {
                    Err(Error::Parse(format!("malformed source in dependency: {s}")))
                } else {
                    s[1..(s.len() - 1)].parse::<SourceId>()
                }
            })
            .transpose()?;

        if parts.next().is_some() {
            return Err(Error::Parse(format!("malformed dependency: {s}")));
        }

        Ok(Self {
            name,
            version,
            // `EncodableDependency::from_str` is found only used by MetadataKey,
            // which is only a thing in lockfile v1.
            // Hence, no need for url encoding.
            source: source.map(EncodableSourceId::without_url_encoded),
        })
    }
}

impl fmt::Display for EncodableDependency {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", &self.name)?;

        if let Some(version) = &self.version {
            write!(f, " {version}")?;
        }

        if let Some(source) = &self.source {
            write!(f, " ({})", source.as_url())?;
        }

        Ok(())
    }
}

/// Note: this only works for `ResolveVersion::V1` dependencies.
impl TryFrom<&EncodableDependency> for Dependency {
    type Error = Error;

    fn try_from(raw_dependency: &EncodableDependency) -> Result<Dependency> {
        raw_dependency.resolve(&[])
    }
}

impl<'de> Deserialize<'de> for EncodableDependency {
    fn deserialize<D: de::Deserializer<'de>>(
        deserializer: D,
    ) -> std::result::Result<Self, D::Error> {
        String::deserialize(deserializer)?
            .parse()
            .map_err(de::Error::custom)
    }
}

impl Serialize for EncodableDependency {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> std::result::Result<S::Ok, S::Error> {
        self.to_string().serialize(serializer)
    }
}

/// Pretty much equivalent to [`SourceId`] with a different serialization method.
///
/// The serialization for `SourceId` doesn't do URL encode for parameters.
/// In contrast, this type is aware of that whenever [`ResolveVersion`] allows
/// us to do so (v4 or later).
#[derive(Deserialize, Debug, PartialOrd, Ord, Clone)]
#[serde(transparent)]
pub(super) struct EncodableSourceId {
    inner: SourceId,
    /// We don't care about the deserialization of this, as the `url` crate
    /// will always decode as the URL was encoded.
    #[serde(skip)]
    encoded: bool,
}

impl EncodableSourceId {
    /// Creates a `EncodableSourceId` that always encodes URL params.
    fn new(inner: SourceId) -> Self {
        Self {
            inner,
            encoded: true,
        }
    }

    /// Creates a `EncodableSourceId` that doesn't encode URL params. This is
    /// for backward compatibility for order lockfile version.
    fn without_url_encoded(inner: SourceId) -> Self {
        Self {
            inner,
            encoded: false,
        }
    }

    /// Encodes the inner [`SourceId`] as a URL.
    fn as_url(&self) -> impl fmt::Display + '_ {
        self.inner.as_url(self.encoded)
    }
}

impl std::ops::Deref for EncodableSourceId {
    type Target = SourceId;

    fn deref(&self) -> &Self::Target {
        &self.inner
    }
}

impl Serialize for EncodableSourceId {
    fn serialize<S: ser::Serializer>(&self, serializer: S) -> std::result::Result<S::Ok, S::Error> {
        serializer.collect_str(&self.as_url())
    }
}

impl std::hash::Hash for EncodableSourceId {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.inner.hash(state)
    }
}

impl PartialEq for EncodableSourceId {
    fn eq(&self, other: &Self) -> bool {
        self.inner == other.inner
    }
}

impl Eq for EncodableSourceId {}
