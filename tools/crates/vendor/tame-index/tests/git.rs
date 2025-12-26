#![allow(missing_docs)]
#![cfg(feature = "__git")]

mod utils;
use tame_index::{
    GitIndex, IndexKrate, IndexLocation, IndexPath, IndexUrl, Path, index::RemoteGitIndex,
};

fn remote_index(
    path: impl AsRef<tame_index::Path>,
    url: impl AsRef<tame_index::Path>,
) -> RemoteGitIndex {
    RemoteGitIndex::new(
        GitIndex::new(IndexLocation {
            url: IndexUrl::NonCratesIo(url.as_ref().as_str().into()),
            root: IndexPath::Exact(path.as_ref().join("sub/dir")),
            cargo_version: None,
        })
        .unwrap(),
        &utils::unlocked(),
    )
    .unwrap()
}

enum UpdateEntry {
    Blob(gix::ObjectId),
    Tree(UpdateTree),
}

type UpdateTree = std::collections::BTreeMap<String, UpdateEntry>;

struct TreeUpdateBuilder {
    update_tree: UpdateTree,
}

impl TreeUpdateBuilder {
    fn new() -> Self {
        Self {
            update_tree: UpdateTree::new(),
        }
    }

    fn upsert_blob(&mut self, path: &Path, oid: gix::ObjectId) {
        let ancestors = path.parent().unwrap();
        let file_name = path.file_name().unwrap();

        let mut ct = &mut self.update_tree;

        for comp in ancestors.components().filter_map(|com| {
            if let camino::Utf8Component::Normal(n) = com {
                Some(n)
            } else {
                None
            }
        }) {
            let entry = ct
                .entry(comp.to_owned())
                .or_insert_with(|| UpdateEntry::Tree(UpdateTree::new()));

            if let UpdateEntry::Tree(t) = entry {
                ct = t;
            } else {
                panic!("blob already inserted");
            }
        }

        if ct.contains_key(file_name) {
            panic!("tree already inserted with same filename as blob");
        }

        ct.insert(file_name.to_owned(), UpdateEntry::Blob(oid));
    }

    fn create_updated(self, repo: &gix::Repository) -> gix::ObjectId {
        let head_tree = repo.head_commit().unwrap().tree().unwrap();
        Self::create_inner(self.update_tree, &head_tree, repo)
    }

    fn create_inner(
        tree: UpdateTree,
        current: &gix::Tree<'_>,
        repo: &gix::Repository,
    ) -> gix::ObjectId {
        use gix::objs::{
            Tree,
            tree::{Entry, EntryKind},
        };

        let mut nt = Tree::empty();
        let tree_ref = current.decode().unwrap();

        // Since they are stored in a btreemap we don't have to worry about
        // sorting here to satisfy the constraints of Tree
        for (name, entry) in tree {
            let filename = name.as_str().into();
            match entry {
                UpdateEntry::Blob(oid) => {
                    nt.entries.push(Entry {
                        mode: EntryKind::Blob.into(),
                        oid,
                        filename,
                    });
                }
                UpdateEntry::Tree(ut) => {
                    // Check if there is already an existing tree
                    let current_tree = tree_ref.entries.iter().find_map(|tre| {
                        if tre.filename == name && tre.mode.is_tree() {
                            Some(repo.find_object(tre.oid).unwrap().into_tree())
                        } else {
                            None
                        }
                    });
                    let current_tree = current_tree.unwrap_or_else(|| repo.empty_tree());

                    let oid = Self::create_inner(ut, &current_tree, repo);
                    nt.entries.push(Entry {
                        mode: EntryKind::Tree.into(),
                        oid,
                        filename,
                    });
                }
            }
        }

        // Insert all the entries from the old tree that weren't added/modified
        // in this builder
        for entry in tree_ref.entries {
            if let Err(i) = nt
                .entries
                .binary_search_by_key(&entry.filename, |e| e.filename.as_ref())
            {
                nt.entries.insert(
                    i,
                    Entry {
                        mode: entry.mode,
                        oid: entry.oid.into(),
                        filename: entry.filename.to_owned(),
                    },
                );
            }
        }

        repo.write_object(nt).unwrap().detach()
    }
}

/// For testing purposes we create a local git repository as the remote for tests
/// so that we avoid
///
/// 1. Using the crates.io git registry. It's massive and slow.
/// 2. Using some other external git registry, could fail for any number of
///    network etc related issues
/// 3. Needing to maintain a blessed remote of any kind
struct FakeRemote {
    repo: gix::Repository,
    td: utils::TempDir,
    // The parent commit
    parent: gix::ObjectId,
    commits: u32,
}

impl FakeRemote {
    fn new() -> Self {
        let td = utils::tempdir();

        let mut repo = gix::init_bare(&td).expect("failed to create remote repo");

        // Create an empty initial commit so we always have _something_
        let parent = {
            let empty_tree_id = repo
                .write_object(gix::objs::Tree::empty())
                .unwrap()
                .detach();

            let repo = Self::snapshot(&mut repo);
            repo.commit(
                "HEAD",
                "initial commit",
                empty_tree_id,
                gix::commit::NO_PARENT_IDS,
            )
            .unwrap()
            .detach()
        };

        Self {
            td,
            repo,
            parent,
            commits: 0,
        }
    }

    #[inline]
    fn snapshot(repo: &mut gix::Repository) -> gix::config::CommitAutoRollback<'_> {
        let mut config = repo.config_snapshot_mut();
        config
            .set_raw_value(&"author.name", "Integration Test")
            .unwrap();
        config
            .set_raw_value(&"committer.name", "Integration Test")
            .unwrap();
        config
            .set_raw_value(&"author.email", "tests@integration.se")
            .unwrap();
        config
            .set_raw_value(&"committer.email", "tests@integration.se")
            .unwrap();

        // Disable GPG signing, it breaks testing if the user has it enabled
        config.set_raw_value(&"commit.gpgsign", "false").unwrap();

        config.commit_auto_rollback().unwrap()
    }

    fn commit(&mut self, krate: &IndexKrate) -> gix::ObjectId {
        let repo = Self::snapshot(&mut self.repo);

        let mut serialized = Vec::new();
        krate.write_json_lines(&mut serialized).unwrap();

        let name: tame_index::KrateName<'_> = krate.name().try_into().unwrap();
        let rel_path = tame_index::PathBuf::from(name.relative_path(None));

        let blob_id = repo.write_blob(serialized).unwrap().into();

        let mut tub = TreeUpdateBuilder::new();
        tub.upsert_blob(Path::new(&rel_path), blob_id);

        let tree_id = tub.create_updated(&repo);

        self.commits += 1;

        let parent = repo
            .commit(
                "HEAD",
                format!("{} - {}", krate.name(), self.commits),
                tree_id,
                [self.parent],
            )
            .unwrap()
            .detach();
        self.parent = parent;
        parent
    }

    fn local(&self) -> (RemoteGitIndex, utils::TempDir) {
        let td = utils::tempdir();

        let rgi = remote_index(&td, &self.td);

        (rgi, td)
    }
}

/// Validates we can clone a new index repo
#[test]
fn clones_new() {
    let remote = FakeRemote::new();
    let lock = &utils::unlocked();

    let (rgi, _td) = remote.local();

    assert!(
        rgi.cached_krate("clones_new".try_into().unwrap(), lock)
            .unwrap()
            .is_none()
    );
}

/// Validates we can open an existing index repo
#[test]
fn opens_existing() {
    let mut remote = FakeRemote::new();
    let lock = &utils::unlocked();

    let krate = utils::fake_krate("opens-existing", 4);
    let expected_head = remote.commit(&krate);

    let (first, td) = remote.local();

    assert_eq!(
        first.local().head_commit().unwrap(),
        expected_head.to_hex().to_string()
    );

    // This should not be in the cache
    assert_eq!(
        first
            .krate("opens-existing".try_into().unwrap(), true, lock)
            .expect("failed to read git blob")
            .expect("expected krate"),
        krate,
    );

    let second = remote_index(&td, &remote.td);

    assert_eq!(
        second.local().head_commit().unwrap(),
        expected_head.to_hex().to_string()
    );

    // This should be in the cache as it is file based not memory based
    assert_eq!(
        first
            .cached_krate("opens-existing".try_into().unwrap(), lock)
            .expect("failed to read cache file")
            .expect("expected cached krate"),
        krate,
    );
}

/// Validates that cache entries can be created and used
#[test]
fn updates_cache() {
    let mut remote = FakeRemote::new();
    let lock = &utils::unlocked();

    let krate = utils::fake_krate("updates-cache", 4);
    let expected_head = remote.commit(&krate);

    let (rgi, _td) = remote.local();

    assert_eq!(
        rgi.local().head_commit().unwrap(),
        expected_head.to_hex().to_string()
    );

    // This should not be in the cache
    assert_eq!(
        rgi.krate("updates-cache".try_into().unwrap(), true, lock)
            .expect("failed to read git blob")
            .expect("expected krate"),
        krate,
    );

    assert_eq!(
        rgi.cached_krate("updates-cache".try_into().unwrap(), lock)
            .expect("failed to read cache file")
            .expect("expected krate"),
        krate,
    );
}

/// Validates we can fetch updates from the remote and invalidate only the cache
/// entries for the crates that have changed
#[test]
fn fetch_invalidates_cache() {
    let mut remote = FakeRemote::new();
    let lock = &utils::unlocked();

    let krate = utils::fake_krate("invalidates-cache", 4);
    let same = utils::fake_krate("will-be-cached", 2);
    remote.commit(&krate);
    let expected_head = remote.commit(&same);

    let (mut rgi, _td) = remote.local();

    assert_eq!(
        rgi.local().head_commit().unwrap(),
        expected_head.to_hex().to_string()
    );

    // These should not be in the cache
    assert_eq!(
        rgi.krate("invalidates-cache".try_into().unwrap(), true, lock)
            .expect("failed to read git blob")
            .expect("expected krate"),
        krate,
    );
    assert_eq!(
        rgi.krate("will-be-cached".try_into().unwrap(), true, lock)
            .expect("failed to read git blob")
            .expect("expected krate"),
        same,
    );

    // Update the remote
    let new_krate = utils::fake_krate("invalidates-cache", 5);
    let new_head = remote.commit(&new_krate);

    assert_eq!(
        rgi.cached_krate("invalidates-cache".try_into().unwrap(), lock)
            .expect("failed to read cache file")
            .expect("expected krate"),
        krate,
    );
    assert_eq!(
        rgi.cached_krate("will-be-cached".try_into().unwrap(), lock)
            .expect("failed to read cache file")
            .expect("expected krate"),
        same,
    );

    // Perform fetch, which should invalidate the cache
    rgi.fetch(lock).unwrap();

    assert_eq!(
        rgi.local().head_commit().unwrap(),
        new_head.to_hex().to_string()
    );

    assert!(
        rgi.cached_krate("invalidates-cache".try_into().unwrap(), lock)
            .unwrap()
            .is_none()
    );

    assert_eq!(
        rgi.krate("invalidates-cache".try_into().unwrap(), true, lock)
            .expect("failed to read git blob")
            .expect("expected krate"),
        new_krate,
    );

    // This crate _should_ still be cached as it was not changed in the fetch
    assert_eq!(
        rgi.cached_krate("will-be-cached".try_into().unwrap(), lock)
            .expect("failed to read cache file")
            .expect("expected krate"),
        same,
    );

    // We haven't made new commits, so the fetch should not move HEAD and thus
    // cache entries should still be valid
    rgi.fetch(lock).unwrap();

    assert_eq!(
        rgi.cached_krate("invalidates-cache".try_into().unwrap(), lock)
            .unwrap()
            .unwrap(),
        new_krate
    );

    let krate3 = utils::fake_krate("krate-3", 3);
    remote.commit(&krate3);

    let krate4 = utils::fake_krate("krate-4", 4);
    let expected_head = remote.commit(&krate4);

    rgi.fetch(lock).unwrap();

    assert_eq!(
        rgi.local().head_commit().unwrap(),
        expected_head.to_hex().to_string()
    );

    assert_eq!(
        rgi.cached_krate("invalidates-cache".try_into().unwrap(), lock)
            .expect("failed to read cache file")
            .expect("expected krate"),
        new_krate,
    );
    assert_eq!(
        rgi.cached_krate("will-be-cached".try_into().unwrap(), lock)
            .expect("failed to read cache file")
            .expect("expected krate"),
        same,
    );
}

/// gix uses a default branch name of `main`, but most cargo git indexes on users
/// disks use the master branch, so just ensure that we support that as well
#[test]
fn non_main_local_branch() {
    let mut remote = FakeRemote::new();

    let local_td = utils::tempdir();

    // Set up the local repo as if it was an already existing index
    // created by cargo
    {
        // Do that actual init
        let mut cmd = std::process::Command::new("git");
        cmd.args(["init", "--bare", "-b", "master"]);
        cmd.arg(local_td.path());
        assert!(
            cmd.status().expect("failed to run git").success(),
            "git failed to init directory"
        );

        // Add a fake commit so that we have a local HEAD
        let mut repo = gix::open(local_td.path()).unwrap();

        let commit = {
            let snap = FakeRemote::snapshot(&mut repo);
            let empty_tree_id = snap
                .write_object(gix::objs::Tree::empty())
                .unwrap()
                .detach();

            snap.commit(
                "refs/heads/master",
                "initial commit",
                empty_tree_id,
                gix::commit::NO_PARENT_IDS,
            )
            .unwrap()
            .detach()
        };

        use gix::refs::transaction as tx;
        repo.edit_references([
            tx::RefEdit {
                change: tx::Change::Update {
                    log: tx::LogChange {
                        mode: tx::RefLog::AndReference,
                        force_create_reflog: false,
                        message: "".into(),
                    },
                    expected: tx::PreviousValue::Any,
                    new: gix::refs::Target::Object(commit),
                },
                name: "refs/heads/master".try_into().unwrap(),
                deref: false,
            },
            tx::RefEdit {
                change: tx::Change::Update {
                    log: tx::LogChange {
                        mode: tx::RefLog::AndReference,
                        force_create_reflog: false,
                        message: "".into(),
                    },
                    expected: tx::PreviousValue::Any,
                    new: gix::refs::Target::Symbolic("refs/heads/master".try_into().unwrap()),
                },
                name: "HEAD".try_into().unwrap(),
                deref: false,
            },
        ])
        .unwrap();

        assert_eq!(commit, repo.head_commit().unwrap().id);
    }

    let mut rgi = remote_index(&local_td, &remote.td);
    let lock = &utils::unlocked();

    let first = utils::fake_krate("first", 1);
    remote.commit(&first);

    rgi.fetch(lock).unwrap();

    assert_eq!(
        rgi.krate("first".try_into().unwrap(), true, lock)
            .unwrap()
            .unwrap(),
        first
    );
}
