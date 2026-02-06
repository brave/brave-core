use crate::simd::Simd;

#[inline(always)]
pub fn quick_zero<T: Copy>(slice: &mut [core::mem::MaybeUninit<T>]) {
    let n = slice.len();
    match n {
        1 => unsafe { *(slice.as_mut_ptr() as *mut [T; 1]) = core::mem::zeroed() },
        2 => unsafe { *(slice.as_mut_ptr() as *mut [T; 2]) = core::mem::zeroed() },
        3 => unsafe { *(slice.as_mut_ptr() as *mut [T; 3]) = core::mem::zeroed() },
        4 => unsafe { *(slice.as_mut_ptr() as *mut [T; 4]) = core::mem::zeroed() },
        5 => unsafe { *(slice.as_mut_ptr() as *mut [T; 5]) = core::mem::zeroed() },
        6 => unsafe { *(slice.as_mut_ptr() as *mut [T; 6]) = core::mem::zeroed() },
        7 => unsafe { *(slice.as_mut_ptr() as *mut [T; 7]) = core::mem::zeroed() },
        8 => unsafe { *(slice.as_mut_ptr() as *mut [T; 8]) = core::mem::zeroed() },
        9 => unsafe { *(slice.as_mut_ptr() as *mut [T; 9]) = core::mem::zeroed() },
        10 => unsafe { *(slice.as_mut_ptr() as *mut [T; 10]) = core::mem::zeroed() },
        11 => unsafe { *(slice.as_mut_ptr() as *mut [T; 11]) = core::mem::zeroed() },
        12 => unsafe { *(slice.as_mut_ptr() as *mut [T; 12]) = core::mem::zeroed() },
        13 => unsafe { *(slice.as_mut_ptr() as *mut [T; 13]) = core::mem::zeroed() },
        14 => unsafe { *(slice.as_mut_ptr() as *mut [T; 14]) = core::mem::zeroed() },
        15 => unsafe { *(slice.as_mut_ptr() as *mut [T; 15]) = core::mem::zeroed() },
        16 => unsafe { *(slice.as_mut_ptr() as *mut [T; 16]) = core::mem::zeroed() },
        17 => unsafe { *(slice.as_mut_ptr() as *mut [T; 17]) = core::mem::zeroed() },
        18 => unsafe { *(slice.as_mut_ptr() as *mut [T; 18]) = core::mem::zeroed() },
        19 => unsafe { *(slice.as_mut_ptr() as *mut [T; 19]) = core::mem::zeroed() },
        20 => unsafe { *(slice.as_mut_ptr() as *mut [T; 20]) = core::mem::zeroed() },
        21 => unsafe { *(slice.as_mut_ptr() as *mut [T; 21]) = core::mem::zeroed() },
        22 => unsafe { *(slice.as_mut_ptr() as *mut [T; 22]) = core::mem::zeroed() },
        23 => unsafe { *(slice.as_mut_ptr() as *mut [T; 23]) = core::mem::zeroed() },
        24 => unsafe { *(slice.as_mut_ptr() as *mut [T; 24]) = core::mem::zeroed() },
        25 => unsafe { *(slice.as_mut_ptr() as *mut [T; 25]) = core::mem::zeroed() },
        26 => unsafe { *(slice.as_mut_ptr() as *mut [T; 26]) = core::mem::zeroed() },
        27 => unsafe { *(slice.as_mut_ptr() as *mut [T; 27]) = core::mem::zeroed() },
        28 => unsafe { *(slice.as_mut_ptr() as *mut [T; 28]) = core::mem::zeroed() },
        29 => unsafe { *(slice.as_mut_ptr() as *mut [T; 29]) = core::mem::zeroed() },
        30 => unsafe { *(slice.as_mut_ptr() as *mut [T; 30]) = core::mem::zeroed() },
        31 => unsafe { *(slice.as_mut_ptr() as *mut [T; 31]) = core::mem::zeroed() },
        32 => unsafe { *(slice.as_mut_ptr() as *mut [T; 32]) = core::mem::zeroed() },
        33 => unsafe { *(slice.as_mut_ptr() as *mut [T; 33]) = core::mem::zeroed() },
        34 => unsafe { *(slice.as_mut_ptr() as *mut [T; 34]) = core::mem::zeroed() },
        35 => unsafe { *(slice.as_mut_ptr() as *mut [T; 35]) = core::mem::zeroed() },
        36 => unsafe { *(slice.as_mut_ptr() as *mut [T; 36]) = core::mem::zeroed() },
        37 => unsafe { *(slice.as_mut_ptr() as *mut [T; 37]) = core::mem::zeroed() },
        38 => unsafe { *(slice.as_mut_ptr() as *mut [T; 38]) = core::mem::zeroed() },
        39 => unsafe { *(slice.as_mut_ptr() as *mut [T; 39]) = core::mem::zeroed() },
        40 => unsafe { *(slice.as_mut_ptr() as *mut [T; 40]) = core::mem::zeroed() },
        41 => unsafe { *(slice.as_mut_ptr() as *mut [T; 41]) = core::mem::zeroed() },
        42 => unsafe { *(slice.as_mut_ptr() as *mut [T; 42]) = core::mem::zeroed() },
        43 => unsafe { *(slice.as_mut_ptr() as *mut [T; 43]) = core::mem::zeroed() },
        44 => unsafe { *(slice.as_mut_ptr() as *mut [T; 44]) = core::mem::zeroed() },
        45 => unsafe { *(slice.as_mut_ptr() as *mut [T; 45]) = core::mem::zeroed() },
        46 => unsafe { *(slice.as_mut_ptr() as *mut [T; 46]) = core::mem::zeroed() },
        47 => unsafe { *(slice.as_mut_ptr() as *mut [T; 47]) = core::mem::zeroed() },
        48 => unsafe { *(slice.as_mut_ptr() as *mut [T; 48]) = core::mem::zeroed() },
        49 => unsafe { *(slice.as_mut_ptr() as *mut [T; 49]) = core::mem::zeroed() },
        50 => unsafe { *(slice.as_mut_ptr() as *mut [T; 50]) = core::mem::zeroed() },
        51 => unsafe { *(slice.as_mut_ptr() as *mut [T; 51]) = core::mem::zeroed() },
        52 => unsafe { *(slice.as_mut_ptr() as *mut [T; 52]) = core::mem::zeroed() },
        53 => unsafe { *(slice.as_mut_ptr() as *mut [T; 53]) = core::mem::zeroed() },
        54 => unsafe { *(slice.as_mut_ptr() as *mut [T; 54]) = core::mem::zeroed() },
        55 => unsafe { *(slice.as_mut_ptr() as *mut [T; 55]) = core::mem::zeroed() },
        56 => unsafe { *(slice.as_mut_ptr() as *mut [T; 56]) = core::mem::zeroed() },
        57 => unsafe { *(slice.as_mut_ptr() as *mut [T; 57]) = core::mem::zeroed() },
        58 => unsafe { *(slice.as_mut_ptr() as *mut [T; 58]) = core::mem::zeroed() },
        59 => unsafe { *(slice.as_mut_ptr() as *mut [T; 59]) = core::mem::zeroed() },
        60 => unsafe { *(slice.as_mut_ptr() as *mut [T; 60]) = core::mem::zeroed() },
        61 => unsafe { *(slice.as_mut_ptr() as *mut [T; 61]) = core::mem::zeroed() },
        62 => unsafe { *(slice.as_mut_ptr() as *mut [T; 62]) = core::mem::zeroed() },
        63 => unsafe { *(slice.as_mut_ptr() as *mut [T; 63]) = core::mem::zeroed() },
        64 => unsafe { *(slice.as_mut_ptr() as *mut [T; 64]) = core::mem::zeroed() },
        _ => {
            for value in slice {
                *value = unsafe { core::mem::zeroed() };
            }
        }
    }
}

#[inline(always)]
unsafe fn quick_copy<T: Copy>(dst: *mut T, src: *const T, n: usize) {
    match n {
        1 => unsafe { *(dst as *mut [T; 1]) = *(src as *const [T; 1]) },
        2 => unsafe { *(dst as *mut [T; 2]) = *(src as *const [T; 2]) },
        3 => unsafe { *(dst as *mut [T; 3]) = *(src as *const [T; 3]) },
        4 => unsafe { *(dst as *mut [T; 4]) = *(src as *const [T; 4]) },
        5 => unsafe { *(dst as *mut [T; 5]) = *(src as *const [T; 5]) },
        6 => unsafe { *(dst as *mut [T; 6]) = *(src as *const [T; 6]) },
        7 => unsafe { *(dst as *mut [T; 7]) = *(src as *const [T; 7]) },
        8 => unsafe { *(dst as *mut [T; 8]) = *(src as *const [T; 8]) },
        9 => unsafe { *(dst as *mut [T; 9]) = *(src as *const [T; 9]) },
        10 => unsafe { *(dst as *mut [T; 10]) = *(src as *const [T; 10]) },
        11 => unsafe { *(dst as *mut [T; 11]) = *(src as *const [T; 11]) },
        12 => unsafe { *(dst as *mut [T; 12]) = *(src as *const [T; 12]) },
        13 => unsafe { *(dst as *mut [T; 13]) = *(src as *const [T; 13]) },
        14 => unsafe { *(dst as *mut [T; 14]) = *(src as *const [T; 14]) },
        15 => unsafe { *(dst as *mut [T; 15]) = *(src as *const [T; 15]) },
        16 => unsafe { *(dst as *mut [T; 16]) = *(src as *const [T; 16]) },
        17 => unsafe { *(dst as *mut [T; 17]) = *(src as *const [T; 17]) },
        18 => unsafe { *(dst as *mut [T; 18]) = *(src as *const [T; 18]) },
        19 => unsafe { *(dst as *mut [T; 19]) = *(src as *const [T; 19]) },
        20 => unsafe { *(dst as *mut [T; 20]) = *(src as *const [T; 20]) },
        21 => unsafe { *(dst as *mut [T; 21]) = *(src as *const [T; 21]) },
        22 => unsafe { *(dst as *mut [T; 22]) = *(src as *const [T; 22]) },
        23 => unsafe { *(dst as *mut [T; 23]) = *(src as *const [T; 23]) },
        24 => unsafe { *(dst as *mut [T; 24]) = *(src as *const [T; 24]) },
        25 => unsafe { *(dst as *mut [T; 25]) = *(src as *const [T; 25]) },
        26 => unsafe { *(dst as *mut [T; 26]) = *(src as *const [T; 26]) },
        27 => unsafe { *(dst as *mut [T; 27]) = *(src as *const [T; 27]) },
        28 => unsafe { *(dst as *mut [T; 28]) = *(src as *const [T; 28]) },
        29 => unsafe { *(dst as *mut [T; 29]) = *(src as *const [T; 29]) },
        30 => unsafe { *(dst as *mut [T; 30]) = *(src as *const [T; 30]) },
        31 => unsafe { *(dst as *mut [T; 31]) = *(src as *const [T; 31]) },
        32 => unsafe { *(dst as *mut [T; 32]) = *(src as *const [T; 32]) },
        33 => unsafe { *(dst as *mut [T; 33]) = *(src as *const [T; 33]) },
        34 => unsafe { *(dst as *mut [T; 34]) = *(src as *const [T; 34]) },
        35 => unsafe { *(dst as *mut [T; 35]) = *(src as *const [T; 35]) },
        36 => unsafe { *(dst as *mut [T; 36]) = *(src as *const [T; 36]) },
        37 => unsafe { *(dst as *mut [T; 37]) = *(src as *const [T; 37]) },
        38 => unsafe { *(dst as *mut [T; 38]) = *(src as *const [T; 38]) },
        39 => unsafe { *(dst as *mut [T; 39]) = *(src as *const [T; 39]) },
        40 => unsafe { *(dst as *mut [T; 40]) = *(src as *const [T; 40]) },
        41 => unsafe { *(dst as *mut [T; 41]) = *(src as *const [T; 41]) },
        42 => unsafe { *(dst as *mut [T; 42]) = *(src as *const [T; 42]) },
        43 => unsafe { *(dst as *mut [T; 43]) = *(src as *const [T; 43]) },
        44 => unsafe { *(dst as *mut [T; 44]) = *(src as *const [T; 44]) },
        45 => unsafe { *(dst as *mut [T; 45]) = *(src as *const [T; 45]) },
        46 => unsafe { *(dst as *mut [T; 46]) = *(src as *const [T; 46]) },
        47 => unsafe { *(dst as *mut [T; 47]) = *(src as *const [T; 47]) },
        48 => unsafe { *(dst as *mut [T; 48]) = *(src as *const [T; 48]) },
        49 => unsafe { *(dst as *mut [T; 49]) = *(src as *const [T; 49]) },
        50 => unsafe { *(dst as *mut [T; 50]) = *(src as *const [T; 50]) },
        51 => unsafe { *(dst as *mut [T; 51]) = *(src as *const [T; 51]) },
        52 => unsafe { *(dst as *mut [T; 52]) = *(src as *const [T; 52]) },
        53 => unsafe { *(dst as *mut [T; 53]) = *(src as *const [T; 53]) },
        54 => unsafe { *(dst as *mut [T; 54]) = *(src as *const [T; 54]) },
        55 => unsafe { *(dst as *mut [T; 55]) = *(src as *const [T; 55]) },
        56 => unsafe { *(dst as *mut [T; 56]) = *(src as *const [T; 56]) },
        57 => unsafe { *(dst as *mut [T; 57]) = *(src as *const [T; 57]) },
        58 => unsafe { *(dst as *mut [T; 58]) = *(src as *const [T; 58]) },
        59 => unsafe { *(dst as *mut [T; 59]) = *(src as *const [T; 59]) },
        60 => unsafe { *(dst as *mut [T; 60]) = *(src as *const [T; 60]) },
        61 => unsafe { *(dst as *mut [T; 61]) = *(src as *const [T; 61]) },
        62 => unsafe { *(dst as *mut [T; 62]) = *(src as *const [T; 62]) },
        63 => unsafe { *(dst as *mut [T; 63]) = *(src as *const [T; 63]) },
        64 => unsafe { *(dst as *mut [T; 64]) = *(src as *const [T; 64]) },
        _ => core::ptr::copy_nonoverlapping(src, dst, n),
    }
}

#[inline(always)]
unsafe fn pack_generic_inner_loop<T: Copy, const N: usize, const DST_WIDTH: usize>(
    mut dst: *mut T,
    mut src: *const T,
    src_rs: isize,
    src_cs: isize,
    src_width: usize,
    k: usize,
) {
    if src_width == DST_WIDTH {
        if src_rs == 1 {
            for _ in 0..k {
                let val = (src as *const [T; DST_WIDTH]).read();
                (dst as *mut [T; DST_WIDTH]).write(val);

                src = src.wrapping_offset(src_cs);
                dst = dst.add(DST_WIDTH);
            }
        } else {
            for _ in 0..k {
                for j in 0..DST_WIDTH {
                    *dst.add(j) = *src.offset(j as isize * src_rs);
                }
                src = src.wrapping_offset(src_cs);
                dst = dst.add(DST_WIDTH);
            }
        }
    } else if src_width == N {
        if src_rs == 1 {
            for _ in 0..k {
                let val = (src as *const [T; N]).read();
                (dst as *mut [T; N]).write(val);

                src = src.wrapping_offset(src_cs);
                dst = dst.add(DST_WIDTH);
            }
        } else {
            for _ in 0..k {
                for j in 0..N {
                    *dst.add(j) = *src.offset(j as isize * src_rs);
                }
                src = src.wrapping_offset(src_cs);
                dst = dst.add(DST_WIDTH);
            }
        }
    } else if src_width == 2 * N {
        if src_rs == 1 {
            for _ in 0..k {
                let val0 = (src as *const [T; N]).read();
                let val1 = (src.add(N) as *const [T; N]).read();
                (dst as *mut [T; N]).write(val0);
                (dst.add(N) as *mut [T; N]).write(val1);

                src = src.wrapping_offset(src_cs);
                dst = dst.add(DST_WIDTH);
            }
        } else {
            for _ in 0..k {
                for j in 0..2 * N {
                    *dst.add(j) = *src.offset(j as isize * src_rs);
                }
                src = src.wrapping_offset(src_cs);
                dst = dst.add(DST_WIDTH);
            }
        }
    } else {
        if src_rs == 1 {
            for _ in 0..k {
                quick_copy(dst, src, src_width);
                quick_zero::<T>(core::slice::from_raw_parts_mut(
                    dst.add(src_width) as _,
                    DST_WIDTH - src_width,
                ));
                src = src.wrapping_offset(src_cs);
                dst = dst.add(DST_WIDTH);
            }
        } else {
            for _ in 0..k {
                for j in 0..src_width {
                    *dst.add(j) = *src.offset(j as isize * src_rs);
                }
                quick_zero::<T>(core::slice::from_raw_parts_mut(
                    dst.add(src_width) as _,
                    DST_WIDTH - src_width,
                ));
                src = src.wrapping_offset(src_cs);
                dst = dst.add(DST_WIDTH);
            }
        }
    }
}

#[inline(always)]
unsafe fn pack_generic<T: Copy, const N: usize, const DST_WIDTH: usize>(
    m: usize,
    k: usize,
    mut dst: *mut T,
    mut src: *const T,
    src_cs: isize,
    src_rs: isize,
    dst_stride: usize,
) {
    let m_width = m / DST_WIDTH * DST_WIDTH;

    let mut i = 0;
    while i < m_width {
        pack_generic_inner_loop::<_, N, DST_WIDTH>(dst, src, src_rs, src_cs, DST_WIDTH, k);
        src = src.wrapping_offset(src_rs * DST_WIDTH as isize);
        dst = dst.add(dst_stride);

        i += DST_WIDTH;
    }
    if i < m {
        pack_generic_inner_loop::<_, N, DST_WIDTH>(dst, src, src_rs, src_cs, m - i, k);
    }
}

#[inline(never)]
pub unsafe fn pack_lhs<T: Copy, const N: usize, const MR: usize, S: Simd>(
    _: S,
    m: usize,
    k: usize,
    dst: crate::Ptr<T>,
    src: crate::Ptr<T>,
    src_cs: isize,
    src_rs: isize,
    dst_stride: usize,
) {
    let dst = dst.0;
    let src = src.0;
    S::vectorize(
        #[inline(always)]
        || pack_generic::<T, N, MR>(m, k, dst, src, src_cs, src_rs, dst_stride),
    );
}

#[inline(never)]
pub unsafe fn pack_rhs<T: Copy, const N: usize, const NR: usize, S: Simd>(
    _: S,
    n: usize,
    k: usize,
    dst: crate::Ptr<T>,
    src: crate::Ptr<T>,
    src_cs: isize,
    src_rs: isize,
    dst_stride: usize,
) {
    let dst = dst.0;
    let src = src.0;
    S::vectorize(
        #[inline(always)]
        || pack_generic::<T, N, NR>(n, k, dst, src, src_rs, src_cs, dst_stride),
    );
}
