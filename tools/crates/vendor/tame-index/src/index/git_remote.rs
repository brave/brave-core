use super::{FileLock, GitIndex};
use crate::{Error, IndexKrate, KrateName, utils};
use std::sync::atomic::AtomicBool;

/// Uses a "bare" git index that fetches files directly from the repo instead of
/// using a local checkout, the same as cargo itself.
///
/// Uses cargo's cache
pub struct RemoteGitIndex {
    index: GitIndex,
    repo: gix::Repository,
    head_commit: gix::ObjectId,
}

const DIR: gix::remote::Direction = gix::remote::Direction::Fetch;

impl RemoteGitIndex {
    /// Creates a new [`Self`] that can access and write local cache entries,
    /// and contact the remote index to retrieve the latest index information
    ///
    /// Note that if a repository does not exist at the local disk path of the
    /// provided [`GitIndex`], a full clone will be performed.
    #[inline]
    pub fn new(index: GitIndex, lock: &FileLock) -> Result<Self, Error> {
        Self::with_options(
            index,
            gix::progress::Discard,
            &gix::interrupt::IS_INTERRUPTED,
            lock,
        )
    }

    /// Breaks [`Self`] into its component parts
    ///
    /// This method is useful if you need thread safe access to the repository
    #[inline]
    pub fn into_parts(self) -> (GitIndex, gix::Repository) {
        (self.index, self.repo)
    }

    /// Creates a new [`Self`] that allows showing of progress of the the potential
    /// fetch if the disk location is empty, as well as allowing interruption
    /// of the fetch operation.
    pub fn with_options<P>(
        mut index: GitIndex,
        progress: P,
        should_interrupt: &AtomicBool,
        _lock: &FileLock,
    ) -> Result<Self, Error>
    where
        P: gix::NestedProgress,
        P::SubProgress: 'static,
    {
        let open_or_clone_repo = || -> Result<_, GitError> {
            let mut mapping = gix::sec::trust::Mapping::default();
            let open_with_complete_config =
                gix::open::Options::default().permissions(gix::open::Permissions {
                    config: gix::open::permissions::Config {
                        // Be sure to get all configuration, some of which is only known by the git binary.
                        // That way we are sure to see all the systems credential helpers
                        git_binary: true,
                        ..Default::default()
                    },
                    ..Default::default()
                });

            mapping.reduced = open_with_complete_config.clone();
            mapping.full = open_with_complete_config.clone();

            // Attempt to open the repository, if it fails for any reason,
            // attempt to perform a fresh clone instead
            let repo = gix::ThreadSafeRepository::discover_opts(
                &index.cache.path,
                gix::discover::upwards::Options::default().apply_environment(),
                mapping,
            )
            .ok()
            .map(|repo| repo.to_thread_local())
            .filter(|repo| {
                // The `cargo` standard registry clone has no configured origin (when created with `git2`).
                repo.find_remote("origin").map_or(true, |remote| {
                    remote
                        .url(DIR)
                        .is_some_and(|remote_url| remote_url.to_bstring() == index.url)
                })
            })
            .or_else(|| gix::open_opts(&index.cache.path, open_with_complete_config).ok());

            let res = if let Some(repo) = repo {
                (repo, None)
            } else {
                // We need to create the directory chain ourselves, gix will fail
                // if any parent directory is missing
                if !index.cache.path.exists() {
                    std::fs::create_dir_all(&index.cache.path).map_err(|source| {
                        GitError::ClonePrep(Box::new(gix::clone::Error::Init(
                            gix::init::Error::Init(gix::create::Error::CreateDirectory {
                                source,
                                path: index.cache.path.clone().into(),
                            }),
                        )))
                    })?;
                }

                let (repo, out) = gix::prepare_clone_bare(index.url.as_str(), &index.cache.path)
                    .map_err(Box::new)?
                    .with_remote_name("origin")
                    .map_err(Box::new)?
                    .configure_remote(|remote| {
                        Ok(remote.with_refspecs(["+HEAD:refs/remotes/origin/HEAD"], DIR)?)
                    })
                    .fetch_only(progress, should_interrupt)
                    .map_err(|err| GitError::from(Box::new(err)))?;

                (repo, Some(out))
            };

            Ok(res)
        };

        let (mut repo, fetch_outcome) = open_or_clone_repo()?;

        if let Some(fetch_outcome) = fetch_outcome {
            utils::git::write_fetch_head(
                &repo,
                &fetch_outcome,
                &repo.find_remote("origin").unwrap(),
            )?;
        }

        repo.object_cache_size_if_unset(4 * 1024 * 1024);

        let head_commit = Self::set_head(&mut index, &repo)?;

        Ok(Self {
            repo,
            index,
            head_commit,
        })
    }

    /// Gets the local index
    #[inline]
    pub fn local(&self) -> &GitIndex {
        &self.index
    }

    /// Get the configuration of the index.
    ///
    /// See the [cargo docs](https://doc.rust-lang.org/cargo/reference/registry-index.html#index-configuration)
    pub fn index_config(&self) -> Result<super::IndexConfig, Error> {
        let blob = self.read_blob("config.json")?.ok_or_else(|| {
            Error::Io(std::io::Error::new(
                std::io::ErrorKind::NotFound,
                "unable to find config.json",
            ))
        })?;
        Ok(serde_json::from_slice(&blob.data)?)
    }

    /// Sets the head commit in the wrapped index so that cache entries can be
    /// properly filtered
    #[inline]
    fn set_head(index: &mut GitIndex, repo: &gix::Repository) -> Result<gix::ObjectId, Error> {
        let find_remote_head = || -> Result<gix::ObjectId, GitError> {
            const CANDIDATE_REFS: &[&str] = &[
                "FETCH_HEAD",    /* the location with the most-recent updates, as written by git2 */
                "origin/HEAD", /* typical refspecs update this symbolic ref to point to the actual remote ref with the fetched commit */
                "origin/master", /* for good measure, resolve this branch by hand in case origin/HEAD is broken */
                "HEAD",
            ];
            let mut candidates: Vec<_> = CANDIDATE_REFS
                .iter()
                .enumerate()
                .filter_map(|(i, refname)| {
                    let ref_id = repo
                        .find_reference(*refname)
                        .ok()?
                        .into_fully_peeled_id()
                        .ok()?;

                    let commit = ref_id.object().ok()?.try_into_commit().ok()?;
                    let commit_time = commit.time().ok()?.seconds;

                    Some((i, commit.id, commit_time))
                })
                .collect();

            // Sort from oldest to newest, the last one will be the best reference
            // we could reasonably locate, and since we are on second resolution,
            // prefer the ordering of candidates if times are equal.
            //
            // This allows FETCH_HEAD to be authoritative, unless one of the other
            // references is more up to date, which can occur in (at least) 2 scenarios:
            //
            // 1. The repo is a fresh clone by cargo either via git or libgit2,
            // neither of which write FETCH_HEAD during clone
            // 2. A fetch was performed by an external crate/program to cargo or
            // ourselves that didn't update FETCH_HEAD
            candidates.sort_by(|a, b| match a.2.cmp(&b.2) {
                std::cmp::Ordering::Equal => b.0.cmp(&a.0),
                o => o,
            });

            // get the most recent commit, the one with most time passed since unix epoch.
            Ok(candidates
                .last()
                .ok_or_else(|| GitError::UnableToFindRemoteHead)?
                .1)
        };

        let sha1 = utils::git::unwrap_sha1(find_remote_head()?);
        index.set_head_commit(Some(sha1));

        Ok(gix::ObjectId::Sha1(sha1))
    }

    /// Attempts to read the specified crate's index metadata
    ///
    /// An attempt is first made to read the cache entry for the crate, and
    /// falls back to reading the metadata from the git blob it is stored in
    ///
    /// This method does no network I/O
    pub fn krate(
        &self,
        name: KrateName<'_>,
        write_cache_entry: bool,
        lock: &FileLock,
    ) -> Result<Option<IndexKrate>, Error> {
        if let Ok(Some(cached)) = self.cached_krate(name, lock) {
            return Ok(Some(cached));
        }

        let Some(blob) = self.read_blob(&name.relative_path(None))? else {
            return Ok(None);
        };

        let krate = IndexKrate::from_slice(&blob.data)?;
        if write_cache_entry {
            // It's unfortunate if fail to write to the cache, but we still were
            // able to retrieve the contents from git
            let mut hex_id = [0u8; 40];
            let sha1 = utils::git::unwrap_sha1(blob.id);
            let blob_id = utils::encode_hex(&sha1, &mut hex_id);

            let _ = self.index.write_to_cache(&krate, Some(blob_id), lock);
        }

        Ok(Some(krate))
    }

    fn read_blob(&self, path: &str) -> Result<Option<gix::ObjectDetached>, GitError> {
        let tree = self
            .repo
            .find_object(self.head_commit)
            .map_err(Box::new)?
            .try_into_commit()?
            .tree()?;

        let Some(entry) = tree
            .lookup_entry_by_path(path)
            .map_err(|err| GitError::BlobLookup(Box::new(err)))?
        else {
            return Ok(None);
        };
        let blob = entry
            .object()
            .map_err(|err| GitError::BlobLookup(Box::new(err)))?;

        // Sanity check this is a blob, it _shouldn't_ be possible to get anything
        // else (like a subtree), but better safe than sorry
        if blob.kind != gix::object::Kind::Blob {
            return Ok(None);
        }

        Ok(Some(blob.detach()))
    }

    /// Attempts to read the locally cached crate information
    ///
    /// Note this method has improvements over using [`GitIndex::cached_krate`].
    ///
    /// In older versions of cargo, only the head commit hash is used as the version
    /// for cached crates, which means a fetch invalidates _all_ cached crates,
    /// even if they have not been modified in any commits since the previous
    /// fetch.
    ///
    /// This method does the same thing as cargo, which is to allow _either_
    /// the head commit oid _or_ the blob oid as a version, which is more
    /// granular and means the cached crate can remain valid as long as it is
    /// not updated in a subsequent fetch. [`GitIndex::cached_krate`] cannot take
    /// advantage of that though as it does not have access to git and thus
    /// cannot know the blob id.
    #[inline]
    pub fn cached_krate(
        &self,
        name: KrateName<'_>,
        lock: &FileLock,
    ) -> Result<Option<IndexKrate>, Error> {
        let Some(cached) = self.index.cache.read_cache_file(name, lock)? else {
            return Ok(None);
        };
        let valid = crate::index::cache::ValidCacheEntry::read(&cached)?;

        if Some(valid.revision) != self.index.head_commit() {
            let Some(blob) = self.read_blob(&name.relative_path(None))? else {
                return Ok(None);
            };

            let mut hex_id = [0u8; 40];
            let sha1 = utils::git::unwrap_sha1(blob.id);
            let blob_id = utils::encode_hex(&sha1, &mut hex_id);

            if valid.revision != blob_id {
                return Ok(None);
            }
        }

        valid.to_krate(None)
    }

    /// Performs a fetch from the remote index repository.
    ///
    /// This method performs network I/O.
    #[inline]
    pub fn fetch(&mut self, lock: &FileLock) -> Result<(), Error> {
        self.fetch_with_options(
            gix::progress::Discard,
            &gix::interrupt::IS_INTERRUPTED,
            lock,
        )
    }

    /// Same as [`Self::fetch`] but allows specifying a progress implementation
    /// and allows interruption of the network operations
    pub fn fetch_with_options<P>(
        &mut self,
        mut progress: P,
        should_interrupt: &AtomicBool,
        _lock: &FileLock,
    ) -> Result<(), Error>
    where
        P: gix::NestedProgress,
        P::SubProgress: 'static,
    {
        // We're updating the reflog which requires a committer be set, which might
        // not be the case, particular in a CI environment, but also would default
        // the the git config for the current directory/global, which on a normal
        // user machine would show the user was the one who updated the database which
        // is kind of misleading, so we just override the config for this operation

        let mut config = self.repo.config_snapshot_mut();
        config
            .set_raw_value(&"committer.name", "tame-index")
            .map_err(GitError::from)?;
        // Note we _have_ to set the email as well, but luckily gix does not actually
        // validate if it's a proper email or not :)
        config
            .set_raw_value(&"committer.email", "")
            .map_err(GitError::from)?;

        let repo = config
            .commit_auto_rollback()
            .map_err(|err| GitError::from(Box::new(err)))?;

        let mut remote = repo.find_remote("origin").ok().unwrap_or_else(|| {
            repo.remote_at(self.index.url.as_str())
                .expect("owned URL is always valid")
        });

        remote
            .replace_refspecs(Some("+HEAD:refs/remotes/origin/HEAD"), DIR)
            .expect("valid statically known refspec");

        // Perform the actual fetch
        let outcome = remote
            .connect(DIR)
            .map_err(|err| GitError::from(Box::new(err)))?
            .prepare_fetch(&mut progress, Default::default())
            .map_err(|err| GitError::from(Box::new(err)))?
            .receive(&mut progress, should_interrupt)
            .map_err(|err| GitError::from(Box::new(err)))?;

        utils::git::write_fetch_head(&repo, &outcome, &remote)?;
        self.head_commit = Self::set_head(&mut self.index, &repo)?;

        Ok(())
    }
}

/// Errors that can occur during a git operation
#[derive(Debug, thiserror::Error)]
#[allow(missing_docs)]
pub enum GitError {
    #[error(transparent)]
    ClonePrep(#[from] Box<gix::clone::Error>),
    #[error(transparent)]
    CloneFetch(#[from] Box<gix::clone::fetch::Error>),
    #[error(transparent)]
    Connect(#[from] Box<gix::remote::connect::Error>),
    #[error(transparent)]
    FetchPrep(#[from] Box<gix::remote::fetch::prepare::Error>),
    #[error(transparent)]
    Fetch(#[from] Box<gix::remote::fetch::Error>),
    #[error(transparent)]
    Open(#[from] Box<gix::open::Error>),
    #[error(transparent)]
    Commit(#[from] gix::object::commit::Error),
    #[error(transparent)]
    InvalidObject(#[from] gix::object::try_into::Error),
    #[error(transparent)]
    ReferenceLookup(#[from] Box<gix::reference::find::existing::Error>),
    #[error(transparent)]
    BlobLookup(#[from] Box<gix::object::find::existing::Error>),
    #[error(transparent)]
    RemoteLookup(#[from] Box<gix::remote::find::existing::Error>),
    #[error(transparent)]
    Lock(#[from] gix::lock::acquire::Error),
    #[error(transparent)]
    RemoteName(#[from] Box<gix::remote::name::Error>),
    #[error(transparent)]
    Config(#[from] Box<gix::config::Error>),
    #[error(transparent)]
    ConfigValue(#[from] gix::config::file::set_raw_value::Error),
    #[error("unable to locate remote HEAD")]
    UnableToFindRemoteHead,
    #[error("unable to update HEAD to remote HEAD")]
    UnableToUpdateHead,
}

impl GitError {
    /// Returns true if the error is a (potentially) spurious network error that
    /// indicates a retry of the operation could succeed
    #[inline]
    pub fn is_spurious(&self) -> bool {
        use gix::protocol::transport::IsSpuriousError;

        match self {
            Self::Fetch(fe) => return fe.is_spurious(),
            Self::CloneFetch(cf) => {
                if let gix::clone::fetch::Error::Fetch(fe) = &**cf {
                    return fe.is_spurious();
                }
            }
            _ => {}
        }

        false
    }

    /// Returns true if a fetch could not be completed successfully due to the
    /// repo being locked, and could succeed if retried
    #[inline]
    pub fn is_locked(&self) -> bool {
        let ure = match self {
            Self::Fetch(fe) => {
                if let gix::remote::fetch::Error::UpdateRefs(ure) = &**fe {
                    ure
                } else {
                    return false;
                }
            }
            Self::CloneFetch(cf) => {
                if let gix::clone::fetch::Error::Fetch(gix::remote::fetch::Error::UpdateRefs(ure)) =
                    &**cf
                {
                    ure
                } else {
                    return false;
                }
            }
            Self::Lock(le) => {
                return !matches!(le, gix::lock::acquire::Error::PermanentlyLocked { .. });
            }
            _ => return false,
        };

        if let gix::remote::fetch::refs::update::Error::EditReferences(ere) = ure {
            match ere {
                gix::reference::edit::Error::FileTransactionPrepare(ftpe) => {
                    use gix::refs::file::transaction::prepare::Error as PrepError;
                    if let PrepError::LockAcquire { source, .. }
                    | PrepError::PackedTransactionAcquire(source) = ftpe
                    {
                        // currently this is either io or permanentlylocked, but just in case
                        // more variants are added, we just assume it's possible to retry
                        // in anything but the permanentlylocked variant
                        !matches!(source, gix::lock::acquire::Error::PermanentlyLocked { .. })
                    } else {
                        false
                    }
                }
                gix::reference::edit::Error::FileTransactionCommit(ftce) => {
                    matches!(
                        ftce,
                        gix::refs::file::transaction::commit::Error::LockCommit { .. }
                    )
                }
                _ => false,
            }
        } else {
            false
        }
    }
}
