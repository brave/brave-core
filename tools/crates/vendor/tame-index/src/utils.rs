//! Provides several useful functions for determining the disk location of a
//! remote registry index

use crate::{Error, InvalidUrl, InvalidUrlError, PathBuf};

pub mod flock;
#[cfg(feature = "__git")]
pub mod git;

/// Returns the storage directory (in utf-8) used by Cargo, often known as
/// `.cargo` or `CARGO_HOME`
#[inline]
pub fn cargo_home() -> Result<crate::PathBuf, crate::Error> {
    Ok(crate::PathBuf::from_path_buf(home::cargo_home()?)?)
}

/// Encodes a slice of bytes into a hexadecimal string to the specified buffer
pub(crate) fn encode_hex<'out, const I: usize, const O: usize>(
    input: &[u8; I],
    output: &'out mut [u8; O],
) -> &'out str {
    assert_eq!(I * 2, O);

    const CHARS: &[u8] = b"0123456789abcdef";

    for (i, &byte) in input.iter().enumerate() {
        let i = i * 2;
        output[i] = CHARS[(byte >> 4) as usize];
        output[i + 1] = CHARS[(byte & 0xf) as usize];
    }

    // SAFETY: we only emit ASCII hex characters
    #[allow(unsafe_code)]
    unsafe {
        std::str::from_utf8_unchecked(output)
    }
}

/// The details for a remote url
pub struct UrlDir {
    /// The unique directory name for the url
    pub dir_name: String,
    /// The canonical url for the remote url
    pub canonical: String,
}

/// Canonicalizes a `git+` url the same as cargo.
///
/// This is similar to cargo's `CanonicalUrl`, which previously was only used for
/// git+ url's, but since cargo 1.85.0 is now used as part of the hash for all
/// sources. Note that cargo removes queries and fragments _only_ from git+ URLs
/// and that happens before canonicalization, so this function does not handle them
/// specifically as we only care about sparse and git registry URLs
pub fn canonicalize_url(mut url: &str) -> Result<String, Error> {
    let scheme_ind = url.find("://").map(|i| i + 3).ok_or_else(|| InvalidUrl {
        url: url.to_owned(),
        source: InvalidUrlError::MissingScheme,
    })?;

    // Could use the Url crate for this, but it's simple enough and we don't
    // need to deal with every possible url (I hope...)
    let (host, path_length) = match url[scheme_ind..].find('/') {
        Some(end) => (
            &url[scheme_ind..scheme_ind + end],
            url.len() - (end + scheme_ind),
        ),
        None => (&url[scheme_ind..], 0),
    };

    // trim port
    let host = host.split(':').next().unwrap();

    if path_length > 1 && url.ends_with('/') {
        url = &url[..url.len() - 1];
    }

    if url.ends_with(".git") {
        url = &url[..url.len() - 4];
    }

    // cargo special cases github.com for reasons, so do the same
    Ok(if host == "github.com" {
        url.to_lowercase()
    } else {
        url.to_owned()
    })
}

/// Converts a url into a relative path and its canonical form
///
/// Cargo uses a small algorithm to create unique directory names for any url
/// so that they can be located in the same root without clashing
///
/// This function currently only supports 2 different URL kinds
///
/// * `(?:registry+)?<git registry url>`
/// * `sparse+<sparse registry url>`
#[allow(deprecated)]
pub fn url_to_local_dir(url: &str, stable: bool) -> Result<UrlDir, Error> {
    use std::hash::{Hash, Hasher, SipHasher};

    // This is extremely irritating, but we need to use usize for the kind, which
    // impacts the hash calculation, making it different based on pointer size.
    //
    // The reason for this is that cargo just uses #[derive(Hash)] for the SourceKind
    // https://github.com/rust-lang/cargo/blob/88b4b3bcd3bbb66873734d97ae412a6bcf9b75ee/crates/cargo-util-schemas/src/core/source_kind.rs#L4-L5,
    // which then uses https://doc.rust-lang.org/core/intrinsics/fn.discriminant_value.html
    // to get the discriminant and add to the hash...and that is pointer width :(
    //
    // Note that these are isize instead of usize because contrary to what one
    // would expect from the automatic discriminant assigned by rustc starting
    // at 0 and incrementing by 1 each time...it's actually signed, which can
    // be seen by overriding `Hasher::write_isize` and hashing a discriminant
    //
    // This is unfortunately a hard requirement because of https://github.com/rust-lang/rustc-stable-hash/blob/24e9848c89917abca155c8f854118e6d00ad4a30/src/stable_hasher.rs#L263-L299
    // where it specializes _only_ isize to only write a u8 if the value is less
    // than 0xff, something that doesn't happen for usize, which of course affects
    // the calculated hash
    const GIT_REGISTRY: isize = 2;
    const SPARSE_REGISTRY: isize = 3;

    // Ensure we have a registry or bare url
    let (url, scheme_ind, kind) = {
        let mut scheme_ind = url.find("://").ok_or_else(|| InvalidUrl {
            url: url.to_owned(),
            source: InvalidUrlError::MissingScheme,
        })?;

        let scheme_str = &url[..scheme_ind];

        let (url, kind) = match scheme_str.split_once('+') {
            Some(("sparse", _)) => (url, SPARSE_REGISTRY),
            // If there is no scheme modifier, assume git registry, same as cargo
            None => (url, GIT_REGISTRY),
            Some(("registry", _)) => {
                scheme_ind -= 9;
                (&url[9..], GIT_REGISTRY)
            }
            Some((_, _)) => {
                return Err(InvalidUrl {
                    url: url.to_owned(),
                    source: InvalidUrlError::UnknownSchemeModifier,
                }
                .into());
            }
        };

        (url, scheme_ind + 3, kind)
    };

    let (dir_name, url) = if stable {
        let canonical = canonicalize_url(url)?;

        let hash = {
            let mut hasher = rustc_stable_hash::StableSipHasher128::new();
            kind.hash(&mut hasher);
            canonical.hash(&mut hasher);
            Hasher::finish(&hasher)
        };

        let mut raw_ident = [0u8; 16];
        let ident = encode_hex(&hash.to_le_bytes(), &mut raw_ident);

        let dir_name = {
            let host = match url[scheme_ind..].find('/') {
                Some(end) => &url[scheme_ind..scheme_ind + end],
                None => &url[scheme_ind..],
            };

            // trim port
            let host = host.split(':').next().unwrap();
            host.split_once('@').map_or(host, |(_user, host)| host)
        };

        (format!("{dir_name}-{ident}"), canonical)
    } else {
        let hash = {
            let mut hasher = SipHasher::new();
            kind.hash(&mut hasher);
            url.hash(&mut hasher);
            hasher.finish()
        };
        let mut raw_ident = [0u8; 16];
        let ident = encode_hex(&hash.to_le_bytes(), &mut raw_ident);

        // Could use the Url crate for this, but it's simple enough and we don't
        // need to deal with every possible url (I hope...)
        let host = match url[scheme_ind..].find('/') {
            Some(end) => &url[scheme_ind..scheme_ind + end],
            None => &url[scheme_ind..],
        };

        // trim port
        let host = host.split(':').next().unwrap();
        let host = host.split_once('@').map_or(host, |(_user, host)| host);

        (format!("{host}-{ident}"), url.to_owned())
    };

    Ok(UrlDir {
        dir_name,
        canonical: url,
    })
}

/// Get the disk location of the specified url, as well as its canonical form
///
/// If not specified, the root directory is the user's default cargo home
pub fn get_index_details(
    url: &str,
    root: Option<PathBuf>,
    stable: bool,
) -> Result<(PathBuf, String), Error> {
    let url_dir = url_to_local_dir(url, stable)?;

    let mut path = match root {
        Some(path) => path,
        None => cargo_home()?,
    };

    path.push("registry");
    path.push("index");
    path.push(url_dir.dir_name);

    Ok((path, url_dir.canonical))
}

use std::io;

/// Parses the output of `cargo -V` to get the semver
///
/// This handles the 2? cases that I am aware of
///
/// 1. Official cargo prints `cargo <semver>(?:-<channel>)? (<sha1[..7]> <date>)`
/// 2. Non-official builds may drop the additional metadata and just print `cargo <semver>`
#[inline]
fn parse_cargo_semver(s: &str) -> Result<semver::Version, Error> {
    let semver = s.trim().split(' ').nth(1).ok_or_else(|| {
        io::Error::new(
            io::ErrorKind::InvalidData,
            "cargo version information was in an invalid format",
        )
    })?;

    Ok(semver.parse()?)
}

/// Retrieves the current version of cargo being used
pub fn cargo_version(working_dir: Option<&crate::Path>) -> Result<crate::Version, Error> {
    let mut cargo = std::process::Command::new(
        std::env::var_os("CARGO")
            .as_deref()
            .unwrap_or(std::ffi::OsStr::new("cargo")),
    );

    cargo.arg("-V");

    if let Some(wd) = working_dir {
        cargo.current_dir(wd);
    }

    cargo.stdout(std::process::Stdio::piped());

    let output = cargo.output()?;
    if !output.status.success() {
        return Err(io::Error::other("failed to request cargo version information").into());
    }

    let stdout = String::from_utf8(output.stdout)
        .map_err(|err| io::Error::new(io::ErrorKind::InvalidData, err))?;

    parse_cargo_semver(&stdout)
}

#[cfg(test)]
mod test {
    use super::{get_index_details, url_to_local_dir};
    use crate::PathBuf;

    #[test]
    #[cfg(all(target_pointer_width = "64", target_endian = "little"))]
    fn matches_cargo() {
        assert_eq!(
            get_index_details(crate::CRATES_IO_INDEX, Some(PathBuf::new()), false).unwrap(),
            (
                "registry/index/github.com-1ecc6299db9ec823".into(),
                crate::CRATES_IO_INDEX.to_owned()
            )
        );

        assert_eq!(
            get_index_details(crate::CRATES_IO_HTTP_INDEX, Some(PathBuf::new()), false).unwrap(),
            (
                "registry/index/index.crates.io-6f17d22bba15001f".into(),
                crate::CRATES_IO_HTTP_INDEX.to_owned(),
            )
        );

        const NON_CRATES_IO_GITHUB: &str = "https://github.com/EmbarkStudios/cargo-test-index";
        assert_eq!(
            get_index_details(NON_CRATES_IO_GITHUB, Some(PathBuf::new()), false).unwrap(),
            (
                "registry/index/github.com-655148e0a865c9e0".into(),
                NON_CRATES_IO_GITHUB.to_owned(),
            )
        );

        const NON_GITHUB_INDEX: &str =
            "https://dl.cloudsmith.io/public/embark/deny/cargo/index.git";
        assert_eq!(
            get_index_details(NON_GITHUB_INDEX, Some(PathBuf::new()), false).unwrap(),
            (
                "registry/index/dl.cloudsmith.io-955e041deb7d37e6".into(),
                NON_GITHUB_INDEX.to_owned(),
            )
        );

        // Just verifies that any non git+ or sparse+ url is treated as a git
        // registry for purposes of hashing
        const FAKE_REGISTRY: &str = "https://github.com/RustSec/advisory-db";

        assert_eq!(
            url_to_local_dir(FAKE_REGISTRY, false).unwrap().dir_name,
            "github.com-a946fc29ac602819"
        );
    }

    #[test]
    fn matches_cargo_1850() {
        assert_eq!(
            get_index_details(crate::CRATES_IO_HTTP_INDEX, Some(PathBuf::new()), true).unwrap(),
            (
                "registry/index/index.crates.io-1949cf8c6b5b557f".into(),
                crate::CRATES_IO_HTTP_INDEX.to_owned(),
            )
        );
        assert_eq!(
            get_index_details(crate::CRATES_IO_INDEX, Some(PathBuf::new()), true).unwrap(),
            (
                "registry/index/github.com-25cdd57fae9f0462".into(),
                crate::CRATES_IO_INDEX.to_owned(),
            )
        );
        assert_eq!(
            get_index_details(
                "https://github.com/EmbarkStudios/cargo-test-index",
                Some(PathBuf::new()),
                true
            )
            .unwrap(),
            (
                "registry/index/github.com-513223c940e0f1e9".into(),
                "https://github.com/embarkstudios/cargo-test-index".to_owned(),
            )
        );

        assert_eq!(
            get_index_details(
                "sparse+https://cargo.cloudsmith.io/embark/deny/",
                Some(PathBuf::new()),
                true
            )
            .unwrap(),
            (
                "registry/index/cargo.cloudsmith.io-2fc1f5411e6e72fd".into(),
                "sparse+https://cargo.cloudsmith.io/embark/deny".to_owned(),
            )
        );
    }

    #[test]
    #[cfg(all(target_pointer_width = "32", target_endian = "little"))]
    fn matches_cargo_32bit() {
        assert_eq!(
            get_index_details(crate::CRATES_IO_HTTP_INDEX, Some(PathBuf::new()), false).unwrap(),
            (
                "registry/index/index.crates.io-1cd66030c949c28d".into(),
                crate::CRATES_IO_HTTP_INDEX.to_owned(),
            )
        );
    }

    #[test]
    fn gets_cargo_version() {
        const MINIMUM: semver::Version = semver::Version::new(1, 70, 0);
        let version = super::cargo_version(None).unwrap();
        assert!(version >= MINIMUM);
    }

    #[test]
    fn parses_cargo_semver() {
        use super::parse_cargo_semver as pcs;

        assert_eq!(
            pcs("cargo 1.71.0 (cfd3bbd8f 2023-06-08)\n").unwrap(),
            semver::Version::new(1, 71, 0)
        );
        assert_eq!(
            pcs("cargo 1.73.0-nightly (7ac9416d8 2023-07-24)\n").unwrap(),
            "1.73.0-nightly".parse().unwrap()
        );
        assert_eq!(
            pcs("cargo 1.70.0\n").unwrap(),
            semver::Version::new(1, 70, 0)
        );
    }
}
