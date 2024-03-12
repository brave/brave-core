#![cfg(any(target_arch = "x86", target_arch = "x86_64"))]
#![allow(dead_code)]
#![allow(unused_imports)]

use curve25519_dalek_derive::{unsafe_target_feature, unsafe_target_feature_specialize};

#[unsafe_target_feature("sse2")]
/// A doc comment.
fn function(a: u32, b: u32) -> u32 {
    a - b
}

#[unsafe_target_feature("sse2")]
fn function_with_const_arg<const N: u32>(b: u32) -> u32 {
    N - b
}

#[unsafe_target_feature("sse2")]
fn function_with_where_clause<T>(a: T, b: T) -> T::Output
where
    T: Copy + core::ops::Sub,
{
    a - b
}

#[unsafe_target_feature("sse2")]
#[cfg(feature = "dummy")]
fn function_with_cfg() {}

#[unsafe_target_feature("sse2")]
#[rustfmt::skip]
fn function_with_rustfmt_skip() {}

struct Struct {
    a: u32,
}

#[unsafe_target_feature("sse2")]
impl Struct {
    #[allow(unused_mut)]
    fn member_function(&self, mut b: u32) -> u32 {
        self.a - b
    }

    fn member_function_with_const_arg<const N: u32>(self) -> u32 {
        self.a - N
    }

    #[cfg(feature = "dummy")]
    fn member_function_with_cfg() {}
}

struct StructWithGenerics<T>
where
    T: Copy + core::ops::Sub,
{
    a: T,
}

#[unsafe_target_feature("sse2")]
impl<T> StructWithGenerics<T>
where
    T: Copy + core::ops::Sub,
{
    #[inline]
    fn member_function(&self, b: T) -> T::Output {
        self.a - b
    }
}

struct StructWithGenericsNoWhere<T: Copy + core::ops::Sub> {
    a: T,
}

#[unsafe_target_feature("sse2")]
impl<T: Copy + core::ops::Sub> StructWithGenericsNoWhere<T> {
    #[inline(always)]
    fn member_function(&self, b: T) -> T::Output {
        self.a - b
    }
}

#[unsafe_target_feature("sse2")]
#[allow(dead_code)]
impl<'a> From<&'a Struct> for () {
    fn from(_: &'a Struct) -> Self {}
}

#[unsafe_target_feature("sse2")]
mod inner {
    fn inner_function(a: u32, b: u32) -> u32 {
        a - b
    }
}

#[unsafe_target_feature_specialize("sse2", "avx2", conditional("avx512ifma", disabled))]
mod inner_spec {
    #[for_target_feature("sse2")]
    const CONST: u32 = 1;

    #[for_target_feature("avx2")]
    const CONST: u32 = 2;

    pub fn spec_function(a: u32, b: u32) -> u32 {
        a - b - CONST
    }

    #[for_target_feature("sse2")]
    const IS_AVX2: bool = false;

    #[for_target_feature("avx2")]
    const IS_AVX2: bool = true;

    #[test]
    fn test_specialized() {
        assert!(!IS_AVX2);
    }

    #[cfg(test)]
    mod tests {
        #[test]
        fn test_specialized_inner() {
            assert!(!super::IS_AVX2);
        }
    }
}

#[unsafe_target_feature("sse2")]
#[test]
fn test_sse2_only() {}

#[unsafe_target_feature("avx2")]
#[test]
fn test_avx2_only() {
    compile_error!();
}

#[test]
fn test_function() {
    assert_eq!(function(10, 3), 7);
    assert_eq!(function_with_where_clause(10, 3), 7);
    assert_eq!(function_with_const_arg::<10>(3), 7);
    assert_eq!(Struct { a: 10 }.member_function(3), 7);
    assert_eq!(StructWithGenerics { a: 10 }.member_function(3), 7);
    assert_eq!(StructWithGenericsNoWhere { a: 10 }.member_function(3), 7);
    assert_eq!(inner_spec_sse2::spec_function(10, 3), 6);
    assert_eq!(inner_spec_avx2::spec_function(10, 3), 5);
}
