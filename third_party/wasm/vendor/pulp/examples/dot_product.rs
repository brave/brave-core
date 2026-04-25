use aligned_vec::avec;
use core::iter;
use diol::prelude::*;
use pulp::{Arch, Simd};

// dot product
// sum x * y
pub fn dot_product_scalar(x: &[f32], y: &[f32]) -> f32 {
	let mut acc = 0.0;
	for (x, y) in iter::zip(x, y) {
		acc += x * y;
	}
	acc
}
pub fn bench_dot_scalar(bencher: Bencher, PlotArg(n): PlotArg) {
	let x = &*vec![1.0_f32; n];
	let y = &*vec![1.0_f32; n];

	bencher.bench(|| dot_product_scalar(x, y))
}

#[allow(dead_code)]
mod inline_examples {
	// inline hint: compiler pls inline this
	#[inline]
	fn foo0(x: i32, y: i32) -> i32 {
		x + y
	}

	// strong inline hint: compiler i really need you to inline this
	// needed for simd
	#[inline(always)]
	fn foo1(x: i32, y: i32) -> i32 {
		x + y
	}

	// no_inline hint: compiler pls inline this
	#[inline(never)]
	fn foo2(x: i32, y: i32) -> i32 {
		x + y
	}

	fn bar(x: i32, y: i32) -> i32 {
		foo0(x, y) + foo1(x, 16)
	}
}

#[cfg(target_arch = "x86_64")]
mod x86 {
	use super::*;

	use pulp::x86::V3;
	use pulp::{cast, f32x8};

	// x86/x86_64
	// - V3 simd uses 256bit registers (f32x8)   => 16 registers
	// - V4 simd uses 512bit registers (f32x16)  => 32 registers (for consumer cpus, only available
	//   on 11th gen intel, and amd zen4 (or zen5?))
	//
	// arm/aarch64
	// typically register size of 128bit (f32x4) => 32 registers

	pub fn dot_product_simd_v3(simd: V3, x: &[f32], y: &[f32]) -> f32 {
		// essentially emulating a closure because closures don't always respect
		// #[inline(always)], which we need for the compiler to inline the simd intrinsics
		struct Impl<'a> {
			simd: V3,
			x: &'a [f32],
			y: &'a [f32],
		}

		impl pulp::NullaryFnOnce for Impl<'_> {
			type Output = f32;

			// PERF: must be #[inline(always)]
			#[inline(always)]
			fn call(self) -> Self::Output {
				let Self { simd, x, y } = self;

				// x0 x1 x2.. x100
				// [[x0 x1 x2 x3 x4 x5 x6 x7] [x8 x9 x10.. x15] [..x95]] | [x96 x97 x98 x99 x100]
				let (x8, x1) = pulp::as_arrays::<8, _>(x);
				let (y8, y1) = pulp::as_arrays::<8, _>(y);

				let mut acc = 0.0;
				for (x, y) in iter::zip(x8, y8) {
					let x: f32x8 = cast(*x);
					let y: f32x8 = cast(*y);

					acc += simd.reduce_sum_f32s(simd.mul_f32x8(x, y));
				}
				for (x, y) in iter::zip(x1, y1) {
					acc += x * y;
				}

				acc
			}
		}

		simd.vectorize(Impl { simd, x, y })
	}

	pub fn dot_product_simd_extract_reduce_v3(simd: V3, x: &[f32], y: &[f32]) -> f32 {
		struct Impl<'a> {
			simd: V3,
			x: &'a [f32],
			y: &'a [f32],
		}

		impl pulp::NullaryFnOnce for Impl<'_> {
			type Output = f32;

			#[inline(always)]
			fn call(self) -> Self::Output {
				let Self { simd, x, y } = self;

				// x0 x1 x2.. x100
				// [[x0 x1 x2 x3 x4 x5 x6 x7] [x8 x9 x10.. x15] [..x95]] | [x96 x97 x98 x99 x100]
				let (x8, x1) = pulp::as_arrays::<8, _>(x);
				let (y8, y1) = pulp::as_arrays::<8, _>(y);

				// sum (x * y)
				// sum (reduce_sum(X * Y))
				// reduce_sum(sum (X * Y))

				// [0.0; 8]
				let mut acc = simd.splat_f32x8(0.0);
				for (x, y) in iter::zip(x8, y8) {
					let x: f32x8 = cast(*x);
					let y: f32x8 = cast(*y);

					acc = simd.add_f32x8(acc, simd.mul_f32x8(x, y));
				}
				// reduce_sum_f32s
				// f32x8 -> f32x4 + f32x4
				// f32x4 -> f32x2 + f32x2
				// f32x2 -> f32 + f32
				let mut acc = simd.reduce_sum_f32s(acc);

				for (x, y) in iter::zip(x1, y1) {
					acc += x * y;
				}

				acc
			}
		}

		simd.vectorize(Impl { simd, x, y })
	}

	// ilp: instruction-level-parallelism
	// out of order execution
	pub fn dot_product_simd_extract_reduce_ilp_v3(simd: V3, x: &[f32], y: &[f32]) -> f32 {
		struct Impl<'a> {
			simd: V3,
			x: &'a [f32],
			y: &'a [f32],
		}

		impl pulp::NullaryFnOnce for Impl<'_> {
			type Output = f32;

			#[inline(always)]
			fn call(self) -> Self::Output {
				let Self { simd, x, y } = self;

				// x0 x1 x2.. x100
				// [[x0 x1 x2 x3 x4 x5 x6 x7] [x8 x9 x10.. x15] [..x95]] | [x96 x97 x98 x99 x100]
				let (x8, x1) = pulp::as_arrays::<8, _>(x);
				let (y8, y1) = pulp::as_arrays::<8, _>(y);

				// sum (x * y)
				// sum (reduce_sum(X * Y))
				// reduce_sum(sum (X * Y))

				let mut acc0 = simd.splat_f32x8(0.0);
				let mut acc1 = simd.splat_f32x8(0.0);
				let mut acc2 = simd.splat_f32x8(0.0);
				let mut acc3 = simd.splat_f32x8(0.0);

				// 12 registers are being used
				// 4 for accumulators + 2×4 inside the loop for x[0|1] and y[0|1]
				let (x8_4, x8_1) = pulp::as_arrays::<4, _>(x8);
				let (y8_4, y8_1) = pulp::as_arrays::<4, _>(y8);

				for ([x0, x1, x2, x3], [y0, y1, y2, y3]) in iter::zip(x8_4, y8_4) {
					let x0: f32x8 = cast(*x0);
					let y0: f32x8 = cast(*y0);
					let x1: f32x8 = cast(*x1);
					let y1: f32x8 = cast(*y1);
					let x2: f32x8 = cast(*x2);
					let y2: f32x8 = cast(*y2);
					let x3: f32x8 = cast(*x3);
					let y3: f32x8 = cast(*y3);

					acc0 = simd.add_f32x8(acc0, simd.mul_f32x8(x0, y0));
					acc1 = simd.add_f32x8(acc1, simd.mul_f32x8(x1, y1));
					acc2 = simd.add_f32x8(acc2, simd.mul_f32x8(x2, y2));
					acc3 = simd.add_f32x8(acc3, simd.mul_f32x8(x3, y3));
				}

				for (x0, y0) in iter::zip(x8_1, y8_1) {
					let x0: f32x8 = cast(*x0);
					let y0: f32x8 = cast(*y0);
					acc0 = simd.add_f32x8(acc0, simd.mul_f32x8(x0, y0));
				}

				// reduce_sum_f32s
				// f32x8 -> f32x4 + f32x4
				// f32x4 -> f32x2 + f32x2
				// f32x2 -> f32 + f32
				acc0 = simd.add_f32x8(acc0, acc1);
				acc2 = simd.add_f32x8(acc2, acc3);

				acc0 = simd.add_f32x8(acc0, acc2);

				let mut acc = simd.reduce_sum_f32s(acc0);

				for (x, y) in iter::zip(x1, y1) {
					acc += x * y;
				}

				acc
			}
		}

		simd.vectorize(Impl { simd, x, y })
	}

	// fma: fused multiply add
	pub fn dot_product_simd_extract_reduce_ilp_fma_v3(simd: V3, x: &[f32], y: &[f32]) -> f32 {
		struct Impl<'a> {
			simd: V3,
			x: &'a [f32],
			y: &'a [f32],
		}

		impl pulp::NullaryFnOnce for Impl<'_> {
			type Output = f32;

			#[inline(always)]
			fn call(self) -> Self::Output {
				let Self { simd, x, y } = self;

				// x0 x1 x2.. x100
				// [[x0 x1 x2 x3 x4 x5 x6 x7] [x8 x9 x10.. x15] [..x95]] | [x96 x97 x98 x99 x100]
				let (x8, x1) = pulp::as_arrays::<8, _>(x);
				let (y8, y1) = pulp::as_arrays::<8, _>(y);

				// sum (x * y)
				// sum (reduce_sum(X * Y))
				// reduce_sum(sum (X * Y))

				let mut acc0 = simd.splat_f32x8(0.0);
				let mut acc1 = simd.splat_f32x8(0.0);
				let mut acc2 = simd.splat_f32x8(0.0);
				let mut acc3 = simd.splat_f32x8(0.0);

				// 12 registers are being used
				// 4 for accumulators + 2×4 inside the loop for x[0|1] and y[0|1]
				let (x8_4, x8_1) = pulp::as_arrays::<4, _>(x8);
				let (y8_4, y8_1) = pulp::as_arrays::<4, _>(y8);

				for ([x0, x1, x2, x3], [y0, y1, y2, y3]) in iter::zip(x8_4, y8_4) {
					let x0 = cast(*x0);
					let y0 = cast(*y0);
					let x1 = cast(*x1);
					let y1 = cast(*y1);
					let x2 = cast(*x2);
					let y2 = cast(*y2);
					let x3 = cast(*x3);
					let y3 = cast(*y3);

					acc0 = simd.mul_add_f32x8(x0, y0, acc0);
					acc1 = simd.mul_add_f32x8(x1, y1, acc1);
					acc2 = simd.mul_add_f32x8(x2, y2, acc2);
					acc3 = simd.mul_add_f32x8(x3, y3, acc3);
				}

				for (x0, y0) in iter::zip(x8_1, y8_1) {
					let x0 = cast(*x0);
					let y0 = cast(*y0);
					acc0 = simd.mul_add_f32x8(x0, y0, acc0);
				}

				// reduce_sum_f32s
				// f32x8 -> f32x4 + f32x4
				// f32x4 -> f32x2 + f32x2
				// f32x2 -> f32 + f32
				acc0 = simd.add_f32x8(acc0, acc1);
				acc2 = simd.add_f32x8(acc2, acc3);

				acc0 = simd.add_f32x8(acc0, acc2);

				let mut acc = simd.reduce_sum_f32s(acc0);

				for (x, y) in iter::zip(x1, y1) {
					acc += x * y;
				}

				acc
			}
		}

		simd.vectorize(Impl { simd, x, y })
	}

	pub fn bench_dot_simd(bencher: Bencher, PlotArg(n): PlotArg) {
		let x = &*vec![1.0_f32; n];
		let y = &*vec![1.0_f32; n];

		if let Some(simd) = V3::try_new() {
			bencher.bench(|| dot_product_simd_v3(simd, x, y))
		} else {
			bencher.skip();
		}
	}
	pub fn bench_dot_simd_extract_reduce(bencher: Bencher, PlotArg(n): PlotArg) {
		let x = &*vec![1.0_f32; n];
		let y = &*vec![1.0_f32; n];

		if let Some(simd) = V3::try_new() {
			bencher.bench(|| dot_product_simd_extract_reduce_v3(simd, x, y))
		} else {
			bencher.skip();
		}
	}

	pub fn bench_dot_simd_extract_reduce_ilp(bencher: Bencher, PlotArg(n): PlotArg) {
		let x = &*vec![1.0_f32; n];
		let y = &*vec![1.0_f32; n];

		if let Some(simd) = V3::try_new() {
			bencher.bench(|| dot_product_simd_extract_reduce_ilp_v3(simd, x, y))
		} else {
			bencher.skip();
		}
	}

	pub fn bench_dot_simd_extract_reduce_ilp_fma(bencher: Bencher, PlotArg(n): PlotArg) {
		let x = &*vec![1.0_f32; n];
		let y = &*vec![1.0_f32; n];

		dbg!(x.as_ptr().addr() % 32);
		dbg!(y.as_ptr().addr() % 32);

		if let Some(simd) = V3::try_new() {
			bencher.bench(|| dot_product_simd_extract_reduce_ilp_fma_v3(simd, x, y))
		} else {
			bencher.skip();
		}
	}

	pub fn bench_dot_simd_extract_reduce_ilp_fma_misaligned(bencher: Bencher, PlotArg(n): PlotArg) {
		let x = &avec![1.0_f32; n + 1][1..];
		let y = &avec![1.0_f32; n + 1][1..];

		if let Some(simd) = V3::try_new() {
			bencher.bench(|| dot_product_simd_extract_reduce_ilp_fma_v3(simd, x, y))
		} else {
			bencher.skip();
		}
	}

	pub fn bench_dot_simd_extract_reduce_ilp_fma_aligned(bencher: Bencher, PlotArg(n): PlotArg) {
		// aligned memory is more efficient for simd loads and stores
		// always use this for benchmarks
		//
		// we didn't use it in the previous benchmarks just to showcase the difference
		let x = &*avec![1.0_f32; n];
		let y = &*avec![1.0_f32; n];

		if let Some(simd) = V3::try_new() {
			bencher.bench(|| dot_product_simd_extract_reduce_ilp_fma_v3(simd, x, y))
		} else {
			bencher.skip();
		}
	}
}

// fma: fused multiply add
pub fn dot_product_simd_extract_reduce_ilp_fma_generic<S: Simd>(
	simd: S,
	x: &[f32],
	y: &[f32],
) -> f32 {
	struct Impl<'a, S> {
		simd: S,
		x: &'a [f32],
		y: &'a [f32],
	}

	impl<S: Simd> pulp::NullaryFnOnce for Impl<'_, S> {
		type Output = f32;

		#[inline(always)]
		fn call(self) -> Self::Output {
			let Self { simd, x, y } = self;

			// x0 x1 x2.. x100
			// [[x0 x1 x2 x3 x4 x5 x6 x7] [x8 x9 x10.. x15] [..x95]] | [x96 x97 x98 x99 x100]
			let (xs, x1) = S::as_simd_f32s(x);
			let (ys, y1) = S::as_simd_f32s(y);

			// sum (x * y)
			// sum (reduce_sum(X * Y))
			// reduce_sum(sum (X * Y))

			let mut acc0 = simd.splat_f32s(0.0);
			let mut acc1 = simd.splat_f32s(0.0);
			let mut acc2 = simd.splat_f32s(0.0);
			let mut acc3 = simd.splat_f32s(0.0);

			// 12 registers are being used
			// 4 for accumulators + 2×4 inside the loop for x[0|1] and y[0|1]
			let (xs_4, xs_1) = pulp::as_arrays::<4, _>(xs);
			let (ys_4, ys_1) = pulp::as_arrays::<4, _>(ys);

			for ([x0, x1, x2, x3], [y0, y1, y2, y3]) in iter::zip(xs_4, ys_4) {
				acc0 = simd.mul_add_f32s(*x0, *y0, acc0);
				acc1 = simd.mul_add_f32s(*x1, *y1, acc1);
				acc2 = simd.mul_add_f32s(*x2, *y2, acc2);
				acc3 = simd.mul_add_f32s(*x3, *y3, acc3);
			}

			for (x0, y0) in iter::zip(xs_1, ys_1) {
				acc0 = simd.mul_add_f32s(*x0, *y0, acc0);
			}

			// reduce_sum_f32s
			// f32x8 -> f32x4 + f32x4
			// f32x4 -> f32x2 + f32x2
			// f32x2 -> f32 + f32
			acc0 = simd.add_f32s(acc0, acc1);
			acc2 = simd.add_f32s(acc2, acc3);

			acc0 = simd.add_f32s(acc0, acc2);

			let mut acc = simd.reduce_sum_f32s(acc0);

			for (x, y) in iter::zip(x1, y1) {
				acc += x * y;
			}

			acc
		}
	}

	simd.vectorize(Impl { simd, x, y })
}

// fma: fused multiply add
pub fn dot_product_simd_extract_reduce_ilp_fma_epilogue_generic<S: Simd>(
	simd: S,
	x: &[f32],
	y: &[f32],
) -> f32 {
	struct Impl<'a, S> {
		simd: S,
		x: &'a [f32],
		y: &'a [f32],
	}

	impl<S: Simd> pulp::NullaryFnOnce for Impl<'_, S> {
		type Output = f32;

		#[inline(always)]
		fn call(self) -> Self::Output {
			let Self { simd, x, y } = self;

			// x0 x1 x2.. x100
			// [[x0 x1 x2 x3 x4 x5 x6 x7] [x8 x9 x10.. x15] [..x95]] | [x96 x97 x98 x99 x100]
			let (xs, x1) = S::as_simd_f32s(x);
			let (ys, y1) = S::as_simd_f32s(y);

			// sum (x * y)
			// sum (reduce_sum(X * Y))
			// reduce_sum(sum (X * Y))

			let mut acc0 = simd.splat_f32s(0.0);
			let mut acc1 = simd.splat_f32s(0.0);
			let mut acc2 = simd.splat_f32s(0.0);
			let mut acc3 = simd.splat_f32s(0.0);

			// 12 registers are being used
			// 4 for accumulators + 2×4 inside the loop for x[0|1] and y[0|1]
			let (xs_4, xs_1) = pulp::as_arrays::<4, _>(xs);
			let (ys_4, ys_1) = pulp::as_arrays::<4, _>(ys);

			for ([x0, x1, x2, x3], [y0, y1, y2, y3]) in iter::zip(xs_4, ys_4) {
				acc0 = simd.mul_add_f32s(*x0, *y0, acc0);
				acc1 = simd.mul_add_f32s(*x1, *y1, acc1);
				acc2 = simd.mul_add_f32s(*x2, *y2, acc2);
				acc3 = simd.mul_add_f32s(*x3, *y3, acc3);
			}

			for (x0, y0) in iter::zip(xs_1, ys_1) {
				acc0 = simd.mul_add_f32s(*x0, *y0, acc0);
			}

			// reduce_sum_f32s
			// f32x8 -> f32x4 + f32x4
			// f32x4 -> f32x2 + f32x2
			// f32x2 -> f32 + f32
			acc0 = simd.add_f32s(acc0, acc1);
			acc2 = simd.add_f32s(acc2, acc3);

			acc0 = simd.add_f32s(acc0, acc2);

			if !x1.is_empty() {
				acc0 =
					simd.mul_add_f32s(simd.partial_load_f32s(x1), simd.partial_load_f32s(y1), acc0);
			}

			simd.reduce_sum_f32s(acc0)
		}
	}

	simd.vectorize(Impl { simd, x, y })
}

pub fn bench_dot_simd_extract_reduce_ilp_fma_aligned_runtime_dispatch(
	bencher: Bencher,
	PlotArg(n): PlotArg,
) {
	let x = &*avec![1.0_f32; n];
	let y = &*avec![1.0_f32; n];

	let arch = Arch::new();

	struct Impl<'a> {
		x: &'a [f32],
		y: &'a [f32],
	}

	impl pulp::WithSimd for Impl<'_> {
		type Output = f32;

		#[inline(always)]
		fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
			let Self { x, y } = self;
			dot_product_simd_extract_reduce_ilp_fma_generic(simd, x, y)
		}
	}

	bencher.bench(|| arch.dispatch(Impl { x, y }));
}

pub fn bench_dot_simd_extract_reduce_ilp_fma_epilogue_aligned_runtime_dispatch(
	bencher: Bencher,
	PlotArg(n): PlotArg,
) {
	let x = &*avec![1.0_f32; n];
	let y = &*avec![1.0_f32; n];

	let arch = Arch::new();

	struct Impl<'a> {
		x: &'a [f32],
		y: &'a [f32],
	}

	impl pulp::WithSimd for Impl<'_> {
		type Output = f32;

		#[inline(always)]
		fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
			let Self { x, y } = self;
			dot_product_simd_extract_reduce_ilp_fma_epilogue_generic(simd, x, y)
		}
	}

	bencher.bench(|| arch.dispatch(Impl { x, y }));
}
fn main() -> std::io::Result<()> {
	let mut bench = Bench::new(BenchConfig::from_args()?);

	let mut params = vec![];
	for i in 1..=16 {
		params.push(i);
	}
	for i in 2..=16 {
		params.push(16 * i);
	}
	for i in 2..=16 {
		params.push(256 * i);
	}

	#[cfg(target_arch = "x86_64")]
	bench.register_many(
		list![
			bench_dot_scalar,
			x86::bench_dot_simd,
			x86::bench_dot_simd_extract_reduce,
			x86::bench_dot_simd_extract_reduce_ilp,
			x86::bench_dot_simd_extract_reduce_ilp_fma,
			x86::bench_dot_simd_extract_reduce_ilp_fma_misaligned,
			x86::bench_dot_simd_extract_reduce_ilp_fma_aligned,
			bench_dot_simd_extract_reduce_ilp_fma_aligned_runtime_dispatch,
			bench_dot_simd_extract_reduce_ilp_fma_epilogue_aligned_runtime_dispatch,
		],
		params.iter().copied().map(PlotArg),
	);

	#[cfg(not(target_arch = "x86_64"))]
	bench.register_many(
		list![
			bench_dot_scalar,
			bench_dot_simd_extract_reduce_ilp_fma_aligned_runtime_dispatch,
			bench_dot_simd_extract_reduce_ilp_fma_epilogue_aligned_runtime_dispatch,
		],
		params.iter().copied().map(PlotArg),
	);

	bench.run()?;
	Ok(())
}
