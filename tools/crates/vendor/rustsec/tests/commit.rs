#![cfg(feature = "git")]

use gix::objs::Write;
use rustsec::repository::git::Repository;

const HEADER: &str = "tree 4b825dc642cb6eb9a060e54bf8d69288fbee4904\nauthor Example <example@example.com> 1700000000 +0000\ncommitter Example <example@example.com> 1700000000 +0000\n";
const SIGNATURE_HEADER: &str =
    "gpgsig -----BEGIN PGP SIGNATURE-----\n \n Zml4dHVyZQ==\n -----END PGP SIGNATURE-----\n";
const SIGNATURE: &[u8] =
    b"-----BEGIN PGP SIGNATURE-----\n\nZml4dHVyZQ==\n-----END PGP SIGNATURE-----\n";

fn latest_commit(raw: &[u8]) -> rustsec::repository::git::Commit {
    let dir = tempfile::tempdir().unwrap();
    let repo = gix::init(dir.path()).unwrap();
    let id = repo
        .objects
        .write_buf(gix::objs::Kind::Commit, raw)
        .unwrap();
    std::fs::write(repo.git_dir().join("HEAD"), format!("{id}\n")).unwrap();
    Repository::open(dir.path())
        .unwrap()
        .latest_commit()
        .unwrap()
}

fn assert_signed(raw: &[u8], expected: &[u8]) {
    let commit = latest_commit(raw);
    assert_eq!(commit.signature.as_ref().unwrap().as_ref(), SIGNATURE);
    assert_eq!(commit.raw_signed_bytes(), Some(expected));
}

#[test]
fn unsigned_commit_has_no_signed_bytes() {
    let commit = latest_commit(format!("{HEADER}\nFixture\n").as_bytes());
    assert!(commit.signature.is_none());
    assert!(commit.raw_signed_bytes().is_none());
    assert_eq!(commit.author, "Example <example@example.com>");
    assert_eq!(commit.summary, "Fixture");
    assert_eq!(commit.timestamp.unix_timestamp(), 1700000000);
}

#[test]
fn signed_commit_preserves_message_and_signature() {
    let message = "\nFixture\n\nBody without a final newline";
    assert_signed(
        format!("{HEADER}{SIGNATURE_HEADER}{message}").as_bytes(),
        format!("{HEADER}{message}").as_bytes(),
    );
}

#[test]
fn signed_commit_preserves_headers_after_signature() {
    let rest = "encoding UTF-8\ncustom value\n continued\n\nFixture\n";
    assert_signed(
        format!("{HEADER}{SIGNATURE_HEADER}{rest}").as_bytes(),
        format!("{HEADER}{rest}").as_bytes(),
    );
}

#[test]
fn signed_commit_preserves_noncanonical_timestamps() {
    let header = HEADER.replace("1700000000 +0000", "01700000000 -0000");
    assert_signed(
        format!("{header}{SIGNATURE_HEADER}\nFixture\n").as_bytes(),
        format!("{header}\nFixture\n").as_bytes(),
    );
}
