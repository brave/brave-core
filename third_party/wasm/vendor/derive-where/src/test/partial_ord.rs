use quote::quote;
use syn::Result;

use super::test_derive;

#[test]
fn struct_() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			struct Test<T> {
				field: std::marker::PhantomData<T>,
			}
		},
		quote! {
			#[automatically_derived]
			impl<T> ::core::cmp::PartialOrd for Test<T> {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					match (self, __other) {
						(Test { field: ref __field_field }, Test { field: ref __other_field_field }) =>
							match ::core::cmp::PartialOrd::partial_cmp(__field_field, __other_field_field) {
								::core::option::Option::Some(::core::cmp::Ordering::Equal) => ::core::option::Option::Some(::core::cmp::Ordering::Equal),
								__cmp => __cmp,
							},
					}
				}
			}
		},
	)
}

#[test]
fn tuple() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			struct Test<T>(std::marker::PhantomData<T>);
		},
		quote! {
			#[automatically_derived]
			impl<T> ::core::cmp::PartialOrd for Test<T> {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					match (self, __other) {
						(Test(ref __field_0), Test(ref __other_field_0)) =>
							match ::core::cmp::PartialOrd::partial_cmp(__field_0, __other_field_0) {
								::core::option::Option::Some(::core::cmp::Ordering::Equal) => ::core::option::Option::Some(::core::cmp::Ordering::Equal),
								__cmp => __cmp,
							},
					}
				}
			}
		},
	)
}

#[test]
fn enum_() -> Result<()> {
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
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(&__self_disc, &__other_disc)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> isize {
			match __this {
				Test::A { field: ref __field_field } => 0,
				Test::B { } => 1,
				Test::C(ref __field_0) => 2,
				Test::D() => 3,
				Test::E => 4
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			enum Test<T> {
				A { field: std::marker::PhantomData<T>},
				B { },
				C(std::marker::PhantomData<T>),
				D(),
				E,
			}
		},
		quote! {
			#[automatically_derived]
			impl<T> ::core::cmp::PartialOrd for Test<T> {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					#discriminant

					if __self_disc == __other_disc {
						match (self, __other) {
							(Test::A { field: ref __field_field }, Test::A { field: ref __other_field_field }) =>
								match ::core::cmp::PartialOrd::partial_cmp(__field_field, __other_field_field) {
									::core::option::Option::Some(::core::cmp::Ordering::Equal) => ::core::option::Option::Some(::core::cmp::Ordering::Equal),
									__cmp => __cmp,
								},
							(Test::C(ref __field_0), Test::C(ref __other_field_0)) =>
								match ::core::cmp::PartialOrd::partial_cmp(__field_0, __other_field_0) {
									::core::option::Option::Some(::core::cmp::Ordering::Equal) => ::core::option::Option::Some(::core::cmp::Ordering::Equal),
									__cmp => __cmp,
								},
							_ => ::core::option::Option::Some(::core::cmp::Ordering::Equal),
						}
					} else {
						#partial_ord
					}
				}
			}
		},
	)
}

#[test]
fn union_() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Clone, Copy)]
			union Test<T> {
				a: std::marker::PhantomData<T>,
				b: u8,
			}
		},
		quote! {
			#[automatically_derived]
			impl<T> ::core::clone::Clone for Test<T> {
				#[inline]
				fn clone(&self) -> Self {
					*self
				}
			}

			#[automatically_derived]
			impl<T> ::core::marker::Copy for Test<T>
			{ }
		},
	)
}

#[test]
fn bound() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Ord; T)]
			#[derive_where(PartialOrd)]
			struct Test<T, U>(T, std::marker::PhantomData<U>);
		},
		quote! {
			#[automatically_derived]
			impl<T, U> ::core::cmp::Ord for Test<T, U>
			where T: ::core::cmp::Ord
			{
				#[inline]
				fn cmp(&self, __other: &Self) -> ::core::cmp::Ordering {
					match (self, __other) {
						(Test(ref __field_0, ref __field_1), Test(ref __other_field_0, ref __other_field_1)) =>
							match ::core::cmp::Ord::cmp(__field_0, __other_field_0) {
								::core::cmp::Ordering::Equal => match ::core::cmp::Ord::cmp(__field_1, __other_field_1) {
									::core::cmp::Ordering::Equal => ::core::cmp::Ordering::Equal,
									__cmp => __cmp,
								},
								__cmp => __cmp,
							},
					}
				}
			}

			#[automatically_derived]
			impl<T, U> ::core::cmp::PartialOrd for Test<T, U> {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					match (self, __other) {
						(Test(ref __field_0, ref __field_1), Test(ref __other_field_0, ref __other_field_1)) =>
							match ::core::cmp::PartialOrd::partial_cmp(__field_0, __other_field_0) {
								::core::option::Option::Some(::core::cmp::Ordering::Equal) => match ::core::cmp::PartialOrd::partial_cmp(__field_1, __other_field_1) {
									::core::option::Option::Some(::core::cmp::Ordering::Equal) => ::core::option::Option::Some(::core::cmp::Ordering::Equal),
									__cmp => __cmp,
								},
								__cmp => __cmp,
							},
					}
				}
			}
		},
	)
}
