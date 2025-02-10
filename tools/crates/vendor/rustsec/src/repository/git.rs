//! Git repository handling for the RustSec advisory DB

mod commit;
mod commit_hash;
#[cfg(feature = "osv-export")]
mod gitpath;
#[cfg(feature = "osv-export")]
mod modification_time;
mod repository;

pub use self::{commit::Commit, commit_hash::CommitHash, repository::Repository};
use tame_index::external::gix;

#[cfg(feature = "osv-export")]
pub use self::{gitpath::GitPath, modification_time::GitModificationTimes};

/// Location of the RustSec advisory database for crates.io
pub const DEFAULT_URL: &str = "https://github.com/RustSec/advisory-db.git";

#[inline]
pub(crate) fn gix_time_to_time(time: gix::date::Time) -> time::OffsetDateTime {
    time::OffsetDateTime::from_unix_timestamp(time.seconds)
        .expect("always valid unix time")
        .to_offset(time::UtcOffset::from_whole_seconds(time.offset).expect("valid offset"))
}
