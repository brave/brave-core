//! Entries in the advisory database

use super::Iter;
use crate::{
    Map,
    advisory::{self, Advisory},
    collection::Collection,
    error::{Error, ErrorKind},
    map,
};
use std::{
    ffi::{OsStr, OsString},
    path::Path,
};

/// "Slots" identify the location in the entries table where a particular
/// advisory is located.
#[derive(Copy, Clone, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub(crate) struct Slot(usize);

/// Entries in the advisory database
#[derive(Debug, Default)]
pub(crate) struct Entries {
    /// Index of advisory IDs to their slots
    index: Map<advisory::Id, Slot>,

    /// Advisory collection
    advisories: Vec<Advisory>,
}

impl Entries {
    /// Create a new database entries collection
    pub(super) fn new() -> Self {
        Self::default()
    }

    /// Load an advisory from a file and insert it into the database entry table
    // TODO(tarcieri): factor more of this into `advisory.rs`?
    pub(super) fn load_file(&mut self, path: &Path) -> Result<Option<Slot>, Error> {
        let mut advisory = Advisory::load_file(path)?;

        // TODO(tarcieri): deprecate and remove legacy TOML-based advisory format
        let expected_filename = match path.extension().and_then(|ext| ext.to_str()) {
            Some("md") => OsString::from(format!("{}.md", advisory.metadata.id)),
            _ => fail!(
                ErrorKind::Repo,
                "unexpected file extension: {}",
                path.display()
            ),
        };

        // Ensure advisory has the correct filename
        if path.file_name().unwrap() != expected_filename && !Advisory::is_draft(path) {
            fail!(
                ErrorKind::Repo,
                "expected {} to be named {:?}; draft advisory filenames must start with \
                 RUSTSEC-0000-0000. and end in .md",
                path.display(),
                expected_filename
            );
        }

        // Ensure advisory is in a directory named after its package
        let package_dir = path.parent().ok_or_else(|| {
            Error::new(
                ErrorKind::Repo,
                format!("advisory has no parent dir: {}", path.display()),
            )
        })?;

        if package_dir.file_name().unwrap() != OsStr::new(advisory.metadata.package.as_str()) {
            fail!(
                ErrorKind::Repo,
                "expected {} to be in {} directory (instead of \"{:?}\")",
                advisory.metadata.id,
                advisory.metadata.package,
                package_dir
            );
        }

        // Get the collection this advisory is part of
        let collection_dir = package_dir
            .parent()
            .ok_or_else(|| {
                Error::new(
                    ErrorKind::Repo,
                    format!("advisory has no collection: {}", path.display()),
                )
            })?
            .file_name()
            .unwrap();

        let collection = if collection_dir == OsStr::new(Collection::Crates.as_str()) {
            Collection::Crates
        } else if collection_dir == OsStr::new(Collection::Rust.as_str()) {
            Collection::Rust
        } else {
            fail!(
                ErrorKind::Repo,
                "invalid package collection: {:?}",
                collection_dir
            );
        };

        match advisory.metadata.collection {
            Some(c) => {
                if c != collection {
                    fail!(
                        ErrorKind::Parse,
                        "collection mismatch for {}",
                        &advisory.metadata.id
                    );
                }
            }
            None => advisory.metadata.collection = Some(collection),
        }

        // Ensure placeholder advisories load and parse correctly, but
        // don't actually insert them into the advisory database
        if advisory.metadata.id.is_placeholder() {
            return Ok(None);
        }

        let id = advisory.metadata.id.clone();
        let slot = Slot(self.advisories.len());
        self.advisories.push(advisory);

        match self.index.entry(id) {
            map::Entry::Vacant(entry) => {
                entry.insert(slot);
            }
            map::Entry::Occupied(entry) => {
                fail!(ErrorKind::Parse, "duplicate advisory ID: {}", entry.key())
            }
        }

        Ok(Some(slot))
    }

    /// Find an advisory by its `advisory::Id`
    pub(super) fn find_by_id(&self, id: &advisory::Id) -> Option<&Advisory> {
        self.index.get(id).and_then(|slot| self.get(*slot))
    }

    /// Get an advisory from the database by its [`Slot`]
    pub(super) fn get(&self, slot: Slot) -> Option<&Advisory> {
        self.advisories.get(slot.0)
    }

    /// Iterate over all of the entries in the database
    pub(super) fn iter(&self) -> Iter<'_> {
        self.advisories.iter()
    }
}

impl IntoIterator for Entries {
    type Item = Advisory;

    type IntoIter = std::vec::IntoIter<Advisory>;

    fn into_iter(self) -> Self::IntoIter {
        self.advisories.into_iter()
    }
}

#[cfg(test)]
mod tests {
    use super::Entries;
    use std::fs;

    #[test]
    fn filename_error_explains_draft_format() {
        let dir = tempfile::tempdir().unwrap();
        let package_dir = dir.path().join("crates/base");
        fs::create_dir_all(&package_dir).unwrap();

        let advisory = include_str!("../../tests/support/example_advisory_v3.md")
            .replace("RUSTSEC-2001-2101", "RUSTSEC-0000-0000");
        let path = package_dir.join("RUSTSEC-0000-0000-decompression.md");
        fs::write(&path, &advisory).unwrap();

        let mut entries = Entries::new();
        let error = entries.load_file(&path).unwrap_err().to_string();
        assert_eq!(
            error,
            format!(
                "git operation failed: expected {} to be named \"RUSTSEC-0000-0000.md\"; \
                 draft advisory filenames must start with RUSTSEC-0000-0000. and end in .md",
                path.display()
            )
        );

        let second_path = package_dir.join("RUSTSEC-0000-0000.brotli.md");
        fs::write(&second_path, &advisory).unwrap();

        let renamed_path = package_dir.join("RUSTSEC-0000-0000.decompression.md");
        fs::rename(&path, &renamed_path).unwrap();

        entries.load_file(&renamed_path).unwrap();
        entries.load_file(&second_path).unwrap();
    }
}
