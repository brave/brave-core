# A more convenient `#[target_feature]` replacement

To get good performance out of SIMD everything on the SIMD codepath must be inlined.
With how SIMD is currently implemented in Rust one of two things have to be true for
a function using SIMD to be inlinable: (and this includes the SIMD intrinsics themselves)

   a) The whole program has to be compiled with the relevant `-C target-cpu` or `-C target-feature` flags.

   b) SIMD support must be automatically detected at runtime, and every function on the SIMD codepath must be marked with `#[target_feature]`.

Both have their downsides. Setting the `target-cpu` or `target-features` makes the resulting binary
incompatible with older CPUs, while using `#[target_feature]` is incredibly inconvenient.

This crate is meant to make `#[target_feature]` less painful to use.

## Problems with `#[target_feature]`

When we're not compiling with the relevant `target-cpu`/`target-feature` flags everything on
the SIMD codepath must be marked with the `#[target_feature]` attribute. This is not a problem
when all of your SIMD code is neatly encapsulated inside of a single function, but once you start
to build out more elaborate abstractions it starts to become painful to use.

  * It can only be used on `unsafe` functions, so everything on your SIMD codepath now has to be `unsafe`.

    In theory this is nice - these functions require the relevant SIMD instructions to be present at runtime,
    so calling them without checking is obviously unsafe! But in practice this is rarely what you want. When
    you build an abstraction over SIMD code you usually want to assume that *internally* within your module
    all of the necessary SIMD instructions are available, and you only want to check this at the boundaries
    when you're first entering your module. You do *not* want to infect everything *inside* of the module with
    `unsafe` since you've already checked this invariant at the module's API boundary.

  * It cannot be used on non-`unsafe` trait methods.

    If you're implementing a trait, say for example `std::ops::Add`, then you cannot mark the method `unsafe`
    unless the original trait also has it marked as `unsafe`, and usually it doesn't.

  * It makes it impossible to abstract over a given SIMD instruction set using a trait.

    For example, let's assume you want to abstract over which SIMD instructions you use using a trait in the following way:

    ```rust
    trait Backend {
        unsafe fn sum(input: &[u32]) -> u32;
    }

    struct AVX;
    # #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    impl Backend for AVX {
        #[target_feature(enable = "avx")]
        unsafe fn sum(xs: &[u32]) -> u32 {
            // ...
            todo!();
        }
    }

    struct AVX2;
    # #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    impl Backend for AVX2 {
        #[target_feature(enable = "avx2")]
        unsafe fn sum(xs: &[u32]) -> u32 {
            // ...
            todo!();
        }
    }

    // And now you want a have function which calls into that trait:
    unsafe fn do_calculations<B>(xs: &[u32]) -> u32 where B: Backend {
        let value = B::sum(xs);
        // ...do some more calculations here...
        value
    }
    ```

    We have a problem here. This has to be marked with `#[target_feature]`, and that has to specify the concrete
    feature flag for a given SIMD instruction set, but this function is generic so we can't do that!

## How does this crate make it better?

### You can now mark safe functions with `#[target_feature]`

This crate exposes an `#[unsafe_target_feature]` macro which works just like `#[target_feature]` except
it moves the `unsafe` from the function prototype into the macro name, and can be used on safe functions.

```rust,compile_fail
// ERROR: `#[target_feature(..)]` can only be applied to `unsafe` functions
#[target_feature(enable = "avx2")]
fn func() {}
```

```rust
// It works, but must be `unsafe`
# #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[target_feature(enable = "avx2")]
unsafe fn func() {}
```

```rust
use curve25519_dalek_derive::unsafe_target_feature;

// No `unsafe` on the function itself!
# #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[unsafe_target_feature("avx2")]
fn func() {}
```

It can also be used to mark functions inside of impls:

```rust,compile_fail
struct S;

impl core::ops::Add for S {
    type Output = S;
    // ERROR: method `add` has an incompatible type for trait
    #[target_feature(enable = "avx2")]
    unsafe fn add(self, rhs: S) -> S {
        S
    }
}
```

```rust
use curve25519_dalek_derive::unsafe_target_feature;

struct S;

# #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[unsafe_target_feature("avx2")]
impl core::ops::Add for S {
    type Output = S;
    // No `unsafe` on the function itself!
    fn add(self, rhs: S) -> S {
        S
    }
}

```

### You can generate specialized copies of a module for each target feature

```rust
use curve25519_dalek_derive::unsafe_target_feature_specialize;

# #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#[unsafe_target_feature_specialize("sse2", "avx2", conditional("avx512ifma", nightly))]
mod simd {
    #[for_target_feature("sse2")]
    pub const CONSTANT: u32 = 1;

    #[for_target_feature("avx2")]
    pub const CONSTANT: u32 = 2;

    #[for_target_feature("avx512ifma")]
    pub const CONSTANT: u32 = 3;

    pub fn func() { /* ... */ }
}

# #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
fn entry_point() {
    #[cfg(nightly)]
    if std::is_x86_feature_detected!("avx512ifma") {
        return simd_avx512ifma::func();
    }

    if std::is_x86_feature_detected!("avx2") {
        return simd_avx2::func();
    }

    if std::is_x86_feature_detected!("sse2") {
        return simd_sse2::func();
    }

    unimplemented!();
}
```

## How to use `#[unsafe_target_feature]`?

  - Can be used on `fn`s, `impl`s and `mod`s.
  - When used on a function will only apply to that function; it won't apply to any nested functions, traits, mods, etc.
  - When used on an `impl` will only apply to all of the functions directly defined inside of that `impl`.
  - When used on a `mod` will only apply to all of the `fn`s and `impl`s directly defined inside of that `mod`.
  - Cannot be used on methods which use `self` or `Self`; instead use it on the `impl` in which the method is defined.

## License

Licensed under either of

  * Apache License, Version 2.0, [LICENSE-APACHE](LICENSE-APACHE)
  * MIT license ([LICENSE-MIT](LICENSE-MIT))

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall be
dual licensed as above, without any additional terms or conditions.
