use core::slice::from_raw_parts_mut;

use num_traits::{One, Zero};
use seq_macro::seq;

use crate::simd::{Boilerplate, MixedSimd, Simd};

#[inline(always)]
pub unsafe fn gemv<
    T: Copy
        + Zero
        + One
        + Send
        + Sync
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

    macro_rules! do_work {
        ($n: tt) => {
            for depth in 0..k {
                seq!(COL in 0..$n {
                    let rhs~COL = beta * *rhs
                        .wrapping_offset(COL as isize * rhs_cs)
                        .wrapping_offset(depth as isize * rhs_rs);
                });
                for row in 0..m {
                    let lhs = *lhs
                        .wrapping_offset(depth as isize * lhs_cs)
                        .wrapping_offset(row as isize * lhs_rs);

                    seq!(COL in 0..$n {
                        {
                            let dst = dst
                                .wrapping_offset(COL as isize * dst_cs)
                                .wrapping_offset(row as isize * dst_rs);
                            *dst = mul_add(rhs~COL, lhs, *dst);
                        }
                    });
                }
            }
        }
    }
    match n {
        1 => do_work!(1),
        _ => unreachable!(),
    }
}

// dst, lhs are colmajor
// n is small
#[inline(always)]
pub unsafe fn mixed_gemv_colmajor<
    Lhs: Boilerplate + One + Zero,
    Rhs: Boilerplate + One + Zero,
    Dst: Boilerplate + One + Zero,
    Acc: Boilerplate + One + Zero,
    S: MixedSimd<Lhs, Rhs, Dst, Acc>,
>(
    simd: S,

    m: usize,
    n: usize,
    k: usize,

    dst: *mut Dst,
    dst_cs: isize,
    dst_rs: isize,

    lhs: *const Lhs,
    lhs_cs: isize,
    lhs_rs: isize,

    rhs: *const Rhs,
    rhs_cs: isize,
    rhs_rs: isize,

    alpha: Acc,
    beta: Acc,
) {
    #[inline(always)]
    unsafe fn implementation<
        'a,
        Lhs: Boilerplate + One + Zero,
        Rhs: Boilerplate + One + Zero,
        Dst: Boilerplate + One + Zero,
        Acc: Boilerplate + One + Zero,
        S: MixedSimd<Lhs, Rhs, Dst, Acc>,
    >(
        noalias_dst: (&'a mut [Dst],),
        simd: S,
        m: usize,
        k: usize,
        lhs: *const Lhs,
        lhs_cs: isize,
        rhs: *const Rhs,
        rhs_cs: isize,
        rhs_rs: isize,
        alpha: Acc,
        beta: Acc,
    ) {
        #[allow(dead_code)]
        struct Impl<'a, Lhs, Rhs, Dst, Acc, S> {
            simd: S,
            m: usize,
            k: usize,
            noalias_dst: (&'a mut [Dst],),
            lhs: *const Lhs,
            lhs_cs: isize,
            rhs: *const Rhs,
            rhs_cs: isize,
            rhs_rs: isize,
            alpha: Acc,
            beta: Acc,
        }
        impl<
                Lhs: Boilerplate + One + Zero,
                Rhs: Boilerplate + One + Zero,
                Dst: Boilerplate + One + Zero,
                Acc: Boilerplate + One + Zero,
                S: MixedSimd<Lhs, Rhs, Dst, Acc>,
            > pulp::NullaryFnOnce for Impl<'_, Lhs, Rhs, Dst, Acc, S>
        {
            type Output = ();

            #[inline(always)]
            fn call(self) -> Self::Output {
                unsafe {
                    let Self {
                        simd,
                        m,
                        k,
                        noalias_dst,
                        lhs,
                        lhs_cs,
                        rhs,
                        rhs_cs: _,
                        rhs_rs,
                        mut alpha,
                        beta,
                    } = self;

                    let lane = S::SIMD_WIDTH;
                    let dst = noalias_dst.0.as_mut_ptr();
                    let m_lane = m / lane * lane;
                    for col in 0..k {
                        let lhs = lhs.wrapping_offset(col as isize * lhs_cs);
                        let rhs = simd.from_rhs(*rhs.wrapping_offset(col as isize * rhs_rs));

                        let alpha_s = alpha;
                        let alpha_v = simd.simd_splat(alpha_s);

                        let rhs_scalar = simd.mult(beta, rhs);
                        let rhs = simd.simd_splat(rhs_scalar);

                        if alpha_s.is_zero() {
                            let mut row = 0usize;
                            while row < m_lane {
                                let dst_ptr = dst.wrapping_add(row) as *mut S::DstN;
                                let lhs =
                                    simd.simd_from_lhs(*(lhs.wrapping_add(row) as *const S::LhsN));
                                *dst_ptr = simd.simd_into_dst(simd.simd_mul(lhs, rhs));
                                row += lane;
                            }
                            while row < m {
                                let dst_ptr = dst.wrapping_add(row);
                                let lhs = simd.from_lhs(*lhs.wrapping_add(row));
                                *dst_ptr = simd.into_dst(simd.mult(lhs, rhs_scalar));
                                row += 1;
                            }
                        } else if alpha_s.is_one() {
                            let mut row = 0usize;
                            while row < m_lane {
                                let dst_ptr = dst.wrapping_add(row) as *mut S::DstN;
                                let dst = *dst_ptr;
                                let lhs =
                                    simd.simd_from_lhs(*(lhs.wrapping_add(row) as *const S::LhsN));
                                *dst_ptr = simd.simd_into_dst(simd.simd_mult_add(
                                    lhs,
                                    rhs,
                                    simd.simd_from_dst(dst),
                                ));
                                row += lane;
                            }
                            while row < m {
                                let dst_ptr = dst.wrapping_add(row);
                                let dst = *dst_ptr;
                                let lhs = simd.from_lhs(*lhs.wrapping_add(row));
                                *dst_ptr = simd.into_dst(simd.mult_add(
                                    lhs,
                                    rhs_scalar,
                                    simd.from_dst(dst),
                                ));
                                row += 1;
                            }
                        } else {
                            let mut row = 0usize;
                            while row < m_lane {
                                let dst_ptr = dst.wrapping_add(row) as *mut S::DstN;
                                let dst = *dst_ptr;
                                let lhs =
                                    simd.simd_from_lhs(*(lhs.wrapping_add(row) as *const S::LhsN));
                                *dst_ptr = simd.simd_into_dst(simd.simd_add(
                                    simd.simd_mul(lhs, rhs),
                                    simd.simd_mul(alpha_v, simd.simd_from_dst(dst)),
                                ));
                                row += lane;
                            }
                            while row < m {
                                let dst_ptr = dst.wrapping_add(row);
                                let dst = *dst_ptr;
                                let lhs = simd.from_lhs(*lhs.wrapping_add(row));
                                *dst_ptr = simd.into_dst(simd.add(
                                    simd.mult(lhs, rhs_scalar),
                                    simd.mult(alpha_s, simd.from_dst(dst)),
                                ));
                                row += 1;
                            }
                        }
                        alpha = Acc::one();
                    }
                }
            }
        }

        simd.vectorize(Impl {
            simd,
            m,
            k,
            noalias_dst,
            lhs,
            lhs_cs,
            rhs,
            rhs_cs,
            rhs_rs,
            alpha,
            beta,
        })
    }

    assert_eq!(lhs_rs, 1);
    assert_eq!(dst_rs, 1);

    if k == 0 {
        if alpha.is_one() {
            return;
        }
        if alpha.is_zero() {
            for j in 0..n {
                core::ptr::write_bytes(dst.wrapping_offset(j as isize * dst_cs), 0u8, m);
            }
            return;
        }

        for j in 0..n {
            let dst = dst.wrapping_offset(j as isize * dst_cs);
            for i in 0..m {
                let dst = dst.add(i);
                *dst = simd.into_dst(simd.mult(simd.from_dst(*dst), alpha));
            }
        }
    }

    for x in 0..n {
        implementation(
            (from_raw_parts_mut(
                dst.wrapping_offset(x as isize * dst_cs) as _,
                m,
            ),),
            simd,
            m,
            k,
            lhs,
            lhs_cs,
            rhs.wrapping_offset(rhs_cs * x as isize),
            rhs_cs,
            rhs_rs,
            alpha,
            beta,
        );
    }
}

// lhs is rowmajor
// rhs is colmajor
// n is small
#[inline(always)]
pub unsafe fn mixed_gemv_rowmajor<
    Lhs: Boilerplate + One + Zero,
    Rhs: Boilerplate + One + Zero,
    Dst: Boilerplate + One + Zero,
    Acc: Boilerplate + One + Zero,
    S: MixedSimd<Lhs, Rhs, Dst, Acc>,
>(
    simd: S,

    m: usize,
    n: usize,
    k: usize,

    dst: *mut Dst,
    dst_cs: isize,
    dst_rs: isize,

    lhs: *const Lhs,
    lhs_cs: isize,
    lhs_rs: isize,

    rhs: *const Rhs,
    rhs_cs: isize,
    rhs_rs: isize,

    alpha: Acc,
    beta: Acc,
) {
    #[inline(always)]
    unsafe fn implementation<
        'a,
        Lhs: Boilerplate + One + Zero,
        Rhs: Boilerplate + One + Zero,
        Dst: Boilerplate + One + Zero,
        Acc: Boilerplate + One + Zero,
        S: MixedSimd<Lhs, Rhs, Dst, Acc>,
    >(
        simd: S,
        dst: *mut Dst,
        dst_rs: isize,
        m: usize,
        k: usize,
        lhs: *const Lhs,
        lhs_rs: isize,
        rhs: *const Rhs,
        alpha: Acc,
        beta: Acc,
    ) {
        #[allow(dead_code)]
        struct Impl<Lhs, Rhs, Dst, Acc, S> {
            simd: S,
            dst: *mut Dst,
            dst_rs: isize,
            m: usize,
            k: usize,
            lhs: *const Lhs,
            lhs_rs: isize,
            rhs: *const Rhs,
            alpha: Acc,
            beta: Acc,
        }
        impl<
                Lhs: Boilerplate + One + Zero,
                Rhs: Boilerplate + One + Zero,
                Dst: Boilerplate + One + Zero,
                Acc: Boilerplate + One + Zero,
                S: MixedSimd<Lhs, Rhs, Dst, Acc>,
            > pulp::NullaryFnOnce for Impl<Lhs, Rhs, Dst, Acc, S>
        {
            type Output = ();

            #[inline(always)]
            fn call(self) -> Self::Output {
                unsafe {
                    let Self {
                        simd,
                        dst,
                        dst_rs,
                        m,
                        k,
                        lhs,
                        lhs_rs,
                        rhs,
                        alpha,
                        beta,
                    } = self;

                    let lane = S::SIMD_WIDTH;
                    let lane8 = 8 * S::SIMD_WIDTH;

                    let k_lane = k / lane * lane;
                    let k_lane8 = k / lane8 * lane8;

                    for row in 0..m {
                        let lhs = lhs.wrapping_offset(row as isize * lhs_rs);

                        let mut depth = 0;

                        let mut acc0 = simd.simd_splat(Acc::zero());
                        let mut acc1 = simd.simd_splat(Acc::zero());
                        let mut acc2 = simd.simd_splat(Acc::zero());
                        let mut acc3 = simd.simd_splat(Acc::zero());
                        let mut acc4 = simd.simd_splat(Acc::zero());
                        let mut acc5 = simd.simd_splat(Acc::zero());
                        let mut acc6 = simd.simd_splat(Acc::zero());
                        let mut acc7 = simd.simd_splat(Acc::zero());

                        while depth < k_lane8 {
                            let lhs0 = *(lhs.wrapping_add(depth + lane * 0) as *const S::LhsN);
                            let rhs0 = *(rhs.wrapping_add(depth + lane * 0) as *const S::RhsN);
                            acc0 = simd.simd_mult_add(
                                simd.simd_from_lhs(lhs0),
                                simd.simd_from_rhs(rhs0),
                                acc0,
                            );

                            let lhs1 = *(lhs.wrapping_add(depth + lane * 1) as *const S::LhsN);
                            let rhs1 = *(rhs.wrapping_add(depth + lane * 1) as *const S::RhsN);
                            acc1 = simd.simd_mult_add(
                                simd.simd_from_lhs(lhs1),
                                simd.simd_from_rhs(rhs1),
                                acc1,
                            );

                            let lhs2 = *(lhs.wrapping_add(depth + lane * 2) as *const S::LhsN);
                            let rhs2 = *(rhs.wrapping_add(depth + lane * 2) as *const S::RhsN);
                            acc2 = simd.simd_mult_add(
                                simd.simd_from_lhs(lhs2),
                                simd.simd_from_rhs(rhs2),
                                acc2,
                            );

                            let lhs3 = *(lhs.wrapping_add(depth + lane * 3) as *const S::LhsN);
                            let rhs3 = *(rhs.wrapping_add(depth + lane * 3) as *const S::RhsN);
                            acc3 = simd.simd_mult_add(
                                simd.simd_from_lhs(lhs3),
                                simd.simd_from_rhs(rhs3),
                                acc3,
                            );

                            let lhs4 = *(lhs.wrapping_add(depth + lane * 4) as *const S::LhsN);
                            let rhs4 = *(rhs.wrapping_add(depth + lane * 4) as *const S::RhsN);
                            acc4 = simd.simd_mult_add(
                                simd.simd_from_lhs(lhs4),
                                simd.simd_from_rhs(rhs4),
                                acc4,
                            );

                            let lhs5 = *(lhs.wrapping_add(depth + lane * 5) as *const S::LhsN);
                            let rhs5 = *(rhs.wrapping_add(depth + lane * 5) as *const S::RhsN);
                            acc5 = simd.simd_mult_add(
                                simd.simd_from_lhs(lhs5),
                                simd.simd_from_rhs(rhs5),
                                acc5,
                            );

                            let lhs6 = *(lhs.wrapping_add(depth + lane * 6) as *const S::LhsN);
                            let rhs6 = *(rhs.wrapping_add(depth + lane * 6) as *const S::RhsN);
                            acc6 = simd.simd_mult_add(
                                simd.simd_from_lhs(lhs6),
                                simd.simd_from_rhs(rhs6),
                                acc6,
                            );

                            let lhs7 = *(lhs.wrapping_add(depth + lane * 7) as *const S::LhsN);
                            let rhs7 = *(rhs.wrapping_add(depth + lane * 7) as *const S::RhsN);
                            acc7 = simd.simd_mult_add(
                                simd.simd_from_lhs(lhs7),
                                simd.simd_from_rhs(rhs7),
                                acc7,
                            );

                            depth += lane8;
                        }

                        let acc0 = simd.simd_add(acc0, acc1);
                        let acc2 = simd.simd_add(acc2, acc3);
                        let acc4 = simd.simd_add(acc4, acc5);
                        let acc6 = simd.simd_add(acc6, acc7);

                        let acc0 = simd.simd_add(acc0, acc2);
                        let acc4 = simd.simd_add(acc4, acc6);

                        let mut acc0 = simd.simd_add(acc0, acc4);

                        while depth < k_lane {
                            let lhs0 = *(lhs.wrapping_add(depth) as *const S::LhsN);
                            let rhs0 = *(rhs.wrapping_add(depth) as *const S::RhsN);
                            acc0 = simd.simd_mult_add(
                                simd.simd_from_lhs(lhs0),
                                simd.simd_from_rhs(rhs0),
                                acc0,
                            );

                            depth += lane;
                        }

                        let acc_ptr = &acc0 as *const _ as *const Acc;
                        let mut acc0 = *acc_ptr;
                        for x in 1..S::SIMD_WIDTH {
                            acc0 = simd.add(acc0, *acc_ptr.add(x));
                        }

                        while depth < k {
                            let lhs0 = *(lhs.wrapping_add(depth + 0));
                            let rhs0 = *(rhs.wrapping_add(depth + 0));

                            acc0 = simd.mult_add(simd.from_lhs(lhs0), simd.from_rhs(rhs0), acc0);

                            depth += 1;
                        }

                        if alpha.is_zero() {
                            let dst = dst.wrapping_offset(dst_rs * row as isize);
                            *dst = simd.into_dst(simd.mult(acc0, beta));
                        } else {
                            let dst = dst.wrapping_offset(dst_rs * row as isize);
                            *dst =
                                simd.into_dst(simd.add(
                                    simd.mult(acc0, beta),
                                    simd.mult(simd.from_dst(*dst), alpha),
                                ));
                        }
                    }
                }
            }
        }

        simd.vectorize(Impl {
            simd,
            dst,
            dst_rs,
            m,
            k,
            lhs,
            lhs_rs,
            rhs,
            alpha,
            beta,
        })
    }

    assert_eq!(lhs_cs, 1);
    assert_eq!(rhs_rs, 1);

    for x in 0..n {
        implementation(
            simd,
            dst.wrapping_offset(x as isize * dst_cs),
            dst_rs,
            m,
            k,
            lhs,
            lhs_rs,
            rhs.wrapping_offset(rhs_cs * x as isize),
            alpha,
            beta,
        );
    }
}
