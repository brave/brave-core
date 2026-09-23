//! Utilities for finding and installing binaries that we depend on.

use anyhow::{anyhow, bail, Context, Result};
use fs4::FileExt;
use siphasher::sip::SipHasher13;
use std::collections::HashSet;
use std::env;
use std::fs;
use std::fs::File;
use std::hash::{Hash, Hasher};
use std::io;
use std::path::{Path, PathBuf};

/// Global cache for wasm-pack, currently containing binaries downloaded from
/// urls like wasm-bindgen and such.
#[derive(Debug)]
pub struct Cache {
    pub destination: PathBuf,
}

/// Representation of a downloaded tarball/zip
#[derive(Debug, Clone)]
pub struct Download {
    root: PathBuf,
}

impl Cache {
    /// Returns the global cache directory, as inferred from env vars and such.
    ///
    /// This function may return an error if a cache directory cannot be
    /// determined.
    pub fn new(name: &str) -> Result<Cache> {
        let cache_name = format!(".{}", name);
        let destination = dirs_next::cache_dir()
            .map(|p| p.join(&cache_name))
            .or_else(|| {
                let home = dirs_next::home_dir()?;
                Some(home.join(&cache_name))
            })
            .ok_or_else(|| anyhow!("couldn't find your home directory, is $HOME not set?"))?;
        if !destination.exists() {
            fs::create_dir_all(&destination)?;
        }
        Ok(Cache::at(&destination))
    }

    /// Creates a new cache specifically at a particular directory, useful in
    /// testing and such.
    pub fn at(path: &Path) -> Cache {
        Cache {
            destination: path.to_path_buf(),
        }
    }

    /// Joins a path to the destination of this cache, returning the result
    pub fn join(&self, path: &Path) -> PathBuf {
        self.destination.join(path)
    }

    /// Downloads a tarball or zip file from the specified url, extracting it
    /// to a directory with the version number and returning the directory that
    /// the contents were extracted into.
    ///
    /// Note that this function requries that the contents of `url` never change
    /// as the contents of the url are globally cached on the system and never
    /// invalidated.
    ///
    /// The `name` is a human-readable name used to go into the folder name of
    /// the destination, and `binaries` is a list of binaries expected to be at
    /// the url. If the URL's extraction doesn't contain all the binaries this
    /// function will return an error.
    pub fn download_version(
        &self,
        install_permitted: bool,
        name: &str,
        binaries: &[&str],
        url: &str,
        version: &str,
    ) -> Result<Option<Download>> {
        self._download(install_permitted, name, binaries, url, Some(version))
    }

    /// Downloads a tarball or zip file from the specified url, extracting it
    /// locally and returning the directory that the contents were extracted
    /// into.
    ///
    /// Note that this function requries that the contents of `url` never change
    /// as the contents of the url are globally cached on the system and never
    /// invalidated.
    ///
    /// The `name` is a human-readable name used to go into the folder name of
    /// the destination, and `binaries` is a list of binaries expected to be at
    /// the url. If the URL's extraction doesn't contain all the binaries this
    /// function will return an error.
    pub fn download(
        &self,
        install_permitted: bool,
        name: &str,
        binaries: &[&str],
        url: &str,
    ) -> Result<Option<Download>> {
        self._download(install_permitted, name, binaries, url, None)
    }

    fn _download(
        &self,
        install_permitted: bool,
        name: &str,
        binaries: &[&str],
        url: &str,
        version: Option<&str>,
    ) -> Result<Option<Download>> {
        let dirname = match version {
            Some(version) => get_dirname(name, version),
            None => hashed_dirname(url, name),
        };

        let destination = self.destination.join(&dirname);

        let flock = File::create(self.destination.join(&format!(".{}.lock", dirname)))?;
        flock.lock_exclusive()?;

        if destination.exists() {
            return Ok(Some(Download { root: destination }));
        }

        if !install_permitted {
            return Ok(None);
        }

        let data =
            download_binary(&url).with_context(|| format!("failed to download from {}", url))?;

        // Extract everything in a temporary directory in case we're ctrl-c'd.
        // Don't want to leave around corrupted data!
        let temp = self.destination.join(&format!(".{}", dirname));
        drop(fs::remove_dir_all(&temp));
        fs::create_dir_all(&temp)?;

        if url.ends_with(".tar.gz") {
            self.extract_tarball(&data, &temp, binaries)
                .with_context(|| format!("failed to extract tarball from {}", url))?;
        } else if url.ends_with(".zip") {
            self.extract_zip(&data, &temp, binaries)
                .with_context(|| format!("failed to extract zip from {}", url))?;
        } else {
            // panic instead of runtime error as it's a static violation to
            // download a different kind of url, all urls should be encoded into
            // the binary anyway
            panic!("don't know how to extract {}", url)
        }

        // Now that everything is ready move this over to our destination and
        // we're good to go.
        fs::rename(&temp, &destination)?;

        flock.unlock()?;
        Ok(Some(Download { root: destination }))
    }

    /// Downloads a tarball from the specified url, extracting it locally and
    /// returning the directory that the contents were extracted into.
    ///
    /// Similar to download; use this function for languages that doesn't emit a
    /// binary.
    pub fn download_artifact(&self, name: &str, url: &str) -> Result<Option<Download>> {
        self._download_artifact(name, url, None)
    }

    /// Downloads a tarball from the specified url, extracting it locally and
    /// returning the directory that the contents were extracted into.
    ///
    /// Similar to download; use this function for languages that doesn't emit a
    /// binary.
    pub fn download_artifact_version(
        &self,
        name: &str,
        url: &str,
        version: &str,
    ) -> Result<Option<Download>> {
        self._download_artifact(name, url, Some(version))
    }

    fn _download_artifact(
        &self,
        name: &str,
        url: &str,
        version: Option<&str>,
    ) -> Result<Option<Download>> {
        let dirname = match version {
            Some(version) => get_dirname(name, version),
            None => hashed_dirname(url, name),
        };
        let destination = self.destination.join(&dirname);

        if destination.exists() {
            return Ok(Some(Download { root: destination }));
        }

        let data =
            download_binary(&url).with_context(|| format!("failed to download from {}", url))?;

        // Extract everything in a temporary directory in case we're ctrl-c'd.
        // Don't want to leave around corrupted data!
        let temp = self.destination.join(&format!(".{}", &dirname));
        drop(fs::remove_dir_all(&temp));
        fs::create_dir_all(&temp)?;

        if url.ends_with(".tar.gz") {
            self.extract_tarball_all(&data, &temp)
                .with_context(|| format!("failed to extract tarball from {}", url))?;
        } else {
            // panic instead of runtime error as it's a static violation to
            // download a different kind of url, all urls should be encoded into
            // the binary anyway
            panic!("don't know how to extract {}", url)
        }

        // Now that everything is ready move this over to our destination and
        // we're good to go.
        fs::rename(&temp, &destination)?;
        Ok(Some(Download { root: destination }))
    }

    /// simiar to extract_tarball, but preserves all the archive's content.
    fn extract_tarball_all(&self, tarball: &[u8], dst: &Path) -> Result<()> {
        let mut archive = tar::Archive::new(flate2::read::GzDecoder::new(tarball));

        for entry in archive.entries()? {
            let mut entry = entry?;
            let dest = match entry.path()?.file_stem() {
                Some(_) => dst.join(entry.path()?.file_name().unwrap()),
                _ => continue,
            };
            entry.unpack(dest)?;
        }

        Ok(())
    }

    fn extract_tarball(&self, tarball: &[u8], dst: &Path, binaries: &[&str]) -> Result<()> {
        let mut binaries: HashSet<_> = binaries.iter().copied().collect();
        let mut archive = tar::Archive::new(flate2::read::GzDecoder::new(tarball));

        for entry in archive.entries()? {
            let mut entry = entry?;

            let dest = match self.extract_binary(&entry.path()?, dst, &mut binaries) {
                Some(dest) => dest,
                _ => continue,
            };

            fs::create_dir_all(
                dest.parent().ok_or_else(|| {
                    anyhow!("could not get parent directory of {}", dest.display())
                })?,
            )?;

            entry.unpack(dest)?;
        }

        if !binaries.is_empty() {
            bail!(
                "the tarball was missing expected executables: {}",
                binaries
                    .iter()
                    .map(|s| s.to_string())
                    .collect::<Vec<_>>()
                    .join(", "),
            )
        }

        Ok(())
    }

    fn extract_zip(&self, zip: &[u8], dst: &Path, binaries: &[&str]) -> Result<()> {
        let mut binaries: HashSet<_> = binaries.iter().copied().collect();

        let data = io::Cursor::new(zip);
        let mut zip = zip::ZipArchive::new(data)?;

        for i in 0..zip.len() {
            let mut entry = zip.by_index(i).unwrap();
            let entry_path = match entry.enclosed_name() {
                Some(path) => path,
                None => continue,
            };

            let dest = match self.extract_binary(&entry_path, dst, &mut binaries) {
                Some(dest) => dest,
                _ => continue,
            };

            fs::create_dir_all(
                dest.parent().ok_or_else(|| {
                    anyhow!("could not get parent directory of {}", dest.display())
                })?,
            )?;

            let mut dest = bin_open_options().write(true).create_new(true).open(dest)?;
            io::copy(&mut entry, &mut dest)?;
        }

        if !binaries.is_empty() {
            bail!(
                "the zip was missing expected executables: {}",
                binaries
                    .iter()
                    .map(|s| s.to_string())
                    .collect::<Vec<_>>()
                    .join(", "),
            )
        }

        return Ok(());

        #[cfg(unix)]
        fn bin_open_options() -> fs::OpenOptions {
            use std::os::unix::fs::OpenOptionsExt;

            let mut opts = fs::OpenOptions::new();
            opts.mode(0o755);
            opts
        }

        #[cfg(not(unix))]
        fn bin_open_options() -> fs::OpenOptions {
            fs::OpenOptions::new()
        }
    }

    /// Works out whether or not to extract a given file from an archive.
    ///
    /// If a file should be extracted, this function removes its corresponding
    /// entry from `binaries`, and returns the destination path where the file should be
    /// extracted to.
    fn extract_binary(
        &self,
        entry_path: &Path,
        dst: &Path,
        binaries: &mut HashSet<&str>,
    ) -> Option<PathBuf> {
        let file_stem = entry_path.file_stem()?;

        for &binary in binaries.iter() {
            if binary == file_stem {
                binaries.remove(binary);
                return Some(dst.join(entry_path.file_name()?));
            } else if binary.contains('/') && entry_path.ends_with(binary) {
                binaries.remove(binary);
                return Some(dst.join(binary));
            }
        }
        None
    }
}

impl Download {
    /// Manually constructs a download at the specified path
    pub fn at(path: &Path) -> Download {
        Download {
            root: path.to_path_buf(),
        }
    }

    /// Returns the path to the binary `name` within this download
    pub fn binary(&self, name: &str) -> Result<PathBuf> {
        use is_executable::IsExecutable;

        let ret = self
            .root
            .join(name)
            .with_extension(env::consts::EXE_EXTENSION);

        if !ret.is_file() {
            bail!("{} binary does not exist", ret.display());
        }
        if !ret.is_executable() {
            bail!("{} is not executable", ret.display());
        }

        Ok(ret)
    }

    /// Returns the path to the root
    pub fn path(&self) -> PathBuf {
        self.root.clone()
    }
}

fn download_binary(url: &str) -> Result<Vec<u8>> {
    let response = ureq::get(url).call()?;

    let status_code = response.status();

    if (200..300).contains(&status_code) {
        // note malicious server might exhaust our memory
        let len: usize = response
            .header("Content-Length")
            .and_then(|s| s.parse().ok())
            .unwrap_or(0);
        let mut bytes: Vec<u8> = Vec::with_capacity(len);
        response.into_reader().read_to_end(&mut bytes)?;
        Ok(bytes)
    } else {
        bail!(
            "received a bad HTTP status code ({}) when requesting {}",
            status_code,
            url
        )
    }
}

fn get_dirname(name: &str, suffix: &str) -> String {
    format!("{}-{}", name, suffix)
}

fn hashed_dirname(url: &str, name: &str) -> String {
    let mut hasher = SipHasher13::new();
    url.hash(&mut hasher);
    let result = hasher.finish();
    let hex = hex::encode(&[
        (result >> 0) as u8,
        (result >> 8) as u8,
        (result >> 16) as u8,
        (result >> 24) as u8,
        (result >> 32) as u8,
        (result >> 40) as u8,
        (result >> 48) as u8,
        (result >> 56) as u8,
    ]);
    format!("{}-{}", name, hex)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_returns_same_hash_for_same_name_and_url() {
        let name = "wasm-pack";
        let url = "http://localhost:7878/wasm-pack-v0.6.0.tar.gz";

        let first = hashed_dirname(url, name);
        let second = hashed_dirname(url, name);

        assert!(!first.is_empty());
        assert!(!second.is_empty());
        assert_eq!(first, second);
    }

    #[test]
    fn it_returns_different_hashes_for_different_urls() {
        let name = "wasm-pack";
        let url = "http://localhost:7878/wasm-pack-v0.5.1.tar.gz";
        let second_url = "http://localhost:7878/wasm-pack-v0.6.0.tar.gz";

        let first = hashed_dirname(url, name);
        let second = hashed_dirname(second_url, name);

        assert_ne!(first, second);
    }

    #[test]
    fn it_returns_same_dirname_for_same_name_and_version() {
        let name = "wasm-pack";
        let version = "0.6.0";

        let first = get_dirname(name, version);
        let second = get_dirname(name, version);

        assert!(!first.is_empty());
        assert!(!second.is_empty());
        assert_eq!(first, second);
    }

    #[test]
    fn it_returns_different_dirnames_for_different_versions() {
        let name = "wasm-pack";
        let version = "0.5.1";
        let second_version = "0.6.0";

        let first = get_dirname(name, version);
        let second = get_dirname(name, second_version);

        assert_ne!(first, second);
    }

    #[test]
    fn it_returns_cache_dir() {
        let name = "wasm-pack";
        let cache = Cache::new(name);

        let expected = dirs_next::cache_dir()
            .unwrap()
            .join(PathBuf::from(".".to_owned() + name));

        assert!(cache.is_ok());
        assert_eq!(cache.unwrap().destination, expected);
    }

    #[test]
    fn it_returns_destination_if_binary_already_exists() {
        use std::fs;

        let binary_name = "wasm-pack";
        let binaries = vec![binary_name];

        let dir = tempfile::TempDir::new().unwrap();
        let cache = Cache::at(dir.path());
        let version = "0.6.0";
        let url = &format!(
            "{}/{}/v{}.tar.gz",
            "http://localhost:7878", binary_name, version
        );

        let dirname = get_dirname(&binary_name, &version);
        let full_path = dir.path().join(dirname);

        // Create temporary directory and binary to simulate that
        // a cached binary already exists.
        fs::create_dir_all(full_path).unwrap();

        let dl = cache.download_version(true, binary_name, &binaries, url, version);

        assert!(dl.is_ok());
        assert!(dl.unwrap().is_some())
    }
}
