//! An implementation of the shared parts of git bitmaps used in `gix-pack`, `gix-index` and `gix-worktree`.
//!
//! Note that many tests are performed indirectly by tests in the aforementioned consumer crates.
#![deny(rust_2018_idioms, unsafe_code, missing_docs)]

/// Bitmap utilities for the advanced word-aligned hybrid bitmap
pub mod ewah;

pub(crate) mod decode {
    #[inline]
    pub(crate) fn u32(data: &[u8]) -> Option<(u32, &[u8])> {
        data.split_at_checked(4)
            .map(|(num, data)| (u32::from_be_bytes(num.try_into().unwrap()), data))
    }
}
