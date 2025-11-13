use crate::Parallelism;
use core::any::TypeId;

#[allow(non_camel_case_types)]
pub type c32 = num_complex::Complex32;
#[allow(non_camel_case_types)]
pub type c64 = num_complex::Complex64;
#[cfg(feature = "f16")]
#[allow(non_camel_case_types)]
pub type f16 = gemm_f16::f16;

unsafe fn gemm_dispatch<T: 'static>(
    m: usize,
    n: usize,
    k: usize,
    dst: *mut T,
    dst_cs: isize,
    dst_rs: isize,
    read_dst: bool,
    lhs: *const T,
    lhs_cs: isize,
    lhs_rs: isize,
    rhs: *const T,
    rhs_cs: isize,
    rhs_rs: isize,
    alpha: T,
    beta: T,
    conj_dst: bool,
    conj_lhs: bool,
    conj_rhs: bool,
    parallelism: Parallelism,
) {
    #[cfg(feature = "f16")]
    if TypeId::of::<T>() == TypeId::of::<f16>() {
        return gemm_f16::gemm::f16::get_gemm_fn()(
            m,
            n,
            k,
            dst as *mut f16,
            dst_cs,
            dst_rs,
            read_dst,
            lhs as *mut f16,
            lhs_cs,
            lhs_rs,
            rhs as *mut f16,
            rhs_cs,
            rhs_rs,
            *(&alpha as *const T as *const f16),
            *(&beta as *const T as *const f16),
            false,
            false,
            false,
            parallelism,
        );
    }

    if TypeId::of::<T>() == TypeId::of::<f64>() {
        gemm_f64::gemm::f64::get_gemm_fn()(
            m,
            n,
            k,
            dst as *mut f64,
            dst_cs,
            dst_rs,
            read_dst,
            lhs as *mut f64,
            lhs_cs,
            lhs_rs,
            rhs as *mut f64,
            rhs_cs,
            rhs_rs,
            *(&alpha as *const T as *const f64),
            *(&beta as *const T as *const f64),
            false,
            false,
            false,
            parallelism,
        )
    } else if TypeId::of::<T>() == TypeId::of::<f32>() {
        gemm_f32::gemm::f32::get_gemm_fn()(
            m,
            n,
            k,
            dst as *mut f32,
            dst_cs,
            dst_rs,
            read_dst,
            lhs as *mut f32,
            lhs_cs,
            lhs_rs,
            rhs as *mut f32,
            rhs_cs,
            rhs_rs,
            *(&alpha as *const T as *const f32),
            *(&beta as *const T as *const f32),
            false,
            false,
            false,
            parallelism,
        )
    } else if TypeId::of::<T>() == TypeId::of::<c64>() {
        gemm_c64::gemm::f64::get_gemm_fn()(
            m,
            n,
            k,
            dst as *mut c64,
            dst_cs,
            dst_rs,
            read_dst,
            lhs as *mut c64,
            lhs_cs,
            lhs_rs,
            rhs as *mut c64,
            rhs_cs,
            rhs_rs,
            *(&alpha as *const T as *const c64),
            *(&beta as *const T as *const c64),
            conj_dst,
            conj_lhs,
            conj_rhs,
            parallelism,
        )
    } else if TypeId::of::<T>() == TypeId::of::<c32>() {
        gemm_c32::gemm::f32::get_gemm_fn()(
            m,
            n,
            k,
            dst as *mut c32,
            dst_cs,
            dst_rs,
            read_dst,
            lhs as *mut c32,
            lhs_cs,
            lhs_rs,
            rhs as *mut c32,
            rhs_cs,
            rhs_rs,
            *(&alpha as *const T as *const c32),
            *(&beta as *const T as *const c32),
            conj_dst,
            conj_lhs,
            conj_rhs,
            parallelism,
        )
    } else {
        panic!();
    }
}

/// dst := alpha×dst + beta×lhs×rhs
///
/// # Panics
///
/// Panics if `T` is not `f32`, `f64`, `gemm::f16`, `gemm::c32`, or `gemm::c64`.
pub unsafe fn gemm<T: 'static>(
    m: usize,
    n: usize,
    k: usize,
    mut dst: *mut T,
    dst_cs: isize,
    dst_rs: isize,
    read_dst: bool,
    lhs: *const T,
    lhs_cs: isize,
    lhs_rs: isize,
    rhs: *const T,
    rhs_cs: isize,
    rhs_rs: isize,
    alpha: T,
    beta: T,
    conj_dst: bool,
    conj_lhs: bool,
    conj_rhs: bool,
    parallelism: Parallelism,
) {
    // we want to transpose if the destination is column-oriented, since the microkernel prefers
    // column major matrices.
    let do_transpose = dst_cs.abs() < dst_rs.abs();

    let (
        m,
        n,
        mut dst_cs,
        mut dst_rs,
        mut lhs,
        lhs_cs,
        mut lhs_rs,
        mut rhs,
        mut rhs_cs,
        rhs_rs,
        conj_lhs,
        conj_rhs,
    ) = if do_transpose {
        (
            n, m, dst_rs, dst_cs, rhs, rhs_rs, rhs_cs, lhs, lhs_rs, lhs_cs, conj_rhs, conj_lhs,
        )
    } else {
        (
            m, n, dst_cs, dst_rs, lhs, lhs_cs, lhs_rs, rhs, rhs_cs, rhs_rs, conj_lhs, conj_rhs,
        )
    };

    if dst_rs < 0 && m > 0 {
        dst = dst.wrapping_offset((m - 1) as isize * dst_rs);
        dst_rs = -dst_rs;
        lhs = lhs.wrapping_offset((m - 1) as isize * lhs_rs);
        lhs_rs = -lhs_rs;
    }

    if dst_cs < 0 && n > 0 {
        dst = dst.wrapping_offset((n - 1) as isize * dst_cs);
        dst_cs = -dst_cs;
        rhs = rhs.wrapping_offset((n - 1) as isize * rhs_cs);
        rhs_cs = -rhs_cs;
    }

    gemm_dispatch(
        m,
        n,
        k,
        dst,
        dst_cs,
        dst_rs,
        read_dst,
        lhs,
        lhs_cs,
        lhs_rs,
        rhs,
        rhs_cs,
        rhs_rs,
        alpha,
        beta,
        conj_dst,
        conj_lhs,
        conj_rhs,
        parallelism,
    )
}

#[inline(never)]
#[cfg(test)]
pub unsafe fn gemm_fallback<T>(
    m: usize,
    n: usize,
    k: usize,
    dst: *mut T,
    dst_cs: isize,
    dst_rs: isize,
    read_dst: bool,
    lhs: *const T,
    lhs_cs: isize,
    lhs_rs: isize,
    rhs: *const T,
    rhs_cs: isize,
    rhs_rs: isize,
    alpha: T,
    beta: T,
) where
    T: num_traits::Zero + Send + Sync,
    for<'a> &'a T: core::ops::Add<&'a T, Output = T>,
    for<'a> &'a T: core::ops::Mul<&'a T, Output = T>,
{
    (0..m).for_each(|row| {
        (0..n).for_each(|col| {
            let mut accum = <T as num_traits::Zero>::zero();
            for depth in 0..k {
                let lhs = &*lhs.wrapping_offset(row as isize * lhs_rs + depth as isize * lhs_cs);

                let rhs = &*rhs.wrapping_offset(depth as isize * rhs_rs + col as isize * rhs_cs);

                accum = &accum + &(lhs * rhs);
            }
            accum = &accum * &beta;

            let dst = dst.wrapping_offset(row as isize * dst_rs + col as isize * dst_cs);
            if read_dst {
                accum = &accum + &(&alpha * &*dst);
            }
            *dst = accum
        });
    });
    return;
}

#[inline(never)]
#[cfg(test)]
pub(crate) unsafe fn gemm_cplx_fallback<T>(
    m: usize,
    n: usize,
    k: usize,
    dst: *mut num_complex::Complex<T>,
    dst_cs: isize,
    dst_rs: isize,
    read_dst: bool,
    lhs: *const num_complex::Complex<T>,
    lhs_cs: isize,
    lhs_rs: isize,
    rhs: *const num_complex::Complex<T>,
    rhs_cs: isize,
    rhs_rs: isize,
    alpha: num_complex::Complex<T>,
    beta: num_complex::Complex<T>,
    conj_dst: bool,
    conj_lhs: bool,
    conj_rhs: bool,
) where
    T: num_traits::Zero + Send + Sync + Copy + num_traits::Num + core::ops::Neg<Output = T>,
    for<'a> &'a T: core::ops::Add<&'a T, Output = T>,
    for<'a> &'a T: core::ops::Sub<&'a T, Output = T>,
    for<'a> &'a T: core::ops::Mul<&'a T, Output = T>,
{
    (0..m).for_each(|row| {
        (0..n).for_each(|col| {
            let mut accum = <num_complex::Complex<T> as num_traits::Zero>::zero();
            for depth in 0..k {
                let lhs = &*lhs.wrapping_offset(row as isize * lhs_rs + depth as isize * lhs_cs);
                let rhs = &*rhs.wrapping_offset(depth as isize * rhs_rs + col as isize * rhs_cs);

                match (conj_lhs, conj_rhs) {
                    (true, true) => accum = &accum + &(lhs.conj() * rhs.conj()),
                    (true, false) => accum = &accum + &(lhs.conj() * rhs),
                    (false, true) => accum = &accum + &(lhs * rhs.conj()),
                    (false, false) => accum = &accum + &(lhs * rhs),
                }
            }
            accum = &accum * &beta;

            let dst = dst.wrapping_offset(row as isize * dst_rs + col as isize * dst_cs);
            if read_dst {
                match conj_dst {
                    true => accum = &accum + &(&alpha * (*dst).conj()),
                    false => accum = &accum + &(&alpha * &*dst),
                }
            }
            *dst = accum
        });
    });
    return;
}
