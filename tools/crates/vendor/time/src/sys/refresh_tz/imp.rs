//! A fallback for any OS not covered.

#[allow(clippy::missing_docs_in_private_items)]
pub(super) unsafe fn refresh_tz_unchecked() {}

#[allow(clippy::missing_docs_in_private_items)]
pub(super) fn refresh_tz() -> Option<()> {
    Some(())
}
