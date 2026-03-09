#![cfg_attr(
	all(target_arch = "x86_64", feature = "nightly"),
	feature(avx512_target_feature)
)]
#[cfg(all(target_arch = "x86_64", feature = "nightly"))]
mod x86 {
	use diol::prelude::*;
	use pulp::Simd;
	use pulp::x86::V4;
	use std::arch::x86_64::__m512d;

	fn bench_masked_store(bencher: Bencher, (): ()) {
		let simd = V4::try_new().unwrap();

		let mut x: __m512d = pulp::cast(simd.splat_f32s(0.0));
		let x: &mut [f32] = bytemuck::cast_slice_mut(core::slice::from_mut(&mut x));
		let x = x.as_mut_ptr();

		bencher.bench(|| {
			simd.vectorize(
				#[inline(always)]
				|| unsafe {
					let mask = simd.mask_between_m32s(3, 13);
					let raw_mask = mask.mask();
					mask_mem(simd, raw_mask.into(), x);
				},
			)
		});
	}

	#[inline]
	#[target_feature(enable = "avx512f")]
	unsafe fn mask_mem(simd: V4, mask: pulp::MemMask<pulp::b16>, x: *mut f32) {
		for _ in 0..16 {
			let y = simd.mask_load_ptr_f32s(mask, x);
			core::arch::asm!("/* */", in("zmm0") x);
			simd.mask_store_ptr_f32s(mask, x, y);
		}
	}

	fn bench_combined_stores(bencher: Bencher, (): ()) {
		let simd = V4::try_new().unwrap();

		let mut x: __m512d = pulp::cast(simd.splat_f32s(0.0));
		let x: &mut [f32] = bytemuck::cast_slice_mut(core::slice::from_mut(&mut x));
		let x = x.as_mut_ptr();

		bencher.bench(|| {
			simd.vectorize(
				#[inline(always)]
				|| unsafe {
					let mask = simd.mask_between_m32s(3, 13);
					for _ in 0..16 {
						simd.mask_store_ptr_f32s(mask, x, simd.mask_load_ptr_f32s(mask, x));
					}
				},
			)
		});
	}

	pub fn main() -> std::io::Result<()> {
		let mut bench = diol::Bench::new(BenchConfig::from_args()?);
		bench.register_many(list![bench_masked_store, bench_combined_stores], [()]);

		bench.run()?;
		Ok(())
	}
}

fn main() -> std::io::Result<()> {
	#[cfg(all(target_arch = "x86_64", feature = "nightly"))]
	x86::main()?;
	Ok(())
}
