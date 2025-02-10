//! XZ footer.

/// File format trailing terminator, see sect. 2.1.2.4.
pub(crate) const XZ_MAGIC_FOOTER: &[u8] = &[0x59, 0x5A];
