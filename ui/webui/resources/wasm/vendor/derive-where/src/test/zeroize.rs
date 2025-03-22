use quote::quote;
use syn::Result;

use super::test_derive;

#[test]
fn basic() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Zeroize)]
			struct Test<T>(std::marker::PhantomData<T>);
		},
		quote! {
			#[automatically_derived]
			impl<T> ::zeroize::Zeroize for Test<T> {
				fn zeroize(&mut self) {
					use ::zeroize::Zeroize;

					match self {
						Test(ref mut __field_0) => {
							__field_0.zeroize();
						}
					}
				}
			}
		},
	)
}

#[test]
fn drop() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(ZeroizeOnDrop; T)]
			struct Test<T, U>(T, std::marker::PhantomData<U>);
		},
		#[cfg(not(feature = "zeroize-on-drop"))]
		quote! {
			#[automatically_derived]
			impl<T, U> ::core::ops::Drop for Test<T, U>
			where T: ::zeroize::ZeroizeOnDrop
			{
				fn drop(&mut self) {
					::zeroize::Zeroize::zeroize(self);
				}
			}
		},
		#[cfg(feature = "zeroize-on-drop")]
		quote! {
			#[automatically_derived]
			impl<T, U> ::core::ops::Drop for Test<T, U>
			where T: ::zeroize::ZeroizeOnDrop
			{
				fn drop(&mut self) {
					use ::zeroize::__internal::AssertZeroize;
					use ::zeroize::__internal::AssertZeroizeOnDrop;

					match self {
						Test(ref mut __field_0, ref mut __field_1) => {
							__field_0.zeroize_or_on_drop();
							__field_1.zeroize_or_on_drop();
						}
					}
				}
			}

			#[automatically_derived]
			impl<T, U> ::zeroize::ZeroizeOnDrop for Test<T, U>
			where T: ::zeroize::ZeroizeOnDrop
			{ }
		},
	)
}

#[test]
fn both() -> Result<()> {
	#[cfg(not(feature = "zeroize-on-drop"))]
	let drop = quote! {
		#[automatically_derived]
		impl<T, U> ::core::ops::Drop for Test<T, U>
		where T: ::zeroize::ZeroizeOnDrop
		{
			fn drop(&mut self) {
				::zeroize::Zeroize::zeroize(self);
			}
		}
	};
	#[cfg(feature = "zeroize-on-drop")]
	let drop = quote! {
		#[automatically_derived]
		impl<T, U> ::core::ops::Drop for Test<T, U>
		where T: ::zeroize::ZeroizeOnDrop
		{
			fn drop(&mut self) {
				use ::zeroize::__internal::AssertZeroize;
				use ::zeroize::__internal::AssertZeroizeOnDrop;

				match self {
					Test(ref mut __field_0, ref mut __field_1) => {
						__field_0.zeroize_or_on_drop();
						__field_1.zeroize_or_on_drop();
					}
				}
			}
		}

		#[automatically_derived]
		impl<T, U> ::zeroize::ZeroizeOnDrop for Test<T, U>
		where T: ::zeroize::ZeroizeOnDrop
		{ }
	};

	test_derive(
		quote! {
			#[derive_where(Zeroize, ZeroizeOnDrop; T)]
			struct Test<T, U>(T, std::marker::PhantomData<U>);
		},
		quote! {
			#[automatically_derived]
			impl<T, U> ::zeroize::Zeroize for Test<T, U>
			where T: ::zeroize::Zeroize
			{
				fn zeroize(&mut self) {
					use ::zeroize::Zeroize;

					match self {
						Test(ref mut __field_0, ref mut __field_1) => {
							__field_0.zeroize();
							__field_1.zeroize();
						}
					}
				}
			}

			#drop
		},
	)
}

#[test]
fn crate_() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Zeroize(crate = zeroize_); T)]
			struct Test<T>(T);
		},
		quote! {
			#[automatically_derived]
			impl<T> zeroize_::Zeroize for Test<T>
			where T: zeroize_::Zeroize
			{
				fn zeroize(&mut self) {
					use zeroize_::Zeroize;

					match self {
						Test(ref mut __field_0) => {
							__field_0.zeroize();
						}
					}
				}
			}
		},
	)
}

#[test]
fn crate_drop() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(ZeroizeOnDrop(crate = zeroize_); T)]
			struct Test<T>(T);
		},
		#[cfg(not(feature = "zeroize-on-drop"))]
		quote! {
			#[automatically_derived]
			impl<T> ::core::ops::Drop for Test<T>
			where T: zeroize_::ZeroizeOnDrop
			{
				fn drop(&mut self) {
					zeroize_::Zeroize::zeroize(self);
				}
			}
		},
		#[cfg(feature = "zeroize-on-drop")]
		quote! {
			#[automatically_derived]
			impl<T> ::core::ops::Drop for Test<T>
			where T: zeroize_::ZeroizeOnDrop
			{
				fn drop(&mut self) {
					use zeroize_::__internal::AssertZeroize;
					use zeroize_::__internal::AssertZeroizeOnDrop;

					match self {
						Test(ref mut __field_0) => {
							__field_0.zeroize_or_on_drop();
						}
					}
				}
			}

			#[automatically_derived]
			impl<T> zeroize_::ZeroizeOnDrop for Test<T>
			where T: zeroize_::ZeroizeOnDrop
			{ }
		},
	)
}

#[test]
fn fqs() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Zeroize)]
			struct Test<T>(#[derive_where(Zeroize(fqs))] std::marker::PhantomData<T>);
		},
		quote! {
			#[automatically_derived]
			impl<T> ::zeroize::Zeroize for Test<T> {
				fn zeroize(&mut self) {
					use ::zeroize::Zeroize;

					match self {
						Test(ref mut __field_0) => {
							::zeroize::Zeroize::zeroize(__field_0);
						}
					}
				}
			}
		},
	)
}
