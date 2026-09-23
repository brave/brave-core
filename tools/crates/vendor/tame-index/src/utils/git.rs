//! Utilities for working with gix that might be useful for downstream users

use crate::{Error, error::GitError};

/// Writes the `FETCH_HEAD` for the specified fetch outcome to the specified git
/// repository
///
/// This function is narrowly focused on on writing a `FETCH_HEAD` that contains
/// exactly two pieces of information, the id of the commit pointed to by the
/// remote `HEAD`, and, if it exists, the same id with the remote branch whose
/// `HEAD` is the same. This focus gives use two things:
///     1. `FETCH_HEAD` that can be parsed to the correct remote HEAD by
/// [`gix`](https://github.com/Byron/gitoxide/commit/eb2b513bd939f6b59891d0a4cf5465b1c1e458b3)
///     1. A `FETCH_HEAD` that closely (or even exactly) matches that created by
/// cargo via git or git2 when fetching only `+HEAD:refs/remotes/origin/HEAD`
///
/// Calling this function for the fetch outcome of a clone will write `FETCH_HEAD`
/// just as if a normal fetch had occurred, but note that AFAICT neither git nor
/// git2 does this, ie. a fresh clone will not have a `FETCH_HEAD` present. I don't
/// _think_ that has negative implications, but if it does...just don't call this
/// function on the result of a clone :)
///
/// Note that the remote provided should be the same remote used for the fetch
/// operation. The reason this is not just grabbed from the repo is because
/// repositories may not have the configured remote, or the remote was modified
/// (eg. replacing refspecs) before the fetch operation
pub fn write_fetch_head(
    repo: &gix::Repository,
    fetch: &gix::remote::fetch::Outcome,
    remote: &gix::Remote<'_>,
) -> Result<gix::ObjectId, Error> {
    use gix::{bstr::ByteSlice, protocol::handshake::Ref};
    use std::fmt::Write;

    // Find the remote head commit
    let (head_target_branch, oid) = fetch
        .ref_map
        .mappings
        .iter()
        .find_map(|mapping| {
            let gix::remote::fetch::refmap::Source::Ref(rref) = &mapping.remote else {
                return None;
            };

            let Ref::Symbolic {
                full_ref_name,
                target,
                object,
                ..
            } = rref
            else {
                return None;
            };

            (full_ref_name == "HEAD").then_some((target, object))
        })
        .ok_or_else(|| GitError::UnableToFindRemoteHead)?;

    let remote_url = {
        let ru = remote
            .url(gix::remote::Direction::Fetch)
            .expect("can't fetch without a fetch url");
        let s = ru.to_bstring();
        let v = s.into();
        String::from_utf8(v).expect("remote url was not utf-8 :-/")
    };

    let fetch_head = {
        let mut hex_id = [0u8; 40];
        let sha1 = unwrap_sha1(*oid);
        let commit_id = crate::utils::encode_hex(&sha1, &mut hex_id);

        let mut fetch_head = String::new();

        let remote_name = remote
            .name()
            .and_then(|n| {
                let gix::remote::Name::Symbol(name) = n else {
                    return None;
                };
                Some(name.as_ref())
            })
            .unwrap_or("origin");

        // We write the remote HEAD first, but _only_ if it was explicitly requested
        if remote
            .refspecs(gix::remote::Direction::Fetch)
            .iter()
            .any(|rspec| {
                let rspec = rspec.to_ref();
                if !rspec.remote().is_some_and(|r| r.ends_with(b"HEAD")) {
                    return false;
                }

                rspec.local().is_some_and(|l| {
                    l.to_str().ok().and_then(|l| {
                        l.strip_prefix("refs/remotes/")
                            .and_then(|l| l.strip_suffix("/HEAD"))
                    }) == Some(remote_name)
                })
            })
        {
            writeln!(&mut fetch_head, "{commit_id}\t\t{remote_url}").unwrap();
        }

        // Attempt to get the branch name, but if it looks suspect just skip this,
        // it _should_ be fine, or at least, we've already written the only thing
        // that gix can currently parse
        if let Some(branch_name) = head_target_branch
            .to_str()
            .ok()
            .and_then(|s| s.strip_prefix("refs/heads/"))
        {
            writeln!(
                &mut fetch_head,
                "{commit_id}\t\tbranch '{branch_name}' of {remote_url}"
            )
            .unwrap();
        }

        fetch_head
    };

    // We _could_ also emit other branches/tags like git does, however it's more
    // complicated than just our limited use case of writing remote HEAD
    //
    // 1. Remote branches are always emitted, however in gix those aren't part
    // of the ref mappings if they haven't been updated since the last fetch
    // 2. Conversely, tags are _not_ written by git unless they have been changed
    // added, but gix _does_ always place those in the fetch mappings

    if fetch_head.is_empty() {
        return Err(GitError::UnableToFindRemoteHead.into());
    }

    let fetch_head_path = crate::PathBuf::from_path_buf(repo.path().join("FETCH_HEAD"))?;
    std::fs::write(&fetch_head_path, fetch_head)
        .map_err(|io| Error::IoPath(io, fetch_head_path))?;

    Ok(*oid)
}

/// Workaround for `#[non_exhaustive]`
#[inline]
pub fn unwrap_sha1(oid: gix::ObjectId) -> [u8; 20] {
    let gix::ObjectId::Sha1(sha1) = oid else {
        unreachable!()
    };
    sha1
}
