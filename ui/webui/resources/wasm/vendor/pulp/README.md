`pulp` is a safe abstraction over SIMD instructions, that allows you to write a function once
and dispatch to equivalent vectorized versions based on the features detected at runtime.

[![Documentation](https://docs.rs/pulp/badge.svg)](https://docs.rs/pulp)
[![Crate](https://img.shields.io/crates/v/pulp.svg)](https://crates.io/crates/pulp)

# Autovectorization example

```rust
use pulp::Arch;
fn main(){
    let mut v = (0..1000).map(|i| i as f64).collect::<Vec<_>>();
    let arch = Arch::new();

    arch.dispatch(|| {
        for x in &mut v {
            *x *= 2.0;
        }
    });

    for (i, x) in v.into_iter().enumerate() {
        assert_eq!(x, 2.0 * i as f64);
    }
}
```

# Manual vectorization example

```rust
use pulp::{Arch, Simd, WithSimd};

struct TimesThree<'a>(&'a mut [f64]);
impl<'a> WithSimd for TimesThree<'a> {
    // No output, the input is modified in place to save time allocating a new vector
    type Output = ();

    #[inline(always)]
    fn with_simd<S: Simd>(self, simd: S) -> Self::Output {
        let v = self.0;
        // the tail is the remainder left after allocating v into simd vectors
        // len(tail) = len(v) % simd_vector_length
        let (head, tail) = S::as_mut_simd_f64s(v);

        // fill the simd vectors with 3.0
        let three = simd.splat_f64s(3.0);

        for x in head {
            *x = simd.mul_f64s(three, *x);
        }
        // the tail is not loaded into simd vectors hence non-simd operations are used
        for x in tail {
            *x = *x * 3.0;
        }
    }
}
fn main() {
    let mut v = (0..1000).map(|i| i as f64).collect::<Vec<_>>();
    let arch = Arch::new();
    arch.dispatch(TimesThree(&mut v)); // dynamically dispatch the function to the correct simd implementation
    for (i, x) in v.into_iter().enumerate() {
        assert_eq!(x, 3.0 * i as f64);
    }
}
```

# Less boilerplate using `pulp::with_simd`

Only available with the `macro` feature.

Requires the first non-lifetime generic parameter, as well as the function's
first input parameter to be the SIMD type.

```rust
use pulp::Simd;

// the macro creates a `sum` function
#[pulp::with_simd(sum = pulp::Arch::new())]
#[inline(always)]
fn sum_with_simd<'a, S: Simd>(simd: S, v: &'a mut [f64]) {
    let (head, tail) = S::as_mut_simd_f64s(v);

    // fill the simd vectors with 3.0
    let three = simd.splat_f64s(3.0);

    for x in head {
        *x = simd.mul_f64s(three, *x);
    }

    for x in tail {
        *x = *x * 3.0;
    }
}
fn main() {
    let mut v = (0..1000).map(|i| i as f64).collect::<Vec<_>>();
    sum(&mut v);

    for (i, x) in v.into_iter().enumerate() {
        assert_eq!(x, 3.0 * i as f64);
    }
}
```
