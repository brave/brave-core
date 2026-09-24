//! An efficient way to check whether a given package has been yanked
use std::{
    collections::{BTreeSet, HashMap},
    time::Duration,
};

use crate::{
    error::{Error, ErrorKind},
    package::{self, Package},
};

pub use tame_index::external::reqwest::ClientBuilder;
use tame_index::utils::flock::{FileLock, LockOptions};

enum Index {
    Git(tame_index::index::RemoteGitIndex),
    SparseCached(tame_index::index::SparseIndex),
    SparseRemote(tame_index::index::AsyncRemoteSparseIndex),
}

impl Index {
    #[inline]
    fn krate(
        &self,
        name: &package::Name,
        lock: &FileLock,
    ) -> Result<Option<tame_index::IndexKrate>, Error> {
        let name = name.as_str().try_into().map_err(Error::from_tame)?;
        let res = match self {
            Self::Git(gi) => gi.krate(name, true, lock),
            Self::SparseCached(si) => si.cached_krate(name, lock),
            Self::SparseRemote(rsi) => rsi.cached_krate(name, lock),
        }
        .map_err(Error::from_tame)?;

        Ok(res)
    }
}

/// Provides an efficient way to check if the given package has been yanked.
///
/// Operations on crates.io index are rather slow.
/// Instead of peforming an index lookup for every version of every crate,
/// this implementation looks up each crate only once and caches the result in memory.
///
/// Please note that this struct will hold a global Cargo package lock while it exists.
/// Cargo operations that download crates (e.g. `cargo update` or even `cargo build`)
/// will not be possible while this lock is held.
pub struct CachedIndex {
    index: Index,
    /// The inner hash map is logically HashMap<Version, IsYanked>
    /// but we don't parse semver because crates.io registry contains invalid semver:
    /// <https://github.com/rustsec/rustsec/issues/759>
    cache: HashMap<package::Name, Result<Option<HashMap<String, bool>>, Error>>,
    /// The lock we hold on the Cargo cache directory
    lock: FileLock,
}

impl CachedIndex {
    /// Open the local crates.io index
    ///
    /// If this opens a git index, it will perform a fetch to get the latest index
    /// information.
    ///
    /// If this is a sparse index, it will be downloaded later on demand.
    ///
    /// ## Locking
    ///
    /// This function will wait for up to `lock_timeout` for the filesystem lock on the repository.
    /// It will fail with [`rustsec::Error::LockTimeout`](Error) if the lock is still held
    /// after that time.
    ///
    /// If `lock_timeout` is set to `std::time::Duration::from_secs(0)`, it will not wait at all,
    /// and instead return an error immediately if it fails to aquire the lock.
    pub fn fetch(lock_timeout: Duration) -> Result<Self, Error> {
        Self::fetch_inner(None, lock_timeout).map_err(Error::from_tame)
    }

    fn fetch_inner(
        client: Option<ClientBuilder>,
        lock_timeout: Duration,
    ) -> Result<Self, tame_index::Error> {
        let index = tame_index::index::ComboIndexCache::new(tame_index::IndexLocation::new(
            tame_index::IndexUrl::crates_io(None, None, None)?,
        ))?;

        let lock = acquire_cargo_package_lock(lock_timeout)?;

        let index = match index {
            tame_index::index::ComboIndexCache::Git(gi) => {
                let mut rgi = tame_index::index::RemoteGitIndex::new(gi, &lock)?;
                rgi.fetch(&lock)?;
                Index::Git(rgi)
            }
            tame_index::index::ComboIndexCache::Sparse(si) => {
                let client_builder = client.unwrap_or_default();
                // note: this would need to change if rustsec ever adds the capability
                // to query other indices that _might_ not support HTTP/2, but
                // hopefully that would never need to happen
                let client = client_builder.build().map_err(tame_index::Error::from)?;

                Index::SparseRemote(tame_index::index::AsyncRemoteSparseIndex::new(si, client))
            }
            _ => panic!("Unsupported crates.io index type"),
        };

        Ok(CachedIndex {
            index,
            cache: Default::default(),
            lock,
        })
    }

    /// Open the local crates.io index
    ///
    /// If this opens a git index, it allows reading of index entries from the repository.
    ///
    /// If this is a sparse index, it only allows reading of index entries that are already cached locally.
    ///
    /// ## Locking
    ///
    /// This function will wait for up to `lock_timeout` for the filesystem lock on the repository.
    /// It will fail with [`rustsec::Error::LockTimeout`](Error) if the lock is still held
    /// after that time.
    ///
    /// If `lock_timeout` is set to `std::time::Duration::from_secs(0)`, it will not wait at all,
    /// and instead return an error immediately if it fails to aquire the lock.
    pub fn open(lock_timeout: Duration) -> Result<Self, Error> {
        Self::open_inner(lock_timeout).map_err(Error::from_tame)
    }

    fn open_inner(lock_timeout: Duration) -> Result<Self, tame_index::Error> {
        let index = tame_index::index::ComboIndexCache::new(tame_index::IndexLocation::new(
            tame_index::IndexUrl::crates_io(None, None, None)?,
        ))?;

        let lock = acquire_cargo_package_lock(lock_timeout)?;

        let index = match index {
            tame_index::index::ComboIndexCache::Git(gi) => {
                let rgi = tame_index::index::RemoteGitIndex::new(gi, &lock)?;
                Index::Git(rgi)
            }
            tame_index::index::ComboIndexCache::Sparse(si) => Index::SparseCached(si),
            _ => panic!("Unsupported crates.io index type"),
        };

        Ok(CachedIndex {
            index,
            cache: Default::default(),
            lock,
        })
    }

    /// Populates the cache entries for all of the specified crates.
    fn populate_cache(&mut self, mut packages: BTreeSet<&package::Name>) -> Result<(), Error> {
        // only look up info on packages that aren't yet cached
        packages.retain(|pkg| !self.cache.contains_key(pkg));
        match &self.index {
            Index::Git(_) | Index::SparseCached(_) => {
                for pkg in packages {
                    self.insert(pkg.to_owned(), self.index.krate(pkg, &self.lock));
                }
            }
            Index::SparseRemote(rsi) => {
                // Ensure we have a runtime
                let rt = tame_index::external::tokio::runtime::Runtime::new().map_err(|err| {
                    Error::with_source(
                        ErrorKind::Registry,
                        "unable to start a tokio runtime".to_owned(),
                        err,
                    )
                })?;
                let _rt = rt.enter();

                /// This is the timeout per individual crate. If a crate fails to be
                /// requested for a retriable reason then it will be retried until
                /// this time limit is reached
                const REQUEST_TIMEOUT: Option<Duration> = Some(Duration::from_secs(10));

                let results = rsi
                    .krates_blocking(
                        packages
                            .into_iter()
                            .map(|p| p.as_str().to_owned())
                            .collect(),
                        true,
                        REQUEST_TIMEOUT,
                        &self.lock,
                    )
                    .map_err(|err| {
                        Error::with_source(
                            ErrorKind::Registry,
                            "unable to acquire tokio runtime".to_owned(),
                            err,
                        )
                    })?;

                for (name, res) in results {
                    self.insert(
                        name.parse().expect("this was a package name before"),
                        res.map_err(Error::from_tame),
                    );
                }
            }
        }

        Ok(())
    }

    #[inline]
    fn insert(
        &mut self,
        package: package::Name,
        krate_res: Result<Option<tame_index::IndexKrate>, Error>,
    ) {
        let krate_res = krate_res.map(|ik| {
            ik.map(|ik| {
                ik.versions
                    .into_iter()
                    .map(|v| (v.version.to_string(), v.is_yanked()))
                    .collect()
            })
        });

        self.cache.insert(package, krate_res);
    }

    /// Is the given package yanked?
    fn is_yanked(&mut self, package: &Package) -> Result<bool, Error> {
        if !self.cache.contains_key(&package.name) {
            self.insert(
                package.name.to_owned(),
                self.index.krate(&package.name, &self.lock),
            );
        }

        match &self.cache[&package.name] {
            Ok(Some(ik)) => match ik.get(&package.version.to_string()) {
                Some(is_yanked) => Ok(*is_yanked),
                None => Err(Error::new(
                    ErrorKind::NotFound,
                    format!(
                        "No such version in crates.io index: {} {}",
                        &package.name, &package.version
                    ),
                )),
            },
            Ok(None) => Err(Error::new(
                ErrorKind::NotFound,
                format!("No such crate in crates.io index: {}", &package.name),
            )),
            Err(err) => Err(err.clone()),
        }
    }

    /// Iterate over the provided packages, returning a vector of the
    /// packages which have been yanked.
    ///
    /// This function should be called with many packages at once rather than one by one;
    /// that way it can download the status of a large number of packages at once from the sparse index
    /// very quickly, orders of magnitude faster than requesting packages one by one.
    pub fn find_yanked<'a, I>(&mut self, packages: I) -> Vec<Result<&'a Package, Error>>
    where
        I: IntoIterator<Item = &'a Package>,
    {
        let mut yanked = Vec::new();

        let dedup_packages: BTreeSet<&Package> = packages.into_iter().collect();
        let package_names: BTreeSet<&package::Name> =
            dedup_packages.iter().map(|p| &p.name).collect();
        if let Err(e) = self.populate_cache(package_names) {
            yanked.push(Err(Error::with_source(
                ErrorKind::Registry,
                "Failed to download crates.io index. \
                    Data may be missing or stale when checking for yanked packages."
                    .to_owned(),
                e,
            )));
        }

        for package in dedup_packages {
            match self.is_yanked(package) {
                Ok(false) => {} // not yanked, nothing to report
                Ok(true) => yanked.push(Ok(package)),
                Err(error) => yanked.push(Err(error)),
            }
        }

        yanked
    }
}

// We cannot expose these publicly because that would leak the `tame_index` SemVer into the public API
fn acquire_cargo_package_lock(lock_timeout: Duration) -> Result<FileLock, tame_index::Error> {
    let lock_opts = LockOptions::cargo_package_lock(None)?.exclusive(false);
    acquire_lock(lock_opts, lock_timeout)
}

fn acquire_lock(
    lock_opts: LockOptions<'_>,
    lock_timeout: Duration,
) -> Result<FileLock, tame_index::Error> {
    if lock_timeout == Duration::from_secs(0) {
        lock_opts.try_lock()
    } else {
        lock_opts.lock(|_| Some(lock_timeout))
    }
}
