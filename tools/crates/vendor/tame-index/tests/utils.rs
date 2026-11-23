#![allow(dead_code)]
#![allow(missing_docs)]

pub use tame_index::{IndexKrate, Path, PathBuf};

pub struct TempDir {
    pub td: tempfile::TempDir,
}

impl TempDir {
    #[inline]
    pub fn path(&self) -> &Path {
        Path::from_path(self.td.path()).unwrap()
    }
}

impl Default for TempDir {
    #[inline]
    fn default() -> Self {
        Self {
            td: tempfile::TempDir::new_in(env!("CARGO_TARGET_TMPDIR")).unwrap(),
        }
    }
}

impl AsRef<Path> for TempDir {
    #[inline]
    fn as_ref(&self) -> &Path {
        self.path()
    }
}

impl AsRef<std::path::Path> for TempDir {
    #[inline]
    fn as_ref(&self) -> &std::path::Path {
        self.td.path()
    }
}

impl<'td> From<&'td TempDir> for PathBuf {
    fn from(td: &'td TempDir) -> Self {
        td.path().to_owned()
    }
}

#[inline]
pub fn tempdir() -> TempDir {
    TempDir::default()
}

#[inline]
pub fn unlocked() -> tame_index::index::FileLock {
    tame_index::index::FileLock::unlocked()
}

pub fn fake_krate(name: &str, num_versions: u8) -> IndexKrate {
    assert!(num_versions > 0);
    let mut version = semver::Version::new(0, 0, 0);
    let mut versions = Vec::new();

    for v in 0..num_versions {
        match v % 3 {
            0 => version.patch += 1,
            1 => {
                version.patch = 0;
                version.minor += 1;
            }
            2 => {
                version.patch = 0;
                version.minor = 0;
                version.major += 1;
            }
            _ => unreachable!(),
        }

        let iv = tame_index::IndexVersion::fake(name, version.to_string());
        versions.push(iv);
    }

    IndexKrate { versions }
}

#[cfg(feature = "sparse")]
pub fn tls_config() -> rustls::ClientConfig {
    let rcs: rustls::RootCertStore = webpki_roots::TLS_SERVER_ROOTS.iter().cloned().collect();
    rustls::ClientConfig::builder_with_provider(std::sync::Arc::new(
        rustls::crypto::ring::default_provider(),
    ))
    .with_protocol_versions(rustls::DEFAULT_VERSIONS)
    .unwrap()
    .with_root_certificates(rcs)
    .with_no_client_auth()
}

#[cfg(feature = "sparse")]
pub fn blocking_client() -> reqwest::blocking::Client {
    reqwest::blocking::Client::builder()
        .tls_backend_preconfigured(tls_config())
        .build()
        .unwrap()
}

#[cfg(feature = "sparse")]
pub fn async_client() -> reqwest::Client {
    reqwest::Client::builder()
        .tls_backend_preconfigured(tls_config())
        .build()
        .unwrap()
}
