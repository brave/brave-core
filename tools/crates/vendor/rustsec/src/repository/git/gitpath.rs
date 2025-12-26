use super::Repository;
use crate::{Error, ErrorKind};
use std::path::Path;

/// A path *relative to the root of the git repository* that is guaranteed to be tracked by Git.
///
/// This type is immutable.
#[cfg_attr(docsrs, doc(cfg(feature = "osv-export")))]
#[derive(Clone, Copy)]
pub struct GitPath<'a> {
    repo: &'a Repository,
    path: &'a Path,
}

impl<'a> GitPath<'a> {
    /// Creates a new `GitPath`, validating that this file is tracked in Git
    pub fn new(repo: &'a Repository, path: &'a Path) -> Result<Self, Error> {
        // Validate that the path is relative for better feedback to API users
        if path.has_root() {
            fail!(
                ErrorKind::BadParam,
                "{} is not a relative path",
                path.display()
            );
        }

        if !repo.has_relative_path(path) {
            Err(Error::new(
                ErrorKind::Repo,
                format!("HEAD commit does not contain path '{}'", path.display()),
            ))
        } else {
            Ok(GitPath { repo, path })
        }
    }

    /// A path *relative to the root of the git repository* that is guaranteed to be tracked by Git
    pub fn path(&self) -> &'a Path {
        self.path
    }

    /// The git repository the path is tracked by
    pub fn repository(&self) -> &'a Repository {
        self.repo
    }
}
