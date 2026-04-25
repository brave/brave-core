use criterion::{black_box, criterion_group, criterion_main, Criterion};
use dyn_stack::{DynStack, GlobalMemBuffer, GlobalPodBuffer, PodStack, ReborrowMut, StackReq};

pub fn criterion_benchmark(c: &mut Criterion) {
    c.bench_function("memalloc", |b| {
        b.iter(|| {
            black_box(GlobalMemBuffer::new(StackReq::new_aligned::<i32>(
                1 << 20,
                16,
            )))
        })
    });
    c.bench_function("memalloc-zeroed", |b| {
        b.iter(|| {
            black_box(GlobalPodBuffer::new(StackReq::new_aligned::<i32>(
                1 << 20,
                16,
            )))
        })
    });

    for n in [32, 64, 1024, 16384] {
        let align = 64;
        let single_scratch = StackReq::new_aligned::<f32>(n, align);
        let scratch = single_scratch.and(single_scratch);

        let mut mem = GlobalMemBuffer::new(scratch);
        let mut stack = DynStack::new(&mut *mem);

        {
            let (src, stack) = stack.rb_mut().make_aligned_with(n, align, |_| 0.0_f32);
            let (mut dst, _) = stack.make_aligned_with(n, align, |_| 0.0_f32);
            c.bench_function(&format!("preallocated-{}", n), |b| {
                b.iter(|| {
                    for (d, s) in dst.iter_mut().zip(src.iter()) {
                        *d = s + s;
                    }
                    black_box(&mut dst);
                })
            });
        }

        {
            let (src, mut stack) = stack.rb_mut().make_aligned_with(n, align, |_| 0.0_f32);
            c.bench_function(&format!("allocate-on-demand-init-{}", n), |b| {
                b.iter(|| {
                    let (mut dst, _) = stack.rb_mut().make_aligned_with(n, align, |_| 0.0_f32);
                    for (d, s) in dst.iter_mut().zip(src.iter()) {
                        *d = s + s;
                    }
                    black_box(&mut dst);
                })
            });
        }

        {
            let (src, mut stack) = stack.rb_mut().make_aligned_with(n, align, |_| 0.0_f32);
            c.bench_function(&format!("allocate-on-demand-uninit-{}", n), |b| {
                b.iter(|| {
                    let (mut dst, _) = stack.rb_mut().make_aligned_uninit::<f32>(n, align);
                    for (d, s) in dst.iter_mut().zip(src.iter()) {
                        d.write(s + s);
                    }
                    black_box(&mut dst);
                })
            });
        }

        {
            let (src, mut stack) = stack.rb_mut().make_aligned_with(n, align, |_| 0.0_f32);
            c.bench_function(&format!("allocate-on-demand-collect-{}", n), |b| {
                b.iter(|| {
                    let (mut dst, _) = stack
                        .rb_mut()
                        .collect_aligned(align, src.iter().zip(src.iter()).map(|(a, b)| a + b));
                    black_box(&mut dst);
                })
            });
        }
    }

    for n in [32, 64, 1024, 16384] {
        let align = 64;
        let single_scratch = StackReq::new_aligned::<f32>(n, align);
        let scratch = single_scratch.and(single_scratch);

        let mut mem = GlobalPodBuffer::new(scratch);
        let mut stack = PodStack::new(&mut *mem);

        {
            let (src, stack) = stack.rb_mut().make_aligned_with(n, align, |_| 0.0_f32);
            let (mut dst, _) = stack.make_aligned_with(n, align, |_| 0.0_f32);
            c.bench_function(&format!("pod-preallocated-{}", n), |b| {
                b.iter(|| {
                    for (d, s) in dst.iter_mut().zip(src.iter()) {
                        *d = s + s;
                    }
                    black_box(&mut dst);
                })
            });
        }

        {
            let (src, mut stack) = stack.rb_mut().make_aligned_with(n, align, |_| 0.0_f32);
            c.bench_function(&format!("pod-allocate-on-demand-init-{}", n), |b| {
                b.iter(|| {
                    let (mut dst, _) = stack.rb_mut().make_aligned_with(n, align, |_| 0.0_f32);
                    for (d, s) in dst.iter_mut().zip(src.iter()) {
                        *d = s + s;
                    }
                    black_box(&mut dst);
                })
            });
        }

        {
            let (src, mut stack) = stack.rb_mut().make_aligned_with(n, align, |_| 0.0_f32);
            c.bench_function(&format!("pod-allocate-on-demand-uninit-{}", n), |b| {
                b.iter(|| {
                    let (mut dst, _) = stack.rb_mut().make_aligned_raw::<f32>(n, align);
                    for (d, s) in dst.iter_mut().zip(src.iter()) {
                        *d = s + s;
                    }
                    black_box(&mut dst);
                })
            });
        }

        {
            let (src, mut stack) = stack.rb_mut().make_aligned_with(n, align, |_| 0.0_f32);
            c.bench_function(&format!("pod-allocate-on-demand-collect-{}", n), |b| {
                b.iter(|| {
                    let (mut dst, _) = stack
                        .rb_mut()
                        .collect_aligned(align, src.iter().zip(src.iter()).map(|(a, b)| a + b));
                    black_box(&mut dst);
                })
            });
        }
    }
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);
