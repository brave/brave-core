use quote::quote;
use syn::Result;

use super::test_derive;

#[test]
fn struct_() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Clone, Copy, Debug, Default, Eq, Hash, Ord, PartialEq, PartialOrd)]
			struct Test<T> {
				field: std::marker::PhantomData<T>,
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

			#[automatically_derived]
			impl<T> ::core::fmt::Debug for Test<T> {
				fn fmt(&self, __f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
					match self {
						Test { field: ref __field_field } => {
							let mut __builder = ::core::fmt::Formatter::debug_struct(__f, "Test");
							::core::fmt::DebugStruct::field(&mut __builder, "field", __field_field);
							::core::fmt::DebugStruct::finish(&mut __builder)
						}
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::default::Default for Test<T> {
				fn default() -> Self {
					Test { field: ::core::default::Default::default() }
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::Eq for Test<T> {
				#[inline]
				fn assert_receiver_is_total_eq(&self) {
					struct __AssertEq<__T: ::core::cmp::Eq + ?::core::marker::Sized>(::core::marker::PhantomData<__T>);

					// For some reason the comparison fails without the extra space at the end.
					let _: __AssertEq<std::marker::PhantomData<T> >;
				}
			}

			#[automatically_derived]
			impl<T> ::core::hash::Hash for Test<T> {
				fn hash<__H: ::core::hash::Hasher>(&self, __state: &mut __H) {
					match self {
						Test { field: ref __field_field } => { ::core::hash::Hash::hash(__field_field, __state); }
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::Ord for Test<T> {
				#[inline]
				fn cmp(&self, __other: &Self) -> ::core::cmp::Ordering {
					match (self, __other) {
						(Test { field: ref __field_field }, Test { field: ref __other_field_field }) =>
							match ::core::cmp::Ord::cmp(__field_field, __other_field_field) {
								::core::cmp::Ordering::Equal => ::core::cmp::Ordering::Equal,
								__cmp => __cmp,
							},
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::PartialEq for Test<T> {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					match (self, __other) {
						(Test { field: ref __field_field }, Test { field: ref __other_field_field }) =>
							true && ::core::cmp::PartialEq::eq(__field_field, __other_field_field),
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::PartialOrd for Test<T> {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					::core::option::Option::Some(::core::cmp::Ord::cmp(self, __other))
				}
			}
		},
	)
}

#[test]
fn tuple() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Clone, Copy, Debug, Default, Eq, Hash, Ord, PartialEq, PartialOrd)]
			struct Test<T>(std::marker::PhantomData<T>);
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

			#[automatically_derived]
			impl<T> ::core::fmt::Debug for Test<T> {
				fn fmt(&self, __f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
					match self {
						Test(ref __field_0) => {
							let mut __builder = ::core::fmt::Formatter::debug_tuple(__f, "Test");
							::core::fmt::DebugTuple::field(&mut __builder, __field_0);
							::core::fmt::DebugTuple::finish(&mut __builder)
						}
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::default::Default for Test<T> {
				fn default() -> Self {
					Test(::core::default::Default::default())
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::Eq for Test<T> {
				#[inline]
				fn assert_receiver_is_total_eq(&self) {
					struct __AssertEq<__T: ::core::cmp::Eq + ?::core::marker::Sized>(::core::marker::PhantomData<__T>);

					// For some reason the comparison fails without the extra space at the end.
					let _: __AssertEq<std::marker::PhantomData<T> >;
				}
			}

			#[automatically_derived]
			impl<T> ::core::hash::Hash for Test<T> {
				fn hash<__H: ::core::hash::Hasher>(&self, __state: &mut __H) {
					match self {
						Test(ref __field_0) => { ::core::hash::Hash::hash(__field_0, __state); }
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::Ord for Test<T> {
				#[inline]
				fn cmp(&self, __other: &Self) -> ::core::cmp::Ordering {
					match (self, __other) {
						(Test(ref __field_0), Test(ref __other_field_0)) =>
							match ::core::cmp::Ord::cmp(__field_0, __other_field_0) {
								::core::cmp::Ordering::Equal => ::core::cmp::Ordering::Equal,
								__cmp => __cmp,
							},
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::PartialEq for Test<T> {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					match (self, __other) {
						(Test(ref __field_0), Test(ref __other_field_0)) =>
							true && ::core::cmp::PartialEq::eq(__field_0, __other_field_0),
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::PartialOrd for Test<T> {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					::core::option::Option::Some(::core::cmp::Ord::cmp(self, __other))
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
	let ord = quote! {
		::core::cmp::Ord::cmp(&__self_disc, &__other_disc)
	};
	#[cfg(not(feature = "nightly"))]
	let ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> isize {
			match __this {
				Test::A { field: ref __field_field } => 0,
				Test::B { } => 1,
				Test::C(ref __field_0) => 2,
				Test::D() => 3,
				Test::E => 4
			}
		}

		::core::cmp::Ord::cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(Clone, Copy, Debug, Default, Eq, Hash, Ord, PartialEq, PartialOrd)]
			enum Test<T> {
				A { field: std::marker::PhantomData<T>},
				B { },
				C(std::marker::PhantomData<T>),
				D(),
				#[derive_where(default)]
				E,
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

			#[automatically_derived]
			impl<T> ::core::fmt::Debug for Test<T> {
				fn fmt(&self, __f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
					match self {
						Test::A { field: ref __field_field } => {
							let mut __builder = ::core::fmt::Formatter::debug_struct(__f, "A");
							::core::fmt::DebugStruct::field(&mut __builder, "field", __field_field);
							::core::fmt::DebugStruct::finish(&mut __builder)
						}
						Test::B { } => {
							let mut __builder = ::core::fmt::Formatter::debug_struct(__f, "B");
							::core::fmt::DebugStruct::finish(&mut __builder)
						}
						Test::C(ref __field_0) => {
							let mut __builder = ::core::fmt::Formatter::debug_tuple(__f, "C");
							::core::fmt::DebugTuple::field(&mut __builder, __field_0);
							::core::fmt::DebugTuple::finish(&mut __builder)
						}
						Test::D() => {
							let mut __builder = ::core::fmt::Formatter::debug_tuple(__f, "D");
							::core::fmt::DebugTuple::finish(&mut __builder)
						}
						Test::E => ::core::fmt::Formatter::write_str(__f, "E"),
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::default::Default for Test<T> {
				fn default() -> Self {
					Test::E
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::Eq for Test<T> {
				#[inline]
				fn assert_receiver_is_total_eq(&self) {
					struct __AssertEq<__T: ::core::cmp::Eq + ?::core::marker::Sized>(::core::marker::PhantomData<__T>);

					// For some reason the comparison fails without the extra space at the end.
					let _: __AssertEq<std::marker::PhantomData<T> >;
					let _: __AssertEq<std::marker::PhantomData<T> >;
				}
			}

			#[automatically_derived]
			impl<T> ::core::hash::Hash for Test<T> {
				fn hash<__H: ::core::hash::Hasher>(&self, __state: &mut __H) {
					match self {
						Test::A { field: ref __field_field } => {
							::core::hash::Hash::hash(&::core::mem::discriminant(self), __state);
							::core::hash::Hash::hash(__field_field, __state);
						}
						Test::B { } => {
							::core::hash::Hash::hash(&::core::mem::discriminant(self), __state);
						}
						Test::C(ref __field_0) => {
							::core::hash::Hash::hash(&::core::mem::discriminant(self), __state);
							::core::hash::Hash::hash(__field_0, __state);
						}
						Test::D() => {
							::core::hash::Hash::hash(&::core::mem::discriminant(self), __state);
						}
						Test::E => {
							::core::hash::Hash::hash(&::core::mem::discriminant(self), __state);
						}
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::Ord for Test<T> {
				#[inline]
				fn cmp(&self, __other: &Self) -> ::core::cmp::Ordering {
					#discriminant

					if __self_disc == __other_disc {
						match (self, __other) {
							(Test::A { field: ref __field_field }, Test::A { field: ref __other_field_field }) =>
								match ::core::cmp::Ord::cmp(__field_field, __other_field_field) {
									::core::cmp::Ordering::Equal => ::core::cmp::Ordering::Equal,
									__cmp => __cmp,
								},
							(Test::C(ref __field_0), Test::C(ref __other_field_0)) =>
								match ::core::cmp::Ord::cmp(__field_0, __other_field_0) {
									::core::cmp::Ordering::Equal => ::core::cmp::Ordering::Equal,
									__cmp => __cmp,
								},
							_ => ::core::cmp::Ordering::Equal,
						}
					} else {
						#ord
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::PartialEq for Test<T> {
				#[inline]
				fn eq(&self, __other: &Self) -> bool {
					if ::core::mem::discriminant(self) == ::core::mem::discriminant(__other) {
						match (self, __other) {
							(Test::A { field: ref __field_field }, Test::A { field: ref __other_field_field }) =>
								true && ::core::cmp::PartialEq::eq(__field_field, __other_field_field),
							(Test::C(ref __field_0), Test::C(ref __other_field_0)) =>
								true && ::core::cmp::PartialEq::eq(__field_0, __other_field_0),
							_ => true,
						}
					} else {
						false
					}
				}
			}

			#[automatically_derived]
			impl<T> ::core::cmp::PartialOrd for Test<T> {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					::core::option::Option::Some(::core::cmp::Ord::cmp(self, __other))
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
