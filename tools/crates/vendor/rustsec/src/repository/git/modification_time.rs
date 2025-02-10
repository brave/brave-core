use crate::advisory::Date;
use crate::error::{Error, ErrorKind};
use gix::date::Time;
use std::{
    cmp::{max, min},
    collections::HashMap,
    path::PathBuf,
};
use tame_index::external::gix;

use super::GitPath;

/// Tracks the time of latest modification of files in git.
#[cfg_attr(docsrs, doc(cfg(feature = "osv-export")))]
pub struct GitModificationTimes {
    mtimes: HashMap<PathBuf, Time>,
    ctimes: HashMap<PathBuf, Time>,
}

impl GitModificationTimes {
    /// Performance: collects all modification times on creation
    /// and caches them. This is more efficient for looking up lots of files,
    /// but wasteful if you just need to look up a couple files.
    pub fn new(repo: &super::Repository) -> Result<Self, Error> {
        use gix::{bstr::ByteVec, diff::tree::recorder::Change, prelude::Find};

        // Sadly I had to hand-roll this; there is no good off-the-shelf impl.
        // libgit2 has had a feature request for this for over a decade:
        // https://github.com/libgit2/libgit2/issues/495
        // as does git2-rs: https://github.com/rust-lang/git2-rs/issues/588
        // To make sure this works I've verified it against a naive shell script using `git log`
        // as well as `git whatchanged`
        let mut mtimes: HashMap<PathBuf, Time> = HashMap::new();
        let mut ctimes: HashMap<PathBuf, Time> = HashMap::new();

        let repo = &repo.repo;

        let walk = repo
            .rev_walk(Some(repo.head_id().map_err(|err| {
                format_err!(ErrorKind::Repo, "unable to find head id: {}", err)
            })?))
            .sorting(gix::traverse::commit::simple::Sorting::ByCommitTimeNewestFirst)
            .all()
            .map_err(|err| format_err!(ErrorKind::Repo, "unable to walk commits: {}", err))?;

        let db = &repo.objects;

        let mut buf = Vec::new();
        let mut buf2 = Vec::new();
        for info in walk {
            let info = info.map_err(|err| {
                format_err!(ErrorKind::Repo, "failed to retrieve commit info: {}", err)
            })?;

            let parent_commit_id = match info.parent_ids.len() {
                1 => Some(info.parent_ids[0]), // Diff with the previous commit
                0 => None,     // We've found the initial commit, diff with empty repo
                _ => continue, // Ignore merge commits (2+ parents) because that's what 'git whatchanged' does.
            };

            buf.clear();
            buf2.clear();
            let (main_tree_id, file_mod_time) = {
                let commit = db
                    .try_find(&info.id, &mut buf)
                    .map_err(|err| {
                        format_err!(
                            ErrorKind::Repo,
                            "failed to find commit '{}': {}",
                            info.id,
                            err
                        )
                    })?
                    .ok_or_else(|| {
                        format_err!(ErrorKind::Repo, "commit '{}' not present", info.id)
                    })?
                    .decode()
                    .map_err(|err| {
                        format_err!(
                            ErrorKind::Repo,
                            "unable to decode commit '{}': {}",
                            info.id,
                            err
                        )
                    })?
                    .into_commit()
                    .expect("id is actually a commit");

                (commit.tree(), commit.time())
            };
            let current_tree = db
                .try_find(&main_tree_id, &mut buf)
                .map_err(|err| {
                    format_err!(
                        ErrorKind::Repo,
                        "failed to find tree for commit '{}': {}",
                        info.id,
                        err
                    )
                })?
                .expect("main tree present")
                .try_into_tree_iter()
                .expect("id to be a tree");
            let previous_tree: Option<_> = {
                parent_commit_id
                    .and_then(|id| db.try_find(&id, &mut buf2).ok().flatten())
                    .and_then(|c| c.decode().ok())
                    .and_then(gix::objs::ObjectRef::into_commit)
                    .map(|c| c.tree())
                    .and_then(|tree| db.try_find(&tree, &mut buf2).ok().flatten())
                    .and_then(|tree| tree.try_into_tree_iter())
            };

            let mut recorder = gix::diff::tree::Recorder::default();
            gix::diff::tree::Changes::from(previous_tree)
                .needed_to_obtain(
                    current_tree,
                    &mut gix::diff::tree::State::default(),
                    db,
                    &mut recorder,
                )
                .map_err(|err| {
                    format_err!(
                        ErrorKind::Repo,
                        "failed to diff commit {} to its parent {:?}: {}",
                        info.id,
                        parent_commit_id,
                        err
                    )
                })?;

            for diff in recorder.records {
                // AFAIK files should never be deleted from an advisory db,
                // though unsure how moves/renames are handled by the recorder
                let file_path = match diff {
                    Change::Addition { path, .. }
                    | Change::Modification { path, .. }
                    | Change::Deletion { path, .. } => {
                        Vec::from(path).into_path_buf().expect("non utf-8 path")
                    }
                };

                mtimes
                    .entry(file_path.clone())
                    .and_modify(|t| *t = max(*t, file_mod_time))
                    .or_insert(file_mod_time);
                ctimes
                    .entry(file_path)
                    .and_modify(|t| *t = min(*t, file_mod_time))
                    .or_insert(file_mod_time);
            }
        }

        Ok(Self { mtimes, ctimes })
    }

    /// Looks up the Git modification time for a given file path.
    /// The path must be relative to the root of the repository.
    pub fn for_path(&self, path: GitPath<'_>) -> Time {
        *self.mtimes.get(path.path()).unwrap()
    }

    /// Looks up the Git creation time for a given file path.
    /// The path must be relative to the root of the repository.
    pub fn mdate_for_path(&self, path: GitPath<'_>) -> Date {
        Self::gix_time_to_date(self.mtimes.get(path.path()).unwrap())
    }

    /// Looks up the Git creation time for a given file path.
    /// The path must be relative to the root of the repository.
    pub fn cdate_for_path(&self, path: GitPath<'_>) -> Date {
        Self::gix_time_to_date(self.ctimes.get(path.path()).unwrap())
    }

    fn gix_time_to_date(timestamp: &Time) -> Date {
        let odt = crate::repository::git::gix_time_to_time(*timestamp);
        let date = odt.date();

        format!(
            "{:0>4}-{:0>2}-{:0>2}",
            date.year(),
            u8::from(date.month()),
            date.day()
        )
        .parse()
        .unwrap()
    }
}
