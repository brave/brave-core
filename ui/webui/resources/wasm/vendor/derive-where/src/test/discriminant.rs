use quote::quote;
use syn::Result;

use super::test_derive;

#[test]
fn default() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> isize {
			const __VALIDATE_ISIZE_A: isize = 0;
			const __VALIDATE_ISIZE_B: isize = (0) + 1;
			const __VALIDATE_ISIZE_C: isize = (0) + 2;

			match __this {
				Test::A => __VALIDATE_ISIZE_A,
				Test::B => __VALIDATE_ISIZE_B,
				Test::C => __VALIDATE_ISIZE_C
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			enum Test {
				A = 0,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn default_clone() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const __VALIDATE_ISIZE_A: isize = 0;
		const __VALIDATE_ISIZE_B: isize = (0) + 1;
		const __VALIDATE_ISIZE_C: isize = (0) + 2;

		::core::cmp::PartialOrd::partial_cmp(&(::core::clone::Clone::clone(self) as isize), &(::core::clone::Clone::clone(__other) as isize))
	};

	test_derive(
		quote! {
			#[derive_where(Clone, PartialOrd)]
			enum Test {
				A = 0,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::clone::Clone for Test {
				#[inline]
				fn clone(&self) -> Self {
					match self {
						Test::A => Test::A,
						Test::B => Test::B,
						Test::C => Test::C,
					}
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn default_copy() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const __VALIDATE_ISIZE_A: isize = 0;
		const __VALIDATE_ISIZE_B: isize = (0) + 1;
		const __VALIDATE_ISIZE_C: isize = (0) + 2;

		::core::cmp::PartialOrd::partial_cmp(&(*self as isize), &(*__other as isize))
	};

	test_derive(
		quote! {
			#[derive_where(Copy, PartialOrd)]
			enum Test {
				A = 0,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::marker::Copy for Test { }

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn default_reverse() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> isize {
			const __VALIDATE_ISIZE_A: isize = 2;
			const __VALIDATE_ISIZE_B: isize = 1;
			const __VALIDATE_ISIZE_C: isize = 0;

			match __this {
				Test::A => __VALIDATE_ISIZE_A,
				Test::B => __VALIDATE_ISIZE_B,
				Test::C => __VALIDATE_ISIZE_C
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			enum Test {
				A = 2,
				B = 1,
				#[derive_where(incomparable)]
				C = 0
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn default_mix() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> isize {
			const __VALIDATE_ISIZE_A: isize = 1;
			const __VALIDATE_ISIZE_B: isize = 0;
			const __VALIDATE_ISIZE_C: isize = 2;
			const __VALIDATE_ISIZE_D: isize = (2) + 1;

			match __this {
				Test::A => __VALIDATE_ISIZE_A,
				Test::B => __VALIDATE_ISIZE_B,
				Test::C => __VALIDATE_ISIZE_C,
				Test::D => __VALIDATE_ISIZE_D
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			enum Test {
				A = 1,
				B = 0,
				C = 2,
				#[derive_where(incomparable)]
				D
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::D) || ::core::matches!(__other, Test::D) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn default_skip() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> isize {
			const __VALIDATE_ISIZE_A: isize = 0;
			const __VALIDATE_ISIZE_B: isize = 3;
			const __VALIDATE_ISIZE_C: isize = (3) + 1;
			const __VALIDATE_ISIZE_D: isize = (3) + 2;
			const __VALIDATE_ISIZE_E: isize = (3) + 3;

			match __this {
				Test::A => __VALIDATE_ISIZE_A,
				Test::B => __VALIDATE_ISIZE_B,
				Test::C => __VALIDATE_ISIZE_C,
				Test::D => __VALIDATE_ISIZE_D,
				Test::E => __VALIDATE_ISIZE_E
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			enum Test {
				A,
				B = 3,
				C,
				#[derive_where(incomparable)]
				D,
				E,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::D) || ::core::matches!(__other, Test::D) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn default_expr() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> isize {
			const __VALIDATE_ISIZE_A: isize = isize::MAX - 2;
			const __VALIDATE_ISIZE_B: isize = (isize::MAX - 2) + 1;
			const __VALIDATE_ISIZE_C: isize = (isize::MAX - 2) + 2;

			match __this {
				Test::A => __VALIDATE_ISIZE_A,
				Test::B => __VALIDATE_ISIZE_B,
				Test::C => __VALIDATE_ISIZE_C
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			enum Test {
				A = isize::MAX - 2,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_c() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> isize {
			const __VALIDATE_ISIZE_A: isize = 0;
			const __VALIDATE_ISIZE_B: isize = (0) + 1;
			const __VALIDATE_ISIZE_C: isize = (0) + 2;

			match __this {
				Test::A => __VALIDATE_ISIZE_A,
				Test::B => __VALIDATE_ISIZE_B,
				Test::C => __VALIDATE_ISIZE_C
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(C)]
			enum Test {
				A = 0,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_c_without_discriminant() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> isize {
			match __this {
				Test::A => 0,
				Test::B => 1,
				Test::C => 2
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(C)]
			enum Test {
				A,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_c_clone() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const __VALIDATE_ISIZE_A: isize = 0;
		const __VALIDATE_ISIZE_B: isize = (0) + 1;
		const __VALIDATE_ISIZE_C: isize = (0) + 2;

		::core::cmp::PartialOrd::partial_cmp(&(::core::clone::Clone::clone(self) as isize), &(::core::clone::Clone::clone(__other) as isize))
	};

	test_derive(
		quote! {
			#[derive_where(Clone, PartialOrd)]
			#[repr(C)]
			enum Test {
				A = 0,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::clone::Clone for Test {
				#[inline]
				fn clone(&self) -> Self {
					match self {
						Test::A => Test::A,
						Test::B => Test::B,
						Test::C => Test::C,
					}
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_c_clone_without_discriminant() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(&(::core::clone::Clone::clone(self) as isize), &(::core::clone::Clone::clone(__other) as isize))
	};

	test_derive(
		quote! {
			#[derive_where(Clone, PartialOrd)]
			#[repr(C)]
			enum Test {
				A,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::clone::Clone for Test {
				#[inline]
				fn clone(&self) -> Self {
					match self {
						Test::A => Test::A,
						Test::B => Test::B,
						Test::C => Test::C,
					}
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_c_copy() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const __VALIDATE_ISIZE_A: isize = 0;
		const __VALIDATE_ISIZE_B: isize = (0) + 1;
		const __VALIDATE_ISIZE_C: isize = (0) + 2;

		::core::cmp::PartialOrd::partial_cmp(&(*self as isize), &(*__other as isize))
	};

	test_derive(
		quote! {
			#[derive_where(Copy, PartialOrd)]
			#[repr(C)]
			enum Test {
				A = 0,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::marker::Copy for Test { }

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_c_copy_without_discriminant() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(&(*self as isize), &(*__other as isize))
	};

	test_derive(
		quote! {
			#[derive_where(Copy, PartialOrd)]
			#[repr(C)]
			enum Test {
				A,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::marker::Copy for Test { }

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_c_reverse() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> isize {
			const __VALIDATE_ISIZE_A: isize = 2;
			const __VALIDATE_ISIZE_B: isize = 1;
			const __VALIDATE_ISIZE_C: isize = 0;

			match __this {
				Test::A => __VALIDATE_ISIZE_A,
				Test::B => __VALIDATE_ISIZE_B,
				Test::C => __VALIDATE_ISIZE_C
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(C)]
			enum Test {
				A = 2,
				B = 1,
				#[derive_where(incomparable)]
				C = 0,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_c_mix() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> isize {
			const __VALIDATE_ISIZE_A: isize = 1;
			const __VALIDATE_ISIZE_B: isize = 0;
			const __VALIDATE_ISIZE_C: isize = 2;
			const __VALIDATE_ISIZE_D: isize = (2) + 1;

			match __this {
				Test::A => __VALIDATE_ISIZE_A,
				Test::B => __VALIDATE_ISIZE_B,
				Test::C => __VALIDATE_ISIZE_C,
				Test::D => __VALIDATE_ISIZE_D
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(C)]
			enum Test {
				A = 1,
				B = 0,
				C = 2,
				#[derive_where(incomparable)]
				D,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::D) || ::core::matches!(__other, Test::D) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_c_skip() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> isize {
			const __VALIDATE_ISIZE_A: isize = 0;
			const __VALIDATE_ISIZE_B: isize = 3;
			const __VALIDATE_ISIZE_C: isize = (3) + 1;
			const __VALIDATE_ISIZE_D: isize = (3) + 2;
			const __VALIDATE_ISIZE_E: isize = (3) + 3;

			match __this {
				Test::A => __VALIDATE_ISIZE_A,
				Test::B => __VALIDATE_ISIZE_B,
				Test::C => __VALIDATE_ISIZE_C,
				Test::D => __VALIDATE_ISIZE_D,
				Test::E => __VALIDATE_ISIZE_E
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(C)]
			enum Test {
				A,
				B = 3,
				C,
				#[derive_where(incomparable)]
				D,
				E,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::D) || ::core::matches!(__other, Test::D) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_c_expr() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> isize {
			const __VALIDATE_ISIZE_A: isize = isize::MAX - 2;
			const __VALIDATE_ISIZE_B: isize = (isize::MAX - 2) + 1;
			const __VALIDATE_ISIZE_C: isize = (isize::MAX - 2) + 2;

			match __this {
				Test::A => __VALIDATE_ISIZE_A,
				Test::B => __VALIDATE_ISIZE_B,
				Test::C => __VALIDATE_ISIZE_C
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(C)]
			enum Test {
				A = isize::MAX - 2,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_c_with_value() -> Result<()> {
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
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u8>() },
			&unsafe { *<*const _>::from(__other).cast::<u8>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> u8 {
			match __this {
				Test::A(ref __field_0) => 0,
				Test::B => (0) + 1,
				Test::C => (0) + 2
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(C, u8)]
			enum Test<T> {
				A(std::marker::PhantomData<T>) = 0,
				B,
				C,
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
							(Test::A(ref __field_0), Test::A(ref __other_field_0)) =>
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
fn repr_c_with_value_reverse() -> Result<()> {
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
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u8>() },
			&unsafe { *<*const _>::from(__other).cast::<u8>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> u8 {
			match __this {
				Test::A(ref __field_0) => 2,
				Test::B => 1,
				Test::C => 0
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(C, u8)]
			enum Test<T> {
				A(std::marker::PhantomData<T>) = 2,
				B = 1,
				C = 0,
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
							(Test::A(ref __field_0), Test::A(ref __other_field_0)) =>
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
fn repr_c_with_value_mix() -> Result<()> {
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
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u8>() },
			&unsafe { *<*const _>::from(__other).cast::<u8>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> u8 {
			match __this {
				Test::A(ref __field_0) => 1,
				Test::B => 0,
				Test::C => 2
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(C, u8)]
			enum Test<T> {
				A(std::marker::PhantomData<T>) = 1,
				B = 0,
				C = 2,
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
							(Test::A(ref __field_0), Test::A(ref __other_field_0)) =>
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
fn repr_c_with_value_skip() -> Result<()> {
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
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u8>() },
			&unsafe { *<*const _>::from(__other).cast::<u8>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> u8 {
			match __this {
				Test::A(ref __field_0) => 0,
				Test::B => 3,
				Test::C => (3) + 1,
				Test::D => (3) + 2
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(C, u8)]
			enum Test<T> {
				A(std::marker::PhantomData<T>),
				B = 3,
				C,
				D,
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
							(Test::A(ref __field_0), Test::A(ref __other_field_0)) =>
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
fn repr_c_with_value_expr() -> Result<()> {
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
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u8>() },
			&unsafe { *<*const _>::from(__other).cast::<u8>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> u8 {
			match __this {
				Test::A(ref __field_0) => isize::MAX - 2,
				Test::B => (isize::MAX - 2) + 1,
				Test::C => (isize::MAX - 2) + 2
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(C, u8)]
			enum Test<T> {
				A(std::marker::PhantomData<T>) = isize::MAX - 2,
				B,
				C,
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
							(Test::A(ref __field_0), Test::A(ref __other_field_0)) =>
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
fn repr() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u64>() },
			&unsafe { *<*const _>::from(__other).cast::<u64>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> u64 {
			match __this {
				Test::A => 0,
				Test::B => (0) + 1,
				Test::C => (0) + 2
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(u64)]
			enum Test {
				A = 0,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_clone() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(&(::core::clone::Clone::clone(self) as u64), &(::core::clone::Clone::clone(__other) as u64))
	};

	test_derive(
		quote! {
			#[derive_where(Clone, PartialOrd)]
			#[repr(u64)]
			enum Test {
				A = 0,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::clone::Clone for Test {
				#[inline]
				fn clone(&self) -> Self {
					match self {
						Test::A => Test::A,
						Test::B => Test::B,
						Test::C => Test::C,
					}
				}
			}

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_copy() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(feature = "nightly"))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(&(*self as u64), &(*__other as u64))
	};

	test_derive(
		quote! {
			#[derive_where(Copy, PartialOrd)]
			#[repr(u64)]
			enum Test {
				A = 0,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::marker::Copy for Test { }

			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_reverse() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u64>() },
			&unsafe { *<*const _>::from(__other).cast::<u64>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> u64 {
			match __this {
				Test::A => 2,
				Test::B => 1,
				Test::C => 0
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(u64)]
			enum Test {
				A = 2,
				B = 1,
				#[derive_where(incomparable)]
				C = 0,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_mix() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u64>() },
			&unsafe { *<*const _>::from(__other).cast::<u64>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> u64 {
			match __this {
				Test::A => 1,
				Test::B => 0,
				Test::C => 2,
				Test::D => (2) + 1
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(u64)]
			enum Test {
				A = 1,
				B = 0,
				C = 2,
				#[derive_where(incomparable)]
				D,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::D) || ::core::matches!(__other, Test::D) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_skip() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u64>() },
			&unsafe { *<*const _>::from(__other).cast::<u64>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> u64 {
			match __this {
				Test::A => 0,
				Test::B => 3,
				Test::C => (3) + 1,
				Test::D => (3) + 2,
				Test::E => (3) + 3
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(u64)]
			enum Test {
				A,
				B = 3,
				C,
				#[derive_where(incomparable)]
				D,
				E
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::D) || ::core::matches!(__other, Test::D) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_expr() -> Result<()> {
	#[cfg(feature = "nightly")]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&::core::intrinsics::discriminant_value(self),
			&::core::intrinsics::discriminant_value(__other),
		)
	};
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u64>() },
			&unsafe { *<*const _>::from(__other).cast::<u64>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant(__this: &Test) -> u64 {
			match __this {
				Test::A => u64::MAX - 2,
				Test::B => (u64::MAX - 2) + 1,
				Test::C => (u64::MAX - 2) + 2
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(u64)]
			enum Test {
				A = u64::MAX - 2,
				B,
				#[derive_where(incomparable)]
				C,
			}
		},
		quote! {
			#[automatically_derived]
			impl ::core::cmp::PartialOrd for Test {
				#[inline]
				fn partial_cmp(&self, __other: &Self) -> ::core::option::Option<::core::cmp::Ordering> {
					if ::core::matches!(self, Test::C) || ::core::matches!(__other, Test::C) {
						return ::core::option::Option::None;
					}

					#partial_ord
				}
			}
		},
	)
}

#[test]
fn repr_with_value() -> Result<()> {
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
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u8>() },
			&unsafe { *<*const _>::from(__other).cast::<u8>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> u8 {
			match __this {
				Test::A(ref __field_0) => 0,
				Test::B => (0) + 1,
				Test::C => (0) + 2
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(u8)]
			enum Test<T> {
				A(std::marker::PhantomData<T>) = 0,
				B,
				C,
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
							(Test::A(ref __field_0), Test::A(ref __other_field_0)) =>
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
fn repr_with_value_reverse() -> Result<()> {
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
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u8>() },
			&unsafe { *<*const _>::from(__other).cast::<u8>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> u8 {
			match __this {
				Test::A(ref __field_0) => 2,
				Test::B => 1,
				Test::C => 0
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(u8)]
			enum Test<T> {
				A(std::marker::PhantomData<T>) = 2,
				B = 1,
				C = 0,
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
							(Test::A(ref __field_0), Test::A(ref __other_field_0)) =>
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
fn repr_with_value_mix() -> Result<()> {
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
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u8>() },
			&unsafe { *<*const _>::from(__other).cast::<u8>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> u8 {
			match __this {
				Test::A(ref __field_0) => 1,
				Test::B => 0,
				Test::C => 2
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(u8)]
			enum Test<T> {
				A(std::marker::PhantomData<T>) = 1,
				B = 0,
				C = 2,
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
							(Test::A(ref __field_0), Test::A(ref __other_field_0)) =>
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
fn repr_with_value_skip() -> Result<()> {
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
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u8>() },
			&unsafe { *<*const _>::from(__other).cast::<u8>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> u8 {
			match __this {
				Test::A(ref __field_0) => 0,
				Test::B => 3,
				Test::C => (3) + 1,
				Test::D => (3) + 2
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(u8)]
			enum Test<T> {
				A(std::marker::PhantomData<T>),
				B = 3,
				C,
				D,
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
							(Test::A(ref __field_0), Test::A(ref __other_field_0)) =>
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
fn repr_with_value_expr() -> Result<()> {
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
	#[cfg(not(any(feature = "nightly", feature = "safe")))]
	let partial_ord = quote! {
		::core::cmp::PartialOrd::partial_cmp(
			&unsafe { *<*const _>::from(self).cast::<u8>() },
			&unsafe { *<*const _>::from(__other).cast::<u8>() },
		)
	};
	#[cfg(all(not(feature = "nightly"), feature = "safe"))]
	let partial_ord = quote! {
		const fn __discriminant<T>(__this: &Test<T>) -> u8 {
			match __this {
				Test::A(ref __field_0) => isize::MAX - 2,
				Test::B => (isize::MAX - 2) + 1,
				Test::C => (isize::MAX - 2) + 2
			}
		}

		::core::cmp::PartialOrd::partial_cmp(&__discriminant(self), &__discriminant(__other))
	};

	test_derive(
		quote! {
			#[derive_where(PartialOrd)]
			#[repr(u8)]
			enum Test<T> {
				A(std::marker::PhantomData<T>) = isize::MAX - 2,
				B,
				C,
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
							(Test::A(ref __field_0), Test::A(ref __other_field_0)) =>
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
