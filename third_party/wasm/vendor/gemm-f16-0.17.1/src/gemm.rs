use dyn_stack::{DynStack, GlobalMemBuffer, StackReq};
#[cfg(feature = "std")]
use gemm_common::gemm::L2_SLAB;
#[cfg(feature = "rayon")]
use gemm_common::gemm::{get_threading_threshold, par_for_each};

use gemm_common::{
    cache::{kernel_params, DivCeil, KernelParams},
    gemm::CACHELINE_ALIGN,
    gemv, gevv,
    microkernel::MicroKernelFn,
    pack_operands::quick_zero,
    simd::{MixedSimd, NullaryFnOnce},
    Parallelism, Ptr,
};
type T = half::f16;

#[allow(unused_imports)]
use gemm_common::simd::*;

#[inline(always)]
unsafe fn pack_generic_inner_loop<
    const N: usize,
    const DST_WIDTH: usize,
    S: MixedSimd<T, T, T, f32>,
>(
    simd: S,
    mut dst: *mut f32,
    mut src: *const T,
    src_rs: isize,
    src_cs: isize,
    src_width: usize,
    k: usize,
) {
    assert_eq!(N, S::SIMD_WIDTH);

    if src_rs == 1 {
        #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
        {
            use core::any::TypeId;

            let id = TypeId::of::<S>();
            if id == TypeId::of::<V3>() {
                let half_simd = V3Half::try_new().unwrap();

                if src_width == 4 {
                    for _ in 0..k {
                        *(dst as *mut [f32; 4]) = half_simd.simd_from_dst(*(src as *const [T; 4]));
                        quick_zero::<f32>(core::slice::from_raw_parts_mut(
                            dst.add(src_width) as _,
                            DST_WIDTH - src_width,
                        ));
                        src = src.wrapping_offset(src_cs);
                        dst = dst.add(DST_WIDTH);
                    }
                    return;
                }
            }

            #[cfg(feature = "nightly")]
            if id == TypeId::of::<V4>() {
                let quarter_simd = V3Half::try_new().unwrap();
                let half_simd = V3::try_new().unwrap_unchecked();

                if src_width == 4 {
                    for _ in 0..k {
                        *(dst as *mut [f32; 4]) =
                            <V3Half as MixedSimd<T, T, T, f32>>::simd_from_dst(
                                quarter_simd,
                                *(src as *const [T; 4]),
                            );
                        quick_zero::<f32>(core::slice::from_raw_parts_mut(
                            dst.add(src_width) as _,
                            DST_WIDTH - src_width,
                        ));
                        src = src.wrapping_offset(src_cs);
                        dst = dst.add(DST_WIDTH);
                    }
                    return;
                }

                if src_width == 8 {
                    for _ in 0..k {
                        *(dst as *mut [f32; 8]) = <V3 as MixedSimd<T, T, T, f32>>::simd_from_dst(
                            half_simd,
                            *(src as *const [T; 8]),
                        );
                        quick_zero::<f32>(core::slice::from_raw_parts_mut(
                            dst.add(src_width) as _,
                            DST_WIDTH - src_width,
                        ));
                        src = src.wrapping_offset(src_cs);
                        dst = dst.add(DST_WIDTH);
                    }
                    return;
                }
            }
        }

        if src_width % N == 0 {
            for _ in 0..k {
                for j in 0..src_width / N {
                    let j = j * N;
                    let dst = dst.add(j) as *mut S::AccN;
                    *dst = simd.simd_from_dst(*(src.offset(j as isize * src_rs) as *const S::DstN));
                }
                src = src.wrapping_offset(src_cs);
                dst = dst.add(DST_WIDTH);
            }
            return;
        }
    }

    for _ in 0..k {
        for j in 0..src_width {
            *dst.add(j) = simd.from_lhs(*src.offset(j as isize * src_rs));
        }
        quick_zero::<f32>(core::slice::from_raw_parts_mut(
            dst.add(src_width) as _,
            DST_WIDTH - src_width,
        ));
        src = src.wrapping_offset(src_cs);
        dst = dst.add(DST_WIDTH);
    }
}

#[inline(always)]
unsafe fn pack_generic<const N: usize, const DST_WIDTH: usize, S: MixedSimd<T, T, T, f32>>(
    simd: S,
    m: usize,
    k: usize,
    mut dst: *mut f32,
    mut src: *const T,
    src_cs: isize,
    src_rs: isize,
    dst_stride: usize,
) {
    let m_width = m / DST_WIDTH * DST_WIDTH;

    let mut i = 0;
    while i < m_width {
        pack_generic_inner_loop::<N, DST_WIDTH, _>(simd, dst, src, src_rs, src_cs, DST_WIDTH, k);
        src = src.wrapping_offset(src_rs * DST_WIDTH as isize);
        dst = dst.add(dst_stride);

        i += DST_WIDTH;
    }
    if i < m {
        pack_generic_inner_loop::<N, DST_WIDTH, _>(simd, dst, src, src_rs, src_cs, m - i, k);
    }
}

#[inline(never)]
pub unsafe fn pack_lhs<const N: usize, const MR: usize, S: MixedSimd<T, T, T, f32>>(
    simd: S,
    m: usize,
    k: usize,
    dst: Ptr<f32>,
    src: Ptr<T>,
    src_cs: isize,
    src_rs: isize,
    dst_stride: usize,
) {
    let dst = dst.0;
    let src = src.0;
    struct Impl<const N: usize, const MR: usize, S> {
        simd: S,
        m: usize,
        k: usize,
        dst: *mut f32,
        src: *mut T,
        src_cs: isize,
        src_rs: isize,
        dst_stride: usize,
    }
    impl<const N: usize, const MR: usize, S: MixedSimd<T, T, T, f32>> NullaryFnOnce for Impl<N, MR, S> {
        type Output = ();

        #[inline(always)]
        fn call(self) -> Self::Output {
            let Self {
                simd,
                m,
                k,
                dst,
                src,
                src_cs,
                src_rs,
                dst_stride,
            } = self;
            unsafe { pack_generic::<N, MR, _>(simd, m, k, dst, src, src_cs, src_rs, dst_stride) };
        }
    }

    simd.vectorize(Impl::<N, MR, _> {
        simd,
        m,
        k,
        dst,
        src,
        src_cs,
        src_rs,
        dst_stride,
    });
}

#[inline(never)]
pub unsafe fn pack_rhs<const N: usize, const NR: usize, S: MixedSimd<T, T, T, f32>>(
    simd: S,
    n: usize,
    k: usize,
    dst: Ptr<f32>,
    src: Ptr<T>,
    src_cs: isize,
    src_rs: isize,
    dst_stride: usize,
) {
    let dst = dst.0;
    let src = src.0;

    struct Impl<const N: usize, const NR: usize, S> {
        simd: S,
        n: usize,
        k: usize,
        dst: *mut f32,
        src: *mut T,
        src_cs: isize,
        src_rs: isize,
        dst_stride: usize,
    }
    impl<const N: usize, const NR: usize, S: MixedSimd<T, T, T, f32>> NullaryFnOnce for Impl<N, NR, S> {
        type Output = ();

        #[inline(always)]
        fn call(self) -> Self::Output {
            let Self {
                simd,
                n,
                k,
                dst,
                src,
                src_cs,
                src_rs,
                dst_stride,
            } = self;
            unsafe { pack_generic::<N, NR, _>(simd, n, k, dst, src, src_rs, src_cs, dst_stride) };
        }
    }

    simd.vectorize(Impl::<N, NR, _> {
        simd,
        n,
        k,
        dst,
        src,
        src_cs,
        src_rs,
        dst_stride,
    });
}

#[inline(always)]
pub unsafe fn gemm_basic_generic<
    const N: usize,
    const MR: usize,
    const NR: usize,
    const MR_DIV_N: usize,
    S: MixedSimd<T, T, T, f32>,
>(
    simd: S,
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
    mut alpha: T,
    beta: T,
    dispatcher: &[[MicroKernelFn<f32>; NR]; MR_DIV_N],
    parallelism: Parallelism,
) {
    if m == 0 || n == 0 {
        return;
    }
    if !read_dst {
        alpha = T::ZERO;
    }

    if k == 0 {
        if alpha == T::ZERO {
            for j in 0..n {
                for i in 0..m {
                    *dst.offset(i as isize * dst_rs + j as isize * dst_cs) = T::ZERO;
                }
            }
            return;
        }
        if alpha == T::ONE {
            return;
        }

        for j in 0..n {
            for i in 0..m {
                let dst = dst.offset(i as isize * dst_rs + j as isize * dst_cs);
                *dst = alpha * *dst;
            }
        }
        return;
    }

    {
        if k <= 2 {
            gevv::gevv(
                simd,
                m,
                n,
                k,
                dst,
                dst_cs,
                dst_rs,
                lhs,
                lhs_cs,
                lhs_rs,
                rhs,
                rhs_cs,
                rhs_rs,
                alpha,
                beta,
                |a, b, c| {
                    simd.into_dst(simd.mult_add(
                        simd.from_dst(a),
                        simd.from_dst(b),
                        simd.from_dst(c),
                    ))
                },
            );
            return;
        }

        let alpha = simd.from_dst(alpha);
        let beta = simd.from_dst(beta);
        if n <= 1 && lhs_rs == 1 && dst_rs == 1 {
            gemv::mixed_gemv_colmajor(
                simd, m, n, k, dst, dst_cs, dst_rs, lhs, lhs_cs, lhs_rs, rhs, rhs_cs, rhs_rs,
                alpha, beta,
            );
            return;
        }
        if n <= 1 && lhs_cs == 1 && rhs_rs == 1 {
            gemv::mixed_gemv_rowmajor(
                simd, m, n, k, dst, dst_cs, dst_rs, lhs, lhs_cs, lhs_rs, rhs, rhs_cs, rhs_rs,
                alpha, beta,
            );
            return;
        }

        if m <= 1 && rhs_cs == 1 && dst_cs == 1 {
            gemv::mixed_gemv_colmajor(
                simd, n, m, k, dst, dst_rs, dst_cs, rhs, rhs_rs, rhs_cs, lhs, lhs_rs, lhs_cs,
                alpha, beta,
            );
            return;
        }
        if m <= 1 && rhs_rs == 1 && lhs_cs == 1 {
            gemv::mixed_gemv_rowmajor(
                simd, n, m, k, dst, dst_rs, dst_cs, rhs, rhs_rs, rhs_cs, lhs, lhs_rs, lhs_cs,
                alpha, beta,
            );
            return;
        }
    }

    let KernelParams { kc, mc, nc } = kernel_params(m, n, k, MR, NR, core::mem::size_of::<f32>());
    let nc = if nc > 0 {
        nc
    } else {
        match parallelism {
            Parallelism::None => 128 * NR,
            #[cfg(feature = "rayon")]
            Parallelism::Rayon(_) => n.msrv_next_multiple_of(NR),
        }
    };

    let simd_align = CACHELINE_ALIGN;

    let packed_rhs_stride = kc * NR;
    let packed_lhs_stride = kc * MR;

    let dst = Ptr(dst);
    let lhs = Ptr(lhs as *mut T);
    let rhs = Ptr(rhs as *mut T);

    let do_prepack_lhs = m <= 2 * mc && ((m % N != 0) || lhs_rs != 1);

    let rhs_req = StackReq::new_aligned::<f32>(packed_rhs_stride * (nc / NR), simd_align);
    let lhs_req = StackReq::new_aligned::<f32>(
        if do_prepack_lhs {
            packed_lhs_stride * (m.msrv_next_multiple_of(MR) / MR)
        } else {
            0
        },
        simd_align,
    );

    let mut mem = GlobalMemBuffer::new(rhs_req.and(lhs_req));
    #[cfg(not(feature = "std"))]
    let mut l2_slab = GlobalMemBuffer::new(StackReq::new_aligned::<f32>(
        packed_lhs_stride * (mc / MR),
        simd_align,
    ));

    let stack = DynStack::new(&mut mem);
    let (mut packed_rhs_storage, stack) =
        stack.make_aligned_uninit::<f32>(packed_rhs_stride * (nc / NR), simd_align);

    let mut packed_lhs_storage = stack
        .make_aligned_uninit::<f32>(
            if do_prepack_lhs {
                packed_lhs_stride * (m.msrv_next_multiple_of(MR) / MR)
            } else {
                0
            },
            simd_align,
        )
        .0;

    let packed_rhs = Ptr(packed_rhs_storage.as_mut_ptr() as *mut f32);
    let prepacked_lhs = Ptr(packed_lhs_storage.as_mut_ptr() as *mut f32);

    let packed_rhs_rs = NR as isize;
    let packed_rhs_cs = 1;

    let mut col_outer = 0;
    while col_outer != n {
        let n_chunk = nc.min(n - col_outer);

        let mut alpha = simd.from_lhs(alpha);

        let mut depth_outer = 0;
        while depth_outer != k {
            let k_chunk = kc.min(k - depth_outer);
            let alpha_status = if alpha == 0.0 {
                0
            } else if alpha == 1.0 {
                1
            } else {
                2
            };

            let n_threads = match parallelism {
                Parallelism::None => 1,
                #[cfg(feature = "rayon")]
                Parallelism::Rayon(n_threads) => {
                    let threading_threshold = get_threading_threshold();
                    let total_work = (m * n_chunk).saturating_mul(k_chunk);
                    if total_work < threading_threshold {
                        1
                    } else {
                        if n_threads == 0 {
                            rayon::current_num_threads()
                        } else {
                            n_threads
                        }
                    }
                }
            };

            // pack rhs
            if n_threads <= 1 {
                pack_rhs::<N, NR, _>(
                    simd,
                    n_chunk,
                    k_chunk,
                    packed_rhs,
                    rhs.wrapping_offset(
                        depth_outer as isize * rhs_rs + col_outer as isize * rhs_cs,
                    ),
                    rhs_cs,
                    rhs_rs,
                    packed_rhs_stride,
                );
            } else {
                #[cfg(feature = "rayon")]
                {
                    let n_tasks = n_chunk.msrv_div_ceil(NR);
                    let base = n_tasks / n_threads;
                    let rem = n_tasks % n_threads;

                    let tid_to_col_inner = |tid: usize| {
                        if tid == n_threads {
                            return n_chunk;
                        }

                        let col = if tid < rem {
                            NR * tid * (base + 1)
                        } else {
                            NR * (rem + tid * base)
                        };

                        col.min(n_chunk)
                    };

                    let func = |tid: usize| {
                        let col_inner = tid_to_col_inner(tid);
                        let ncols = tid_to_col_inner(tid + 1) - col_inner;
                        let j = col_inner / NR;

                        if ncols > 0 {
                            pack_rhs::<N, NR, _>(
                                simd,
                                ncols,
                                k_chunk,
                                packed_rhs.wrapping_add(j * packed_rhs_stride),
                                rhs.wrapping_offset(
                                    depth_outer as isize * rhs_rs
                                        + (col_outer + col_inner) as isize * rhs_cs,
                                ),
                                rhs_cs,
                                rhs_rs,
                                packed_rhs_stride,
                            );
                        }
                    };
                    par_for_each(n_threads, func);
                }

                #[cfg(not(feature = "rayon"))]
                unreachable!();
            }
            if do_prepack_lhs {
                pack_lhs::<N, MR, _>(
                    simd,
                    m,
                    k_chunk,
                    prepacked_lhs,
                    lhs.wrapping_offset(depth_outer as isize * lhs_cs),
                    lhs_cs,
                    lhs_rs,
                    packed_lhs_stride,
                );
            }

            let n_col_mini_chunks = (n_chunk + (NR - 1)) / NR;

            let mut n_jobs = 0;
            let mut row_outer = 0;
            while row_outer != m {
                let mut m_chunk = mc.min(m - row_outer);
                if m_chunk > N && !do_prepack_lhs {
                    m_chunk = m_chunk / N * N;
                }
                let n_row_mini_chunks = (m_chunk + (MR - 1)) / MR;
                n_jobs += n_col_mini_chunks * n_row_mini_chunks;
                row_outer += m_chunk;
            }

            // use a single thread for small workloads

            let func = move |tid, packed_lhs: Ptr<f32>| {
                let min_jobs_per_thread = n_jobs / n_threads;
                let rem = n_jobs - n_threads * min_jobs_per_thread;

                // thread `tid` takes min_jobs_per_thread or min_jobs_per_thread + 1
                let (job_start, job_end) = if tid < rem {
                    let start = tid * (min_jobs_per_thread + 1);
                    (start, start + min_jobs_per_thread + 1)
                } else {
                    // start = rem * (min_jobs_per_thread + 1) + (tid - rem) * min_jobs_per_thread;
                    let start = tid * min_jobs_per_thread + rem;
                    (start, start + min_jobs_per_thread)
                };

                let mut row_outer = 0;
                let mut job_id = 0;
                while row_outer != m {
                    let mut m_chunk = mc.min(m - row_outer);
                    if m_chunk > N && !do_prepack_lhs {
                        m_chunk = m_chunk / N * N;
                    }
                    let n_row_mini_chunks = (m_chunk + (MR - 1)) / MR;

                    let n_mini_jobs = n_col_mini_chunks * n_row_mini_chunks;

                    if job_id >= job_end {
                        return;
                    }
                    if job_id + n_mini_jobs < job_start {
                        row_outer += m_chunk;
                        job_id += n_mini_jobs;
                        continue;
                    }

                    let packed_lhs_cs = MR as isize;

                    if !do_prepack_lhs {
                        pack_lhs::<N, MR, _>(
                            simd,
                            m_chunk,
                            k_chunk,
                            packed_lhs,
                            lhs.wrapping_offset(
                                row_outer as isize * lhs_rs + depth_outer as isize * lhs_cs,
                            ),
                            lhs_cs,
                            lhs_rs,
                            packed_lhs_stride,
                        );
                    }

                    let mut j = 0;
                    while j < n_col_mini_chunks {
                        let mut i = 0;
                        while i < n_row_mini_chunks {
                            let col_inner = NR * j;
                            let n_chunk_inner = NR.min(n_chunk - col_inner);

                            let row_inner = MR * i;
                            let m_chunk_inner = MR.min(m_chunk - row_inner);

                            let inner_idx = &mut i;
                            if job_id < job_start || job_id >= job_end {
                                job_id += 1;
                                *inner_idx += 1;
                                continue;
                            }
                            job_id += 1;

                            let dst = dst.wrapping_offset(
                                (row_outer + row_inner) as isize * dst_rs
                                    + (col_outer + col_inner) as isize * dst_cs,
                            );

                            let func =
                                dispatcher[(m_chunk_inner + (N - 1)) / N - 1][n_chunk_inner - 1];

                            let mut tmp = [[0.0f32; MR]; NR];

                            func(
                                m_chunk_inner,
                                n_chunk_inner,
                                k_chunk,
                                tmp.as_mut_ptr() as *mut f32,
                                if do_prepack_lhs {
                                    packed_lhs
                                        .wrapping_add((i + row_outer / MR) * packed_lhs_stride)
                                        .0
                                } else {
                                    packed_lhs.wrapping_add(i * packed_lhs_stride).0
                                },
                                packed_rhs.wrapping_add(j * packed_rhs_stride).0,
                                MR as isize,
                                1,
                                packed_lhs_cs,
                                packed_rhs_rs,
                                packed_rhs_cs,
                                0.0,
                                beta.into(),
                                0,
                                false,
                                false,
                                false,
                                packed_lhs.wrapping_add((i + 1) * packed_lhs_stride).0,
                            );

                            match alpha_status {
                                0 => {
                                    for j in 0..n_chunk_inner {
                                        for i in 0..m_chunk_inner {
                                            let dst = dst
                                                .wrapping_offset(j as isize * dst_cs)
                                                .wrapping_offset(i as isize * dst_rs)
                                                .0;
                                            *dst = simd.into_dst(tmp[j][i]);
                                        }
                                    }
                                }
                                1 => {
                                    for j in 0..n_chunk_inner {
                                        for i in 0..m_chunk_inner {
                                            let dst = dst
                                                .wrapping_offset(j as isize * dst_cs)
                                                .wrapping_offset(i as isize * dst_rs)
                                                .0;
                                            *dst = simd.into_dst(simd.from_dst(*dst) + tmp[j][i]);
                                        }
                                    }
                                }
                                _ => {
                                    for j in 0..n_chunk_inner {
                                        for i in 0..m_chunk_inner {
                                            let dst = dst
                                                .wrapping_offset(j as isize * dst_cs)
                                                .wrapping_offset(i as isize * dst_rs)
                                                .0;
                                            *dst = simd
                                                .into_dst(alpha * simd.from_dst(*dst) + tmp[j][i]);
                                        }
                                    }
                                }
                            }

                            i += 1;
                        }
                        j += 1;
                    }

                    row_outer += m_chunk;
                }
            };

            if do_prepack_lhs {
                match parallelism {
                    Parallelism::None => func(0, prepacked_lhs),
                    #[cfg(feature = "rayon")]
                    Parallelism::Rayon(_) => {
                        if n_threads == 1 {
                            func(0, prepacked_lhs);
                        } else {
                            par_for_each(n_threads, |tid| func(tid, prepacked_lhs));
                        }
                    }
                }
            } else {
                #[cfg(feature = "std")]
                let func = |tid: usize| {
                    L2_SLAB.with(|mem| {
                        let mut mem = mem.borrow_mut();
                        let stack = DynStack::new(&mut mem);
                        let (mut packed_lhs_storage, _) = stack
                            .make_aligned_uninit::<f32>(packed_lhs_stride * (mc / MR), simd_align);
                        let packed_lhs = Ptr(packed_lhs_storage.as_mut_ptr() as *mut f32);
                        func(tid, packed_lhs);
                    });
                };

                #[cfg(not(feature = "std"))]
                let mut func = |tid: usize| {
                    let stack = DynStack::new(&mut l2_slab);
                    let (mut packed_lhs_storage, _) =
                        stack.make_aligned_uninit::<f32>(packed_lhs_stride * (mc / MR), simd_align);
                    let packed_lhs = Ptr(packed_lhs_storage.as_mut_ptr() as *mut f32);
                    func(tid, packed_lhs);
                };

                match parallelism {
                    Parallelism::None => func(0),
                    #[cfg(feature = "rayon")]
                    Parallelism::Rayon(_) => {
                        if n_threads == 1 {
                            func(0);
                        } else {
                            par_for_each(n_threads, func);
                        }
                    }
                }
            }

            alpha = 1.0;
            depth_outer += k_chunk;
        }
        col_outer += n_chunk;
    }
}

pub mod f16 {
    use super::gemm_basic_generic;
    use gemm_common::Parallelism;

    type T = half::f16;
    type GemmTy = unsafe fn(
        usize,
        usize,
        usize,
        *mut T,
        isize,
        isize,
        bool,
        *const T,
        isize,
        isize,
        *const T,
        isize,
        isize,
        T,
        T,
        bool,
        bool,
        bool,
        Parallelism,
    );

    fn init_gemm_fn() -> GemmTy {
        #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
        {
            #[cfg(feature = "nightly")]
            if gemm_common::feature_detected!("avx512f") {
                return avx512f::gemm_basic;
            }
            if gemm_common::feature_detected!("fma") {
                fma::gemm_basic
            } else {
                scalar::gemm_basic
            }
        }

        #[cfg(target_arch = "aarch64")]
        {
            if gemm_common::feature_detected!("neon") {
                #[cfg(feature = "experimental-apple-amx")]
                if gemm_common::cache::HasAmx::get() {
                    return amx::gemm_basic;
                }
                if gemm_common::feature_detected!("fp16") {
                    neonfp16::gemm_basic
                } else {
                    neon::gemm_basic
                }
            } else {
                scalar::gemm_basic
            }
        }

        #[cfg(not(any(target_arch = "x86", target_arch = "x86_64", target_arch = "aarch64")))]
        {
            scalar::gemm_basic
        }
    }

    static GEMM_PTR: ::core::sync::atomic::AtomicPtr<()> =
        ::core::sync::atomic::AtomicPtr::new(::core::ptr::null_mut());

    #[inline(never)]
    fn init_gemm_ptr() -> GemmTy {
        let gemm_fn = init_gemm_fn();
        GEMM_PTR.store(gemm_fn as *mut (), ::core::sync::atomic::Ordering::Relaxed);
        gemm_fn
    }

    #[inline(always)]
    pub fn get_gemm_fn() -> GemmTy {
        let mut gemm_fn = GEMM_PTR.load(::core::sync::atomic::Ordering::Relaxed);
        if gemm_fn.is_null() {
            gemm_fn = init_gemm_ptr() as *mut ();
        }
        unsafe { ::core::mem::transmute(gemm_fn) }
    }

    mod scalar {
        use super::*;
        use gemm_common::simd::Scalar;
        use gemm_f32::microkernel::scalar::f32::*;
        const N: usize = 1;

        #[inline(never)]
        pub unsafe fn gemm_basic(
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
            _conj_dst: bool,
            _conj_lhs: bool,
            _conj_rhs: bool,
            parallelism: gemm_common::Parallelism,
        ) {
            gemm_basic_generic::<N, { MR_DIV_N * N }, NR, MR_DIV_N, _>(
                Scalar,
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
                &UKR,
                parallelism,
            );
        }
    }

    #[cfg(target_arch = "aarch64")]
    mod neon {
        use super::*;
        use gemm_common::simd::MixedSimd;
        use gemm_f32::microkernel::neon::f32::*;
        const N: usize = 4;

        #[inline(never)]
        pub unsafe fn gemm_basic(
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
            _conj_dst: bool,
            _conj_lhs: bool,
            _conj_rhs: bool,
            parallelism: gemm_common::Parallelism,
        ) {
            gemm_basic_generic::<N, { MR_DIV_N * N }, NR, MR_DIV_N, _>(
                gemm_common::simd::Neon::try_new().unwrap(),
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
                &UKR,
                parallelism,
            );
        }
    }

    #[cfg(target_arch = "aarch64")]
    mod neonfp16 {
        use crate::microkernel::neonfp16::f16::*;
        use gemm_common::simd::{MixedSimd, NeonFp16};
        type T = half::f16;

        #[inline(never)]
        pub unsafe fn gemm_basic(
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
            _conj_dst: bool,
            _conj_lhs: bool,
            _conj_rhs: bool,
            parallelism: gemm_common::Parallelism,
        ) {
            let simd = <NeonFp16 as MixedSimd<T, T, T, T>>::try_new().unwrap();

            gemm_common::gemm::gemm_basic_generic::<_, _, N, { MR_DIV_N * N }, NR, MR_DIV_N>(
                simd,
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
                false,
                false,
                false,
                move |a, b, c| <NeonFp16 as MixedSimd<T, T, T, T>>::mult_add(simd, a, b, c),
                &UKR,
                false,
                parallelism,
            );
        }
    }

    #[cfg(target_arch = "aarch64")]
    #[cfg(feature = "experimental-apple-amx")]
    mod amx {
        use crate::microkernel::amx::f16::*;
        use gemm_common::simd::{MixedSimd, NeonFp16};
        type T = half::f16;

        #[inline(never)]
        pub unsafe fn gemm_basic(
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
            _conj_dst: bool,
            _conj_lhs: bool,
            _conj_rhs: bool,
            parallelism: gemm_common::Parallelism,
        ) {
            let simd = <NeonFp16 as MixedSimd<T, T, T, T>>::try_new().unwrap();

            gemm_common::gemm::gemm_basic_generic::<_, _, N, { MR_DIV_N * N }, NR, MR_DIV_N>(
                simd,
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
                false,
                false,
                false,
                move |a, b, c| <NeonFp16 as MixedSimd<T, T, T, T>>::mult_add(simd, a, b, c),
                &UKR,
                true,
                parallelism,
            );
        }
    }

    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    mod fma {
        use super::*;
        use gemm_common::simd::V3;
        use gemm_f32::microkernel::fma::f32::*;
        const N: usize = 8;

        #[inline(never)]
        pub unsafe fn gemm_basic(
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
            _conj_dst: bool,
            _conj_lhs: bool,
            _conj_rhs: bool,
            parallelism: gemm_common::Parallelism,
        ) {
            gemm_basic_generic::<N, { MR_DIV_N * N }, NR, MR_DIV_N, _>(
                V3::try_new().unwrap(),
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
                &UKR,
                parallelism,
            );
        }
    }

    #[cfg(all(feature = "nightly", any(target_arch = "x86", target_arch = "x86_64")))]
    mod avx512f {
        use super::*;
        use gemm_common::simd::V4;
        use gemm_f32::microkernel::avx512f::f32::*;
        const N: usize = 16;

        #[inline(never)]
        pub unsafe fn gemm_basic(
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
            _conj_dst: bool,
            _conj_lhs: bool,
            _conj_rhs: bool,
            parallelism: gemm_common::Parallelism,
        ) {
            gemm_basic_generic::<N, { MR_DIV_N * N }, NR, MR_DIV_N, _>(
                V4::try_new().unwrap(),
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
                &UKR,
                parallelism,
            );
        }
    }
}
