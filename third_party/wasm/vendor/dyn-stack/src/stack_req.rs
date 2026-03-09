use core::alloc::{Layout, LayoutError};
use core::num::NonZeroUsize;

/// Stack allocation requirements.
#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub struct StackReq {
	align: Option<NonZeroUsize>,
	size: usize,
}

impl Default for StackReq {
	#[inline]
	fn default() -> Self {
		Self::empty()
	}
}

#[inline(always)]
const fn try_round_up_pow2(a: usize, b: usize) -> Option<usize> {
	match a.checked_add(!b.wrapping_neg()) {
		None => None,
		Some(x) => Some(x & b.wrapping_neg()),
	}
}

#[inline(always)]
const fn max(a: usize, b: usize) -> usize {
	if a > b { a } else { b }
}

impl StackReq {
	/// Allocation requirements for an empty unaligned buffer.
	pub const EMPTY: Self = Self {
		align: unsafe { Some(NonZeroUsize::new_unchecked(1)) },
		size: 0,
	};
	/// Unsatisfiable allocation requirements.
	pub const OVERFLOW: Self = Self { align: None, size: 0 };

	/// Allocation requirements for an empty unaligned buffer.
	#[inline]
	pub const fn empty() -> StackReq {
		Self::EMPTY
	}

	/// Allocation requirements sufficient for `n` elements of type `T`, overaligned with alignment
	/// `align`.
	///
	/// # Errors
	///
	/// * if `align` is smaller than the minimum required alignment for an object of type `T`.
	/// * if `align` is not a power of two.
	/// * if the size computation overflows
	#[inline]
	pub const fn new_aligned<T>(n: usize, align: usize) -> StackReq {
		if align >= core::mem::align_of::<T>() && align.is_power_of_two() {
			StackReq {
				align: unsafe { Some(NonZeroUsize::new_unchecked(align)) },
				size: core::mem::size_of::<T>(),
			}
			.array(n)
		} else {
			StackReq { align: None, size: 0 }
		}
	}

	/// Allocation requirements sufficient for `n` elements of type `T`.
	///
	/// # Errors
	///
	/// * if the size computation overflows
	#[inline]
	pub const fn new<T>(n: usize) -> StackReq {
		StackReq::new_aligned::<T>(n, core::mem::align_of::<T>())
	}

	/// The number of allocated bytes required, aligned to `self.align_bytes()`.
	#[inline]
	pub const fn size_bytes(&self) -> usize {
		self.size
	}

	/// The alignment of allocated bytes required, or `0` if the size overflowed.
	#[inline]
	pub const fn align_bytes(&self) -> usize {
		match self.align {
			Some(align) => align.get(),
			None => 0,
		}
	}

	/// The number of allocated bytes required, with no alignment constraints, or `usize::MAX` in
	/// the case of overflow.
	///
	/// # Panics
	///
	/// * if the size computation overflowed
	#[inline]
	pub const fn unaligned_bytes_required(&self) -> usize {
		match self.layout() {
			Ok(layout) => layout.size() + (layout.align() - 1),
			Err(_) => usize::MAX,
		}
	}

	/// Returns the corresponding layout for the allocation size and alignment.
	#[inline]
	pub const fn layout(self) -> Result<Layout, LayoutError> {
		Layout::from_size_align(self.size_bytes(), self.align_bytes())
	}

	/// The required allocation to allocate storage sufficient for both of `self` and `other`,
	/// simultaneously and in any order.
	///
	/// # Panics
	///
	/// * if the allocation requirement computation overflows.
	#[inline]
	pub const fn and(self, other: StackReq) -> StackReq {
		match (self.align, other.align) {
			(Some(left), Some(right)) => {
				let align = max(left.get(), right.get());
				let left = try_round_up_pow2(self.size, align);
				let right = try_round_up_pow2(other.size, align);

				match (left, right) {
					(Some(left), Some(right)) => {
						match left.checked_add(right) {
							Some(size) => StackReq {
								// SAFETY: align is either self.align or other.align, both of which are non zero
								align: unsafe { Some(NonZeroUsize::new_unchecked(align)) },
								size,
							},
							_ => StackReq::OVERFLOW,
						}
					},
					_ => StackReq::OVERFLOW,
				}
			},
			_ => StackReq::OVERFLOW,
		}
	}

	/// The required allocation to allocate storage sufficient for all the requirements produced by
	/// the given iterator, simultaneously and in any order.
	///
	/// # Panics
	///
	/// * if the allocation requirement computation overflows.
	#[inline]
	pub const fn all_of(reqs: &[Self]) -> Self {
		let mut total = StackReq::EMPTY;
		let mut reqs = reqs;
		while let Some((req, next)) = reqs.split_first() {
			total = total.and(*req);
			reqs = next;
		}
		total
	}

	/// The required allocation to allocate storage sufficient for either of `self` and `other`,
	/// with only one being active at a time.
	///
	/// # Panics
	///
	/// * if the allocation requirement computation overflows.
	#[inline]
	pub const fn or(self, other: StackReq) -> StackReq {
		match (self.align, other.align) {
			(Some(left), Some(right)) => {
				let align = max(left.get(), right.get());
				let left = try_round_up_pow2(self.size, align);
				let right = try_round_up_pow2(other.size, align);

				match (left, right) {
					(Some(left), Some(right)) => {
						let size = max(left, right);
						StackReq {
							// SAFETY: align is either self.align or other.align, both of which are non zero
							align: unsafe { Some(NonZeroUsize::new_unchecked(align)) },
							size,
						}
					},
					_ => StackReq::OVERFLOW,
				}
			},
			_ => StackReq::OVERFLOW,
		}
	}

	/// The required allocation to allocate storage sufficient for any of the requirements produced
	/// by the given iterator, with at most one being active at a time.
	///
	/// # Panics
	///
	/// * if the allocation requirement computation overflows.
	#[inline]
	pub fn any_of(reqs: &[StackReq]) -> StackReq {
		let mut total = StackReq::EMPTY;
		let mut reqs = reqs;
		while let Some((req, next)) = reqs.split_first() {
			total = total.or(*req);
			reqs = next;
		}
		total
	}

	/// Same as [`StackReq::and`] repeated `n` times.
	#[inline]
	pub const fn array(self, n: usize) -> StackReq {
		match self.align {
			Some(align) => {
				let size = self.size.checked_mul(n);
				match size {
					Some(size) => StackReq { size, align: Some(align) },
					None => StackReq::OVERFLOW,
				}
			},
			None => StackReq::OVERFLOW,
		}
	}
}

#[cfg(test)]
mod tests {
	use super::*;

	#[test]
	fn round_up() {
		assert_eq!(try_round_up_pow2(0, 4), Some(0));
		assert_eq!(try_round_up_pow2(1, 4), Some(4));
		assert_eq!(try_round_up_pow2(2, 4), Some(4));
		assert_eq!(try_round_up_pow2(3, 4), Some(4));
		assert_eq!(try_round_up_pow2(4, 4), Some(4));
	}

	#[test]
	fn overflow() {
		assert_eq!(StackReq::new::<u32>(usize::MAX).align_bytes(), 0);
	}

	#[test]
	fn and_overflow() {
		assert_eq!(StackReq::new::<u8>(usize::MAX).and(StackReq::new::<u8>(1)).align_bytes(), 0,);
	}
}
