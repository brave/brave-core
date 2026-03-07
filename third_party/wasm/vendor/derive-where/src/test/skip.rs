use quote::quote;
use syn::Result;

use super::test_derive;

#[test]
fn struct_inner() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Debug)]
			#[derive_where(skip_inner)]
			struct Test<T>(std::marker::PhantomData<T>);
		},
		quote! {
			#[automatically_derived]
			impl<T> ::core::fmt::Debug for Test<T> {
				fn fmt(&self, __f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
					match self {
						Test(ref __field_0) => {
							let mut __builder = ::core::fmt::Formatter::debug_tuple(__f, "Test");
							::core::fmt::DebugTuple::finish(&mut __builder)
						}
					}
				}
			}
		},
	)
}

#[test]
fn enum_inner() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Debug)]
			enum Test<T> {
				#[derive_where(skip_inner)]
				A(std::marker::PhantomData<T>),
			}
		},
		quote! {
			#[automatically_derived]
			impl<T> ::core::fmt::Debug for Test<T> {
				fn fmt(&self, __f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
					match self {
						Test::A(ref __field_0) => {
							let mut __builder = ::core::fmt::Formatter::debug_tuple(__f, "A");
							::core::fmt::DebugTuple::finish(&mut __builder)
						}
					}
				}
			}
		},
	)
}

#[test]
fn struct_empty() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Ord)]
			#[derive_where(skip_inner)]
			struct Test<T>(std::marker::PhantomData<T>);
		},
		quote! {
			#[automatically_derived]
			impl<T> ::core::cmp::Ord for Test<T> {
				#[inline]
				fn cmp(&self, __other: &Self) -> ::core::cmp::Ordering {
					::core::cmp::Ordering::Equal
				}
			}
		},
	)
}

#[test]
fn variant_empty() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Ord)]
			enum Test<T> {
				#[derive_where(skip_inner)]
				A(std::marker::PhantomData<T>),
			}
		},
		quote! {
			#[automatically_derived]
			impl<T> ::core::cmp::Ord for Test<T> {
				#[inline]
				fn cmp(&self, __other: &Self) -> ::core::cmp::Ordering {
					::core::cmp::Ordering::Equal
				}
			}
		},
	)
}

#[test]
fn variants_empty() -> Result<()> {
	#[cfg(feature = "nightly")]
	let ord = quote! {
		::core::cmp::Ord::cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> isize {
			match __this {
				Test::A(ref __field_0) => 0,
				Test::B(ref __field_0) => 1
			}
		}

		::core::cmp::Ord::cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(Ord)]
			enum Test<T> {
				#[derive_where(skip_inner)]
				A(std::marker::PhantomData<T>),
				#[derive_where(skip_inner)]
				B(std::marker::PhantomData<T>),
			}
		},
		quote! {
			#[automatically_derived]
			impl<T> ::core::cmp::Ord for Test<T> {
				#[inline]
				fn cmp(&self, __other: &Self) -> ::core::cmp::Ordering {
					#ord
				}
			}
		},
	)
}

#[test]
fn variants_partly_empty() -> Result<()> {
	#[cfg(feature = "nightly")]
	let discriminant = quote! {
		let __self_disc = ::core::intrinsics::discriminant_value(self);
		let __other_disc = ::core::intrinsics::discriminant_value(__other);
	};
	#[cfg(not(feature = "nightly"))]
	let discriminant = quote! {
		let __self_disc = ::core::mem::discriminant(self);
		let __other_disc = ::core::mem::discriminant(__other);
	};
	#[cfg(feature = "nightly")]
	let ord = quote! {
		::core::cmp::Ord::cmp(&__self_disc, &__other_disc)
	};
	#[cfg(not(feature = "nightly"))]
	let ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> isize {
			match __this {
				Test::A(ref __field_0) => 0,
				Test::B(ref __field_0, ref __field_1) => 1
			}
		}

		::core::cmp::Ord::cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(Ord)]
			enum Test<T> {
				#[derive_where(skip_inner)]
				A(std::marker::PhantomData<T>),
				B(#[derive_where(skip)] std::marker::PhantomData<T>, std::marker::PhantomData<T>),
			}
		},
		quote! {
			#[automatically_derived]
			impl<T> ::core::cmp::Ord for Test<T> {
				#[inline]
				fn cmp(&self, __other: &Self) -> ::core::cmp::Ordering {
					#discriminant

					if __self_disc == __other_disc {
						match (self , __other) {
							(Test::B(ref __field_0, ref __field_1), Test::B(ref __other_field_0, ref __other_field_1)) =>
								match ::core::cmp::Ord::cmp(__field_1 ,__other_field_1) {
									::core::cmp::Ordering::Equal => ::core::cmp::Ordering::Equal, __cmp => __cmp,
								},
							_ => ::core::cmp::Ordering::Equal,
						}
					} else {
						#ord
					}
				}
			}
		},
	)
}
