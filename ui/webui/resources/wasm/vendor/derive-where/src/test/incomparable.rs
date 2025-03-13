use quote::quote;
use syn::Result;

use super::test_derive;

#[test]
fn variants() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(PartialEq, PartialOrd)]
			enum Test{
				A,
				#[derive_where(incomparable)]
				B(Box<dyn Fn()>),
				C(String),
				#[derive_where(incomparable)]
				D,
				#[derive_where(incomparable)]
				E { test: u8 }
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialEq for Test {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					if ::core::mem::discriminant(self) == ::core::mem::discriminant(__other) {
						match (self, __other) {
							(Test::C(ref __field_0), Test::C(ref __other_field_0)) => true && ::core::cmp::PartialEq::eq(__field_0, __other_field_0),
							(Test::B(..) | Test::D | Test::E {..}, ..) => false,
							_ => true,
						}
					} else {
						false
					}
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::B(..) | Test::D | Test::E { .. })
						|| ::core::matches!(__other, Test::B(..) | Test::D | Test::E { .. })
					{
						return ::core::option::Option::None;
					}
					let __self_disc = ::core::mem::discriminant(self);
					let __other_disc = ::core::mem::discriminant(__other);
					if __self_disc == __other_disc {
						match (self, __other) {
							(Test::C(ref __field_0), Test::C(ref __other_field_0)) =>
								match ::core::cmp::PartialOrd::partial_cmp(__field_0, __other_field_0) {
									::core::option::Option::Some(::core::cmp::Ordering::Equal) =>
										::core::option::Option::Some(::core::cmp::Ordering::Equal),
									__cmp => __cmp,
								},
							_ => ::core::option::Option::Some(::core::cmp::Ordering::Equal),
						}
					} else {
						const fn __discriminant(__this: &Test) -> isize {
							match __this {
								Test::A => 0,
								Test::B(ref __field_0) => 1,
								Test::C(ref __field_0) => 2,
								Test::D => 3,
								Test::E { test: ref __field_test } => 4
							}
						}

						::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
					}
				}
			}
		},
	)
}

#[test]
fn enum_empty_and_empty_incomparable_variants() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(PartialEq, PartialOrd)]
			enum Test{
				#[derive_where(incomparable)]
				A,
				B
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialEq for Test {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					if ::core::mem::discriminant(self) == ::core::mem::discriminant(__other) {
						if ::core::matches!(self, Test::A) {
							return false;
						}
						true
					} else {
						false
					}
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::A) || ::core::matches!(__other, Test::A) {
						::core::option::Option::None
					} else {
						::core::option::Option::Some(::core::cmp::Ordering::Equal)
					}
				}
			}
		},
	)
}

#[test]
fn enum_empty_and_non_empty_incomparable_variants() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(PartialEq, PartialOrd)]
			enum Test{
				#[derive_where(incomparable)]
				A(String),
				B
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialEq for Test {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					if ::core::mem::discriminant(self) == ::core::mem::discriminant(__other) {
						match (self , __other) {
							(Test::A (..) , ..) => false,
							_ => true,
						}
					} else {
						false
					}
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::A(..)) || ::core::matches!(__other, Test::A(..)) {
						::core::option::Option::None
					} else {
						::core::option::Option::Some(::core::cmp::Ordering::Equal)
					}
				}
			}
		},
	)
}

#[test]
fn enum_empty_and_multiple_incomparable_variants() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(PartialEq, PartialOrd)]
			enum Test{
				#[derive_where(incomparable)]
				A,
				B,
				#[derive_where(incomparable)]
				C(String)
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialEq for Test {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					if ::core::mem::discriminant(self) == ::core::mem::discriminant(__other) {
						match (self, __other) {
							(Test::A | Test::C(..), ..) => false,
							_ => true,
						}
					} else {
						false
					}
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::A | Test::C(..))
						|| ::core::matches!(__other, Test::A | Test::C(..))
					{
						::core::option::Option::None
					} else {
						::core::option::Option::Some(::core::cmp::Ordering::Equal)
					}
				}
			}
		},
	)
}

#[test]
fn enum_skipped_and_incomparable_variant() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(PartialEq, PartialOrd)]
			enum Test{
				#[derive_where(incomparable)]
				A,
				#[derive_where(skip_inner)]
				B(String),
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialEq for Test {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					if ::core::mem::discriminant(self) == ::core::mem::discriminant(__other) {
						if ::core::matches!(self, Test::A) {
							return false;
						}
						true
					} else {
						false
					}
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::A) || ::core::matches!(__other, Test::A) {
						::core::option::Option::None
					} else {
						::core::option::Option::Some(::core::cmp::Ordering::Equal)
					}
				}
			}
		},
	)
}

#[test]
fn enum_non_empty_and_incomparable_variant() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(PartialEq, PartialOrd)]
			enum Test{
				#[derive_where(incomparable)]
				A,
				B(String),
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialEq for Test {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					if ::core::mem::discriminant(self) == ::core::mem::discriminant(__other) {
						match (self, __other) {
							(Test::B(ref __field_0), Test::B(ref __other_field_0)) =>
								true && ::core::cmp::PartialEq::eq(__field_0, __other_field_0),
							(Test::A, ..) => false,
							_ => unsafe { ::core::hint::unreachable_unchecked() },
						}
					} else {
						false
					}
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::A) || ::core::matches!(__other, Test::A) {
						::core::option::Option::None
					} else {
						match (self, __other) {
							(Test::B(ref __field_0), Test::B(ref __other_field_0)) =>
								match ::core::cmp::PartialOrd::partial_cmp(__field_0, __other_field_0) {
									::core::option::Option::Some(::core::cmp::Ordering::Equal) =>
										::core::option::Option::Some(::core::cmp::Ordering::Equal),
									__cmp => __cmp,
								},
							_ => unsafe { ::core::hint::unreachable_unchecked() },
						}
					}
				}
			}
		},
	)
}

#[test]
fn enum_incomparable_and_skipped_variant() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(PartialEq, PartialOrd)]
			enum Test{
				#[derive_where(incomparable, skip_inner)]
				A(String),
				B
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialEq for Test {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					if ::core::mem::discriminant(self) == ::core::mem::discriminant(__other) {
						if ::core::matches!(self, Test::A(..)) {
							return false;
						}
						true
					} else {
						false
					}
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::A(..)) || ::core::matches!(__other, Test::A(..)) {
						::core::option::Option::None
					} else {
						::core::option::Option::Some(::core::cmp::Ordering::Equal)
					}
				}
			}
		},
	)
}

#[test]
fn items() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(PartialEq, PartialOrd)]
			#[derive_where(incomparable)]
			enum Test{}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialEq for Test {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					false
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					::core::option::Option::None
				}
			}
		},
	)?;
	test_derive(
		quote! {
			#[derive_where(PartialEq, PartialOrd)]
			#[derive_where(incomparable)]
			struct Test;
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialEq for Test {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					false
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					::core::option::Option::None
				}
			}
		},
	)
}
