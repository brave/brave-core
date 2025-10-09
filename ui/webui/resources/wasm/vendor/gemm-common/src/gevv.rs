use crate::simd::Simd;
use num_traits::{One, Zero};

#[inline(always)]
pub unsafe fn gevv<
    T: Copy
        + Zero
        + One
        + Send
        + Sync
        + core::fmt::Debug
        + core::ops::Add<Output = T>
        + core::ops::Mul<Output = T>
        + core::cmp::PartialEq,
    S: Simd,
>(
    _simd: S,
    m: usize,
    n: usize,
    k: usize,
    dst: *mut T,
    dst_cs: isize,
    dst_rs: isize,
    lhs: *const T,
    lhs_cs: isize,
    lhs_rs: isize,
    rhs: *const T,
    rhs_cs: isize,
    rhs_rs: isize,
    alpha: T,
    beta: T,
    mul_add: impl Fn(T, T, T) -> T,
) {
    macro_rules! do_work {
        () => {
            match k {
                0 => {
                    if !alpha.is_zero() {
                        for col in 0..n {
                            for row in 0..m {
                                let dst = dst
                                    .wrapping_offset(row as isize * dst_rs)
                                    .wrapping_offset(col as isize * dst_cs);

                                *dst = alpha * *dst;
                            }
                        }
                    } else {
                        for col in 0..n {
                            for row in 0..m {
                                let dst = dst
                                    .wrapping_offset(row as isize * dst_rs)
                                    .wrapping_offset(col as isize * dst_cs);

                                *dst = T::zero();
                            }
                        }
                    }
                    return;
                }
                1 => {
                    if !alpha.is_zero() {
                        if alpha.is_one() {
                            for col in 0..n {
                                let rhs = beta * *rhs.wrapping_offset(col as isize * rhs_cs);
                                for row in 0..m {
                                    let lhs = *lhs.wrapping_offset(row as isize * lhs_rs);
                                    let dst = dst
                                        .wrapping_offset(row as isize * dst_rs)
                                        .wrapping_offset(col as isize * dst_cs);

                                    *dst = mul_add(lhs, rhs, *dst);
                                }
                            }
                        } else {
                            for col in 0..n {
                                let rhs = beta * *rhs.wrapping_offset(col as isize * rhs_cs);
                                for row in 0..m {
                                    let lhs = *lhs.wrapping_offset(row as isize * lhs_rs);
                                    let dst = dst
                                        .wrapping_offset(row as isize * dst_rs)
                                        .wrapping_offset(col as isize * dst_cs);

                                    *dst = mul_add(lhs, rhs, alpha * *dst);
                                }
                            }
                        }
                    } else {
                        for col in 0..n {
                            let rhs = beta * *rhs.wrapping_offset(col as isize * rhs_cs);
                            for row in 0..m {
                                let lhs = *lhs.wrapping_offset(row as isize * lhs_rs);
                                let dst = dst
                                    .wrapping_offset(row as isize * dst_rs)
                                    .wrapping_offset(col as isize * dst_cs);

                                *dst = lhs * rhs;
                            }
                        }
                    }
                    return;
                }
                2 => {
                    if !alpha.is_zero() {
                        if alpha.is_one() {
                            for col in 0..n {
                                let rhs0 =
                                    beta * *rhs.wrapping_offset(col as isize * rhs_cs + 0 * rhs_rs);
                                let rhs1 =
                                    beta * *rhs.wrapping_offset(col as isize * rhs_cs + 1 * rhs_rs);
                                for row in 0..m {
                                    let lhs0 =
                                        *lhs.wrapping_offset(row as isize * lhs_rs + 0 * lhs_cs);
                                    let lhs1 =
                                        *lhs.wrapping_offset(row as isize * lhs_rs + 1 * lhs_cs);
                                    let dst = dst
                                        .wrapping_offset(row as isize * dst_rs)
                                        .wrapping_offset(col as isize * dst_cs);

                                    *dst = mul_add(lhs1, rhs1, mul_add(lhs0, rhs0, *dst));
                                }
                            }
                        } else {
                            for col in 0..n {
                                let rhs0 =
                                    beta * *rhs.wrapping_offset(col as isize * rhs_cs + 0 * rhs_rs);
                                let rhs1 =
                                    beta * *rhs.wrapping_offset(col as isize * rhs_cs + 1 * rhs_rs);
                                for row in 0..m {
                                    let lhs0 =
                                        *lhs.wrapping_offset(row as isize * lhs_rs + 0 * lhs_cs);
                                    let lhs1 =
                                        *lhs.wrapping_offset(row as isize * lhs_rs + 1 * lhs_cs);
                                    let dst = dst
                                        .wrapping_offset(row as isize * dst_rs)
                                        .wrapping_offset(col as isize * dst_cs);

                                    *dst = mul_add(lhs1, rhs1, mul_add(lhs0, rhs0, alpha * *dst));
                                }
                            }
                        }
                    } else {
                        for col in 0..n {
                            let rhs0 =
                                beta * *rhs.wrapping_offset(col as isize * rhs_cs + 0 * rhs_rs);
                            let rhs1 =
                                beta * *rhs.wrapping_offset(col as isize * rhs_cs + 1 * rhs_rs);
                            for row in 0..m {
                                let lhs0 = *lhs.wrapping_offset(row as isize * lhs_rs + 0 * lhs_cs);
                                let lhs1 = *lhs.wrapping_offset(row as isize * lhs_rs + 1 * lhs_cs);
                                let dst = dst
                                    .wrapping_offset(row as isize * dst_rs)
                                    .wrapping_offset(col as isize * dst_cs);

                                *dst = mul_add(lhs1, rhs1, lhs0 * rhs0);
                            }
                        }
                    }
                    return;
                }
                _ => unreachable!(),
            }
        };
    }
    do_work!()
}
