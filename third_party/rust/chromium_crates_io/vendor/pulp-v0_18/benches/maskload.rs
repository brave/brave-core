use aligned_vec::avec;
use diol::prelude::*;
use pulp::x86::V3;
use pulp::{m64x4, Offset, Read, Simd, Write};

fn masked(bencher: Bencher, PlotArg(n): PlotArg) {
    if let Some(simd) = V3::try_new() {
        let dst = &mut *avec![1.0; 8];
        let src = &*avec![1.0; 8];

        let dst: &mut [f64; 7] = (&mut dst[1..]).try_into().unwrap();
        let src: &[f64; 7] = (&src[1..]).try_into().unwrap();
        let offset = simd.f64s_align_offset(dst.as_ptr(), dst.len());

        bencher.bench(|| {
            struct Impl<'a> {
                simd: V3,
                dst: &'a mut [f64],
                src: &'a [f64],
                n: usize,
                offset: Offset<m64x4>,
            }

            impl pulp::NullaryFnOnce for Impl<'_> {
                type Output = ();

                #[inline(always)]
                fn call(self) -> Self::Output {
                    let Self {
                        simd,
                        dst,
                        src,
                        n,
                        offset,
                    } = self;

                    let (mut dst_prefix, _, mut dst_suffix) =
                        simd.f64s_as_aligned_mut_simd(dst, offset);
                    let (src_prefix, _, src_suffix) = simd.f64s_as_aligned_simd(src, offset);
                    for _ in 0..n {
                        dst_prefix.write(src_prefix.read_or(simd.splat_f64x4(0.0)));
                        dst_suffix.write(src_suffix.read_or(simd.splat_f64x4(0.0)));
                        core::hint::black_box((&mut dst_prefix, &mut dst_suffix));
                    }
                }
            }

            simd.vectorize(Impl {
                simd,
                dst,
                src,
                n,
                offset,
            })
        })
    }
}

fn non_masked(bencher: Bencher, PlotArg(n): PlotArg) {
    let dst = &mut *avec![1.0; 8];
    let src = &*avec![1.0; 8];

    let mut dst: &mut [f64; 7] = &mut dst[1..].try_into().unwrap();
    let src: &[f64; 7] = &src[1..].try_into().unwrap();

    bencher.bench(move || {
        for _ in 0..n {
            dst.copy_from_slice(src);
            core::hint::black_box(&mut dst);
        }
    })
}

fn main() -> std::io::Result<()> {
    let mut bench = Bench::new(BenchConfig::from_args()?);

    bench.register_many(
        list![masked, non_masked],
        [1, 2, 3, 4, 16, 32, 128, 16384].map(PlotArg),
    );
    bench.run()?;

    Ok(())
}
