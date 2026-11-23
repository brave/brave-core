//! Bulk operations over slices and pointers.

use crate::{optimization_barrier, volatile_write};
use core::slice;

/// Whether this target supports an assembly optimization barrier.
const HAS_ASM: bool = cfg!(any(
    target_arch = "aarch64",
    target_arch = "arm",
    target_arch = "arm64ec",
    target_arch = "loongarch64",
    target_arch = "riscv32",
    target_arch = "riscv64",
    target_arch = "s390x",
    target_arch = "x86",
    target_arch = "x86_64",
));

/// Fill a slice using an assembly optimization barrier or portable volatile writes.
#[inline]
pub(crate) fn fill_slice<T: Copy>(dst: &mut [T], src: T) {
    if size_of::<T>() == 0 {
        return;
    }

    if HAS_ASM {
        dst.fill(src);
        optimization_barrier(dst);
    } else {
        volatile_fill_slice(dst, src);
    }
}

/// Zero bytes which may be uninitialized, including padding.
///
/// # Safety
/// `dst` must be non-NULL and provide exclusive access to `len` writable bytes within a single
/// allocation.
///
/// The size must not exceed `isize::MAX` and the address range must not wrap. If zeroes are not a
/// valid bit pattern for the underlying type, the caller must restore a valid value before any
/// typed access or drop.
#[inline]
pub(crate) unsafe fn zeroize_byte_ptr(dst: *mut u8, len: usize) {
    if HAS_ASM {
        // SAFETY: the caller supplies exclusive access to `len` writable bytes.
        // `write_bytes` initializes every byte before constructing the slice.
        unsafe {
            core::ptr::write_bytes(dst, 0, len);
            optimization_barrier(slice::from_raw_parts(dst, len));
        }
    } else {
        use core::mem::MaybeUninit;

        // SAFETY: the caller supplies exclusive access to this range. `MaybeUninit<u8>` permits
        // uninitialized bytes and has byte alignment.
        let bytes = unsafe { slice::from_raw_parts_mut(dst.cast::<MaybeUninit<u8>>(), len) };
        volatile_fill_slice(bytes, MaybeUninit::new(0));
    }
}

/// Fill a slice with a repeated value, preserving its type and alignment.
#[inline(always)]
fn volatile_fill_slice<T: Copy>(dst: &mut [T], src: T) {
    // Target 16-byte chunks
    match size_of::<T>() {
        0 => {}
        1 => volatile_fill_chunked::<_, 16>(dst, src),
        2 => volatile_fill_chunked::<_, 8>(dst, src),
        4 => volatile_fill_chunked::<_, 4>(dst, src),
        8 => volatile_fill_chunked::<_, 2>(dst, src),
        _ => volatile_fill_iterative(dst, src),
    }
}

/// Prepare the repeated pattern and handle the tail independently of the bulk backend.
#[inline(always)]
fn volatile_fill_chunked<T: Copy, const N: usize>(dst: &mut [T], src: T) {
    let (chunks, tail) = as_chunks_mut::<T, N>(dst);

    if !chunks.is_empty() {
        volatile_fill_iterative(chunks, [src; N]);
    }

    volatile_fill_iterative(tail, src);
}

/// Write individual elements or complete chunks with the same volatile primitive.
#[inline(always)]
fn volatile_fill_iterative<T: Copy>(dst: &mut [T], src: T) {
    for item in dst {
        volatile_write(item, src);
    }
}

/// MSRV-compatible copy of core's `slice::as_chunks_mut` taken from Rust 1.88.0.
/// Source: <https://github.com/rust-lang/rust/blob/1.88.0/library/core/src/slice/mod.rs>
// TODO(tarcieri): use `slice::as_chunks_mut` when MSRV is 1.88 or newer.
#[inline]
#[track_caller]
#[must_use]
const fn as_chunks_mut<T, const N: usize>(slice: &mut [T]) -> (&mut [[T; N]], &mut [T]) {
    assert!(N != 0, "chunk size must be non-zero");
    let len_rounded_down = slice.len() / N * N;
    // SAFETY: The rounded-down value is always the same or smaller than the
    // original length, and thus must be in-bounds of the slice.
    let (multiple_of_n, remainder) = unsafe { slice.split_at_mut_unchecked(len_rounded_down) };
    // SAFETY: We already panicked for zero, and ensured by construction
    // that the length of the subslice is a multiple of N. Arrays have the same alignment as T.
    let array_slice = unsafe {
        slice::from_raw_parts_mut(multiple_of_n.as_mut_ptr().cast(), multiple_of_n.len() / N)
    };
    (array_slice, remainder)
}
