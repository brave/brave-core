//! Poly1305 backends

#[cfg(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    not(poly1305_force_soft)
))]
pub(crate) mod avx2;

#[cfg(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    not(poly1305_force_soft)
))]
pub(crate) mod autodetect;

pub(crate) mod soft;
