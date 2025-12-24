#![doc = include_str!("../README.md")]
#![deny(rust_2018_idioms)]
#![deny(missing_docs)]
#![deny(unnameable_types)]
#![cfg_attr(not(feature = "std"), no_std)]
#![cfg_attr(docsrs, feature(doc_cfg))]

#[cfg(all(
    feature = "alloc",
    any(feature = "xxhash3_64", feature = "xxhash3_128")
))]
extern crate alloc;

#[cfg(any(feature = "std", doc, test))]
extern crate std;

#[cfg(feature = "xxhash32")]
#[cfg_attr(docsrs, doc(cfg(feature = "xxhash32")))]
pub mod xxhash32;

#[cfg(feature = "xxhash32")]
#[cfg_attr(docsrs, doc(cfg(feature = "xxhash32")))]
pub use xxhash32::Hasher as XxHash32;

#[cfg(feature = "xxhash64")]
#[cfg_attr(docsrs, doc(cfg(feature = "xxhash64")))]
pub mod xxhash64;

#[cfg(feature = "xxhash64")]
#[cfg_attr(docsrs, doc(cfg(feature = "xxhash64")))]
pub use xxhash64::Hasher as XxHash64;

#[cfg(any(feature = "xxhash3_64", feature = "xxhash3_128"))]
mod xxhash3;

#[cfg(feature = "xxhash3_64")]
#[cfg_attr(docsrs, doc(cfg(feature = "xxhash3_64")))]
pub mod xxhash3_64;

#[cfg(feature = "xxhash3_64")]
#[cfg_attr(docsrs, doc(cfg(feature = "xxhash3_64")))]
pub use xxhash3_64::Hasher as XxHash3_64;

#[cfg(feature = "xxhash3_128")]
#[cfg_attr(docsrs, doc(cfg(feature = "xxhash3_128")))]
pub mod xxhash3_128;

#[cfg(feature = "xxhash3_128")]
#[cfg_attr(docsrs, doc(cfg(feature = "xxhash3_128")))]
pub use xxhash3_128::Hasher as XxHash3_128;

#[allow(dead_code, reason = "Too lazy to cfg-gate these")]
trait IntoU32 {
    fn into_u32(self) -> u32;
}

impl IntoU32 for u8 {
    fn into_u32(self) -> u32 {
        self.into()
    }
}

#[allow(dead_code, reason = "Too lazy to cfg-gate these")]
trait IntoU64 {
    fn into_u64(self) -> u64;
}

impl IntoU64 for u8 {
    fn into_u64(self) -> u64 {
        self.into()
    }
}

impl IntoU64 for u32 {
    fn into_u64(self) -> u64 {
        self.into()
    }
}

#[cfg(any(target_pointer_width = "32", target_pointer_width = "64"))]
impl IntoU64 for usize {
    fn into_u64(self) -> u64 {
        self as u64
    }
}

#[allow(dead_code, reason = "Too lazy to cfg-gate these")]
trait IntoU128 {
    fn into_u128(self) -> u128;
}

impl IntoU128 for u64 {
    fn into_u128(self) -> u128 {
        u128::from(self)
    }
}
