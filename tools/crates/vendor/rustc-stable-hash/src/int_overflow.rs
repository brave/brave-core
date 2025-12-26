//! Weaker version of `-Coverflow-checks`.

// FIXME: These two traits seems overkill for this crate, we should probably
// replace these by two simple free floating function instead since we only
// operate on `usize`.

/// Addition, but only overflow checked when `cfg(debug_assertions)` is set
/// instead of respecting `-Coverflow-checks`.
///
/// This exists for performance reasons, as we ship rustc with overflow checks.
/// While overflow checks are perf neutral in almost all of the compiler, there
/// are a few particularly hot areas where we don't want overflow checks in our
/// dist builds. Overflow is still a bug there, so we want overflow check for
/// builds with debug assertions.
///
/// That's a long way to say that this should be used in areas where overflow
/// is a bug but overflow checking is too slow.
pub(crate) trait DebugStrictAdd {
    /// See [`DebugStrictAdd`].
    fn debug_strict_add(self, other: Self) -> Self;
}

macro_rules! impl_debug_strict_add {
    ($( $ty:ty )*) => {
        $(
            impl DebugStrictAdd for $ty {
                fn debug_strict_add(self, other: Self) -> Self {
                    if cfg!(debug_assertions) {
                        self + other
                    } else {
                        self.wrapping_add(other)
                    }
                }
            }
        )*
    };
}

/// See [`DebugStrictAdd`].
pub(crate) trait DebugStrictSub {
    /// See [`DebugStrictAdd`].
    fn debug_strict_sub(self, other: Self) -> Self;
}

macro_rules! impl_debug_strict_sub {
    ($( $ty:ty )*) => {
        $(
            impl DebugStrictSub for $ty {
                fn debug_strict_sub(self, other: Self) -> Self {
                    if cfg!(debug_assertions) {
                        self - other
                    } else {
                        self.wrapping_sub(other)
                    }
                }
            }
        )*
    };
}

impl_debug_strict_add! {
    usize
}

/*
impl_debug_strict_add! {
    u8 u16 u32 u64 u128 usize
    i8 i16 i32 i64 i128 isize
}
*/

impl_debug_strict_sub! {
    usize
}

/*
impl_debug_strict_sub! {
    u8 u16 u32 u64 u128 usize
    i8 i16 i32 i64 i128 isize
}
*/
