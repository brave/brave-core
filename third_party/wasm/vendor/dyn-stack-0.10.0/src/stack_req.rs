use core::num::NonZeroUsize;

/// Stack allocation requirements.
#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub struct StackReq {
    align: NonZeroUsize,
    size: usize,
}

impl Default for StackReq {
    #[inline]
    fn default() -> Self {
        Self::empty()
    }
}

const fn unwrap(o: Option<usize>) -> usize {
    match o {
        Some(x) => x,
        None => panic!(),
    }
}

const fn round_up_pow2(a: usize, b: usize) -> usize {
    unwrap(a.checked_add(!b.wrapping_neg())) & b.wrapping_neg()
}

const fn try_round_up_pow2(a: usize, b: usize) -> Option<usize> {
    match a.checked_add(!b.wrapping_neg()) {
        None => None,
        Some(x) => Some(x & b.wrapping_neg()),
    }
}

const fn max(a: usize, b: usize) -> usize {
    if a > b {
        a
    } else {
        b
    }
}

/// Size overflow error during the computation of stack allocation requirements.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct SizeOverflow;

impl core::fmt::Display for SizeOverflow {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> Result<(), core::fmt::Error> {
        f.write_str("size computation overflowed")
    }
}

#[cfg(feature = "std")]
#[cfg_attr(docsrs, doc(cfg(feature = "std")))]
impl std::error::Error for SizeOverflow {}

impl StackReq {
    /// Allocation requirements for an empty unaligned buffer.
    #[inline]
    pub const fn empty() -> StackReq {
        Self {
            align: unsafe { NonZeroUsize::new_unchecked(1) },
            size: 0,
        }
    }

    /// Allocation requirements sufficient for `n` elements of type `T`, overaligned with alignment
    /// `align`.
    ///
    /// # Panics
    ///
    /// * if `align` is smaller than the minimum required alignment for an object of type `T`.
    /// * if `align` is not a power of two.
    /// * if the size computation overflows
    #[inline]
    pub const fn new_aligned<T>(n: usize, align: usize) -> StackReq {
        assert!(align >= core::mem::align_of::<T>());
        assert!(align.is_power_of_two());
        StackReq {
            // SAFETY: because align >= alignof::<T>() > 0
            align: unsafe { NonZeroUsize::new_unchecked(align) },
            size: unwrap(core::mem::size_of::<T>().checked_mul(n)),
        }
    }

    /// Allocation requirements sufficient for `n` elements of type `T`.
    ///
    /// # Panics
    ///
    /// * if the size computation overflows
    #[inline]
    pub const fn new<T>(n: usize) -> StackReq {
        StackReq::new_aligned::<T>(n, core::mem::align_of::<T>())
    }

    /// Same as [`StackReq::new_aligned`], but returns an error in case the size computation
    /// overflows.
    ///
    /// # Panics
    ///
    /// * if `align` is smaller than the minimum required alignment for an object of type `T`.
    /// * if `align` is not a power of two.
    #[inline]
    pub const fn try_new_aligned<T>(n: usize, align: usize) -> Result<StackReq, SizeOverflow> {
        assert!(align >= core::mem::align_of::<T>());
        assert!(align.is_power_of_two());
        match core::mem::size_of::<T>().checked_mul(n) {
            Some(x) => Ok(StackReq {
                // SAFETY: same as above
                align: unsafe { NonZeroUsize::new_unchecked(align) },
                size: x,
            }),
            None => Err(SizeOverflow),
        }
    }

    /// Same as [`StackReq::new`], but returns an error in case the size computation
    /// overflows.
    #[inline]
    pub const fn try_new<T>(n: usize) -> Result<StackReq, SizeOverflow> {
        StackReq::try_new_aligned::<T>(n, core::mem::align_of::<T>())
    }

    /// The number of allocated bytes required, aligned to `self.align_bytes()`.
    #[inline]
    pub const fn size_bytes(&self) -> usize {
        self.size
    }

    /// The alignment of allocated bytes required.
    #[inline]
    pub const fn align_bytes(&self) -> usize {
        self.align.get()
    }

    /// The number of allocated bytes required, with no alignment constraints.
    ///
    /// # Panics
    ///
    /// * if the size computation overflows
    #[inline]
    pub const fn unaligned_bytes_required(&self) -> usize {
        unwrap(self.size.checked_add(self.align.get() - 1))
    }

    /// Same as [`StackReq::unaligned_bytes_required`], but returns an error if the size computation
    /// overflows.
    #[inline]
    pub const fn try_unaligned_bytes_required(&self) -> Result<usize, SizeOverflow> {
        match self.size.checked_add(self.align.get() - 1) {
            Some(x) => Ok(x),
            None => Err(SizeOverflow),
        }
    }

    /// The required allocation to allocate storage sufficient for both of `self` and `other`,
    /// simultaneously and in any order.
    ///
    /// # Panics
    ///
    /// * if the allocation requirement computation overflows.
    #[inline]
    pub const fn and(self, other: StackReq) -> StackReq {
        let align = max(self.align.get(), other.align.get());
        StackReq {
            // SAFETY: align is either self.align or other.align, both of which are non zero
            align: unsafe { NonZeroUsize::new_unchecked(align) },
            size: unwrap(
                round_up_pow2(self.size, align).checked_add(round_up_pow2(other.size, align)),
            ),
        }
    }

    /// The required allocation to allocate storage sufficient for all the requirements produced by
    /// the given iterator, simultaneously and in any order.
    ///
    /// # Panics
    ///
    /// * if the allocation requirement computation overflows.
    #[inline]
    pub fn all_of(reqs: impl IntoIterator<Item = StackReq>) -> StackReq {
        fn all_of_impl(mut reqs: impl Iterator<Item = StackReq>) -> StackReq {
            let mut total = StackReq::empty();
            while let Some(req) = reqs.next() {
                total = total.and(req);
            }
            total
        }
        all_of_impl(reqs.into_iter())
    }

    /// The required allocation to allocate storage sufficient for either of `self` and `other`,
    /// with only one being active at a time.
    ///
    /// # Panics
    ///
    /// * if the allocation requirement computation overflows.
    #[inline]
    pub const fn or(self, other: StackReq) -> StackReq {
        let align = max(self.align.get(), other.align.get());
        StackReq {
            // SAFETY: same as above
            align: unsafe { NonZeroUsize::new_unchecked(align) },
            size: max(
                round_up_pow2(self.size, align),
                round_up_pow2(other.size, align),
            ),
        }
    }

    /// The required allocation to allocate storage sufficient for any of the requirements produced
    /// by the given iterator, with at most one being active at a time.
    ///
    /// # Panics
    ///
    /// * if the allocation requirement computation overflows.
    #[inline]
    pub fn any_of(reqs: impl IntoIterator<Item = StackReq>) -> StackReq {
        fn any_of_impl(mut reqs: impl Iterator<Item = StackReq>) -> StackReq {
            let mut total = StackReq::empty();
            while let Some(req) = reqs.next() {
                total = total.or(req);
            }
            total
        }
        any_of_impl(reqs.into_iter())
    }

    /// Same as [`StackReq::and`], but returns an error if the size computation overflows.
    #[inline]
    pub const fn try_and(self, other: StackReq) -> Result<StackReq, SizeOverflow> {
        let align = max(self.align.get(), other.align.get());
        Ok(StackReq {
            // SAFETY: same as above
            align: unsafe { NonZeroUsize::new_unchecked(align) },
            size: match match try_round_up_pow2(self.size, align) {
                Some(x) => x,
                None => return Err(SizeOverflow),
            }
            .checked_add(match try_round_up_pow2(other.size, align) {
                Some(x) => x,
                None => return Err(SizeOverflow),
            }) {
                Some(x) => x,
                None => return Err(SizeOverflow),
            },
        })
    }

    /// Same as [`StackReq::all_of`], but returns an error if the size computation overflows.
    #[inline]
    pub fn try_all_of(reqs: impl IntoIterator<Item = StackReq>) -> Result<StackReq, SizeOverflow> {
        fn try_all_of_impl(
            mut reqs: impl Iterator<Item = StackReq>,
        ) -> Result<StackReq, SizeOverflow> {
            let mut total = StackReq::empty();
            while let Some(req) = reqs.next() {
                total = total.try_and(req)?;
            }
            Ok(total)
        }
        try_all_of_impl(reqs.into_iter())
    }

    /// Same as [`StackReq::or`], but returns an error if the size computation overflows.
    #[inline]
    pub const fn try_or(self, other: StackReq) -> Result<StackReq, SizeOverflow> {
        let align = max(self.align.get(), other.align.get());
        Ok(StackReq {
            // SAFETY: same as above
            align: unsafe { NonZeroUsize::new_unchecked(align) },
            size: max(
                match try_round_up_pow2(self.size, align) {
                    Some(x) => x,
                    None => return Err(SizeOverflow),
                },
                match try_round_up_pow2(other.size, align) {
                    Some(x) => x,
                    None => return Err(SizeOverflow),
                },
            ),
        })
    }

    /// Same as [`StackReq::any_of`], but returns an error if the size computation overflows.
    #[inline]
    pub fn try_any_of(reqs: impl IntoIterator<Item = StackReq>) -> Result<StackReq, SizeOverflow> {
        fn try_any_of_impl(
            mut reqs: impl Iterator<Item = StackReq>,
        ) -> Result<StackReq, SizeOverflow> {
            let mut total = StackReq::empty();
            while let Some(req) = reqs.next() {
                total = total.try_or(req)?;
            }
            Ok(total)
        }
        try_any_of_impl(reqs.into_iter())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn size_of() {
        let size = core::mem::size_of::<usize>() * 2;
        assert_eq!(core::mem::size_of::<StackReq>(), size);
        assert_eq!(core::mem::size_of::<Option<StackReq>>(), size);
    }

    #[test]
    fn round_up() {
        assert_eq!(round_up_pow2(0, 4), 0);
        assert_eq!(round_up_pow2(1, 4), 4);
        assert_eq!(round_up_pow2(2, 4), 4);
        assert_eq!(round_up_pow2(3, 4), 4);
        assert_eq!(round_up_pow2(4, 4), 4);
    }

    #[test]
    #[should_panic]
    fn overflow() {
        let _ = StackReq::new::<i32>(usize::MAX);
    }

    #[test]
    #[should_panic]
    fn and_overflow() {
        let _ = StackReq::new::<u8>(usize::MAX).and(StackReq::new::<u8>(1));
    }
}
