//! Helpers for initializing the remote and local disk location of an index

use crate::{Error, Path, PathBuf};
use std::borrow::Cow;

/// A remote index url
#[derive(Default, Debug)]
pub enum IndexUrl<'iu> {
    /// The canonical crates.io HTTP sparse index.
    ///
    /// See [`crate::CRATES_IO_HTTP_INDEX`]
    #[default]
    CratesIoSparse,
    /// The canonical crates.io git index.
    ///
    /// See [`crate::CRATES_IO_INDEX`]
    CratesIoGit,
    /// A non-crates.io index.
    ///
    /// This variant uses the url to determine the index kind (sparse or git) by
    /// inspecting the url's scheme. This is because sparse indices are required
    /// to have the `sparse+` scheme modifier
    NonCratesIo(Cow<'iu, str>),
    /// A [local registry](crate::index::LocalRegistry)
    Local(Cow<'iu, Path>),
}

impl<'iu> IndexUrl<'iu> {
    /// Gets the url as a string
    pub fn as_str(&'iu self) -> &'iu str {
        match self {
            Self::CratesIoSparse => crate::CRATES_IO_HTTP_INDEX,
            Self::CratesIoGit => crate::CRATES_IO_INDEX,
            Self::NonCratesIo(url) => url,
            Self::Local(pb) => pb.as_str(),
        }
    }

    /// Returns true if the url points to a sparse registry
    pub fn is_sparse(&self) -> bool {
        match self {
            Self::CratesIoSparse => true,
            Self::CratesIoGit | Self::Local(..) => false,
            Self::NonCratesIo(url) => url.starts_with("sparse+http"),
        }
    }

    /// Gets the [`IndexUrl`] for crates.io, depending on the local environment.
    ///
    /// 1. Determines if the crates.io registry has been [replaced](https://doc.rust-lang.org/cargo/reference/source-replacement.html)
    /// 2. Determines if the protocol was explicitly [configured](https://doc.rust-lang.org/cargo/reference/config.html#registriescrates-ioprotocol) by the user
    /// 3. Otherwise, detects the version of cargo (see [`crate::utils::cargo_version`]), and uses that to determine the appropriate default
    pub fn crates_io(
        config_root: Option<PathBuf>,
        cargo_home: Option<&Path>,
        cargo_version: Option<&str>,
    ) -> Result<Self, Error> {
        // If the crates.io registry has been replaced it doesn't matter what
        // the protocol for it has been changed to
        if let Some(replacement) = get_source_replacement(
            config_root.clone(),
            cargo_home,
            crate::CRATES_IO_INDEX,
            "crates-io",
        )? {
            return Ok(replacement);
        }

        let sparse_index = match std::env::var("CARGO_REGISTRIES_CRATES_IO_PROTOCOL")
            .ok()
            .as_deref()
        {
            Some("sparse") => true,
            Some("git") => false,
            _ => {
                let sparse_index = read_cargo_config(config_root, cargo_home, |_, config| {
                    match config
                        .pointer("/registries/crates-io/protocol")
                        .and_then(|p| p.as_str())?
                    {
                        "sparse" => Some(true),
                        "git" => Some(false),
                        _ => None,
                    }
                })?;

                if let Some(si) = sparse_index {
                    si
                } else {
                    let vers = match cargo_version {
                        Some(v) => v.trim().parse()?,
                        None => crate::utils::cargo_version(None)?,
                    };

                    vers >= semver::Version::new(1, 70, 0)
                }
            }
        };

        Ok(if sparse_index {
            Self::CratesIoSparse
        } else {
            Self::CratesIoGit
        })
    }

    /// Creates an [`IndexUrl`] for the specified registry name
    ///
    /// 1. Checks if [`CARGO_REGISTRIES_<name>_INDEX`](https://doc.rust-lang.org/cargo/reference/config.html#registriesnameindex) is set
    /// 2. Checks if the source for the registry has been [replaced](https://doc.rust-lang.org/cargo/reference/source-replacement.html)
    /// 3. Uses the value of [`registries.<name>.index`](https://doc.rust-lang.org/cargo/reference/config.html#registriesnameindex) otherwise
    pub fn for_registry_name(
        config_root: Option<PathBuf>,
        cargo_home: Option<&Path>,
        registry_name: &str,
    ) -> Result<Self, Error> {
        // Check if the index was explicitly specified
        let mut env = String::with_capacity(17 + registry_name.len() + 6);
        env.push_str("CARGO_REGISTRIES_");

        if registry_name.is_ascii() {
            for c in registry_name.chars() {
                if c == '-' {
                    env.push('_');
                } else {
                    env.push(c.to_ascii_uppercase());
                }
            }
        } else {
            let mut upper = registry_name.to_uppercase();
            if upper.contains('-') {
                upper = upper.replace('-', "_");
            }

            env.push_str(&upper);
        }

        env.push_str("_INDEX");

        match std::env::var(&env) {
            Ok(index) => return Ok(Self::NonCratesIo(index.into())),
            Err(err) => {
                if let std::env::VarError::NotUnicode(_nu) = err {
                    return Err(Error::NonUtf8EnvVar(env.into()));
                }
            }
        }

        let registry_url = read_cargo_config(config_root.clone(), cargo_home, |_, config| {
            let path = format!("/registries/{registry_name}/index");
            config.pointer(&path)?.as_str().map(String::from)
        })?
        .ok_or_else(|| Error::UnknownRegistry(registry_name.into()))?;

        if let Some(replacement) = get_source_replacement(
            config_root.clone(),
            cargo_home,
            &registry_url,
            registry_name,
        )? {
            return Ok(replacement);
        }

        Ok(Self::NonCratesIo(registry_url.into()))
    }
}

impl<'iu> From<&'iu str> for IndexUrl<'iu> {
    #[inline]
    fn from(s: &'iu str) -> Self {
        Self::NonCratesIo(s.into())
    }
}

/// The local disk location to place an index
#[derive(Default)]
pub enum IndexPath {
    /// The default cargo home root path
    #[default]
    CargoHome,
    /// User-specified root path
    UserSpecified(PathBuf),
    /// An exact path on disk where an index is located.
    ///
    /// Unlike the other two variants, this variant won't take the index's url
    /// into account to calculate the unique url hash as part of the full path
    Exact(PathBuf),
}

impl From<Option<PathBuf>> for IndexPath {
    /// Converts an optional path to a rooted path.
    ///
    /// This never constructs a [`Self::Exact`], that can only be done explicitly
    fn from(pb: Option<PathBuf>) -> Self {
        if let Some(pb) = pb {
            Self::UserSpecified(pb)
        } else {
            Self::CargoHome
        }
    }
}

/// Helper for constructing an index location, consisting of the remote url for
/// the index and the local location on disk
#[derive(Default)]
pub struct IndexLocation<'il> {
    /// The remote url of the registry index
    pub url: IndexUrl<'il>,
    /// The local disk path of the index
    pub root: IndexPath,
    /// The index location depends on the version of cargo used, as 1.85.0
    /// introduced a change to how the url is hashed. Not specifying the version
    /// will acquire the cargo version pertaining to the current environment.
    pub cargo_version: Option<crate::Version>,
}

impl<'il> IndexLocation<'il> {
    /// Constructs an index with the specified url located in the default cargo
    /// home
    pub fn new(url: IndexUrl<'il>) -> Self {
        Self {
            url,
            root: IndexPath::CargoHome,
            cargo_version: None,
        }
    }

    /// Changes the root location of the index on the local disk.
    ///
    /// If not called, or set to [`None`], the default cargo home disk location
    /// is used as the root
    pub fn with_root(mut self, root: Option<PathBuf>) -> Self {
        self.root = root.into();
        self
    }

    /// Obtains the full local disk path and URL of this index location
    pub fn into_parts(self) -> Result<(PathBuf, String), Error> {
        let url = self.url.as_str();

        let root = match self.root {
            IndexPath::CargoHome => crate::utils::cargo_home()?,
            IndexPath::UserSpecified(root) => root,
            IndexPath::Exact(path) => return Ok((path, url.to_owned())),
        };

        let vers = if let Some(v) = self.cargo_version {
            v
        } else {
            crate::utils::cargo_version(None)?
        };

        let stable = vers >= semver::Version::new(1, 85, 0);

        let (path, mut url) = crate::utils::get_index_details(url, Some(root), stable)?;

        if !url.ends_with('/') {
            url.push('/');
        }

        Ok((path, url))
    }
}

/// Calls the specified function for each cargo config located according to
/// cargo's standard hierarchical structure
///
/// Note that this only supports the use of `.cargo/config.toml`, which is not
/// supported below cargo 1.39.0
///
/// See <https://doc.rust-lang.org/cargo/reference/config.html#hierarchical-structure>
pub(crate) fn read_cargo_config<T>(
    root: Option<PathBuf>,
    cargo_home: Option<&Path>,
    callback: impl Fn(&Path, &toml_span::value::Value<'_>) -> Option<T>,
) -> Result<Option<T>, Error> {
    if let Some(mut path) = root.or_else(|| {
        std::env::current_dir()
            .ok()
            .and_then(|pb| PathBuf::from_path_buf(pb).ok())
    }) {
        loop {
            path.push(".cargo/config.toml");
            if path.exists() {
                let contents = match std::fs::read_to_string(&path) {
                    Ok(c) => c,
                    Err(err) => return Err(Error::IoPath(err, path)),
                };

                let toml = toml_span::parse(&contents).map_err(Box::new)?;
                if let Some(value) = callback(&path, &toml) {
                    return Ok(Some(value));
                }
            }
            path.pop();
            path.pop();

            // Walk up to the next potential config root
            if !path.pop() {
                break;
            }
        }
    }

    if let Some(home) = cargo_home
        .map(Cow::Borrowed)
        .or_else(|| crate::utils::cargo_home().ok().map(Cow::Owned))
    {
        let path = home.join("config.toml");
        if path.exists() {
            let fc = std::fs::read_to_string(&path)?;
            let toml = toml_span::parse(&fc).map_err(Box::new)?;
            if let Some(value) = callback(&path, &toml) {
                return Ok(Some(value));
            }
        }
    }

    Ok(None)
}

/// Gets the url of a replacement registry for the specified registry if one has been configured
///
/// See <https://doc.rust-lang.org/cargo/reference/source-replacement.html>
#[inline]
pub(crate) fn get_source_replacement<'iu>(
    root: Option<PathBuf>,
    cargo_home: Option<&Path>,
    registry_url: &str,
    registry_name: &str,
) -> Result<Option<IndexUrl<'iu>>, Error> {
    read_cargo_config(root, cargo_home, |config_path, config| {
        let sources = config.as_table()?.get("source")?.as_table()?;
        let repw = sources.iter().find_map(|(source_name, source)| {
            let source = source.as_table()?;

            let matches = registry_name == source_name.name
                || registry_url == source.get("registry")?.as_str()?;
            matches.then_some(source.get("replace-with")?.as_str()?)
        })?;

        let sources = config.pointer("/source")?.as_table()?;
        let replace_src = sources.get(repw)?.as_table()?;

        if let Some(rr) = replace_src.get("registry") {
            rr.as_str()
                .map(|r| IndexUrl::NonCratesIo(r.to_owned().into()))
        } else if let Some(rlr) = replace_src.get("local-registry") {
            let rel_path = rlr.as_str()?;
            Some(IndexUrl::Local(
                config_path.parent()?.parent()?.join(rel_path).into(),
            ))
        } else {
            None
        }
    })
}

#[cfg(test)]
mod test {
    // Current stable is 1.70.0
    #[test]
    fn opens_sparse() {
        assert!(std::env::var_os("CARGO_REGISTRIES_CRATES_IO_PROTOCOL").is_none());
        assert!(matches!(
            crate::index::ComboIndexCache::new(super::IndexLocation::new(
                super::IndexUrl::crates_io(None, None, None).unwrap()
            ))
            .unwrap(),
            crate::index::ComboIndexCache::Sparse(_)
        ));
    }

    /// Verifies we can parse .cargo/config.toml files to either use the crates-io
    /// protocol set, or source replacements
    #[test]
    fn parses_from_file() {
        assert!(std::env::var_os("CARGO_REGISTRIES_CRATES_IO_PROTOCOL").is_none());

        let td = tempfile::tempdir().unwrap();
        let root = crate::PathBuf::from_path_buf(td.path().to_owned()).unwrap();
        let cfg_toml = td.path().join(".cargo/config.toml");

        std::fs::create_dir_all(cfg_toml.parent().unwrap()).unwrap();

        const GIT: &str = r#"[registries.crates-io]
protocol = "git"
"#;

        // First just set the protocol from the sparse default to git
        std::fs::write(&cfg_toml, GIT).unwrap();

        let iurl = super::IndexUrl::crates_io(Some(root.clone()), None, None).unwrap();
        assert_eq!(iurl.as_str(), crate::CRATES_IO_INDEX);
        assert!(!iurl.is_sparse());

        // Next set replacement registries
        for (i, (kind, url)) in [
            (
                "registry",
                "sparse+https://sparse-registry-parses-from-file.com",
            ),
            ("registry", "https://sparse-registry-parses-from-file.git"),
            ("local-registry", root.as_str()),
        ]
        .iter()
        .enumerate()
        {
            std::fs::write(&cfg_toml, format!("{GIT}\n[source.crates-io]\nreplace-with = 'replacement'\n[source.replacement]\n{kind} = '{url}'")).unwrap();

            let iurl = super::IndexUrl::crates_io(Some(root.clone()), None, None).unwrap();
            assert_eq!(i == 0, iurl.is_sparse());
            assert_eq!(iurl.as_str(), *url);
        }
    }

    #[test]
    #[allow(unsafe_code)]
    fn custom() {
        assert!(std::env::var_os("CARGO_REGISTRIES_TAME_INDEX_TEST_INDEX").is_none());

        let td = tempfile::tempdir().unwrap();
        let root = crate::PathBuf::from_path_buf(td.path().to_owned()).unwrap();
        let cfg_toml = td.path().join(".cargo/config.toml");

        std::fs::create_dir_all(cfg_toml.parent().unwrap()).unwrap();

        const SPARSE: &str = r#"[registries.tame-index-test]
index = "sparse+https://some-url.com"
"#;

        const GIT: &str = r#"[registries.tame-index-test]
        index = "https://some-url.com"
        "#;

        unsafe {
            std::fs::write(&cfg_toml, SPARSE).unwrap();

            let iurl =
                super::IndexUrl::for_registry_name(Some(root.clone()), None, "tame-index-test")
                    .unwrap();
            assert_eq!(iurl.as_str(), "sparse+https://some-url.com");
            assert!(iurl.is_sparse());

            std::env::set_var(
                "CARGO_REGISTRIES_TAME_INDEX_TEST_INDEX",
                "sparse+https://some-other-url.com",
            );

            let iurl =
                super::IndexUrl::for_registry_name(Some(root.clone()), None, "tame-index-test")
                    .unwrap();
            assert_eq!(iurl.as_str(), "sparse+https://some-other-url.com");
            assert!(iurl.is_sparse());

            std::env::remove_var("CARGO_REGISTRIES_TAME_INDEX_TEST_INDEX");
        }

        unsafe {
            std::fs::write(&cfg_toml, GIT).unwrap();

            let iurl =
                super::IndexUrl::for_registry_name(Some(root.clone()), None, "tame-index-test")
                    .unwrap();
            assert_eq!(iurl.as_str(), "https://some-url.com");
            assert!(!iurl.is_sparse());

            std::env::set_var(
                "CARGO_REGISTRIES_TAME_INDEX_TEST_INDEX",
                "https://some-other-url.com",
            );

            let iurl =
                super::IndexUrl::for_registry_name(Some(root.clone()), None, "tame-index-test")
                    .unwrap();
            assert_eq!(iurl.as_str(), "https://some-other-url.com");
            assert!(!iurl.is_sparse());

            std::env::remove_var("CARGO_REGISTRIES_TAME_INDEX_TEST_INDEX");
        }

        #[allow(unused_variables)]
        {
            let err = crate::Error::UnknownRegistry("non-existant".to_owned());
            assert!(matches!(
                super::IndexUrl::for_registry_name(Some(root.clone()), None, "non-existant"),
                Err(err),
            ));
        }
    }
}
