use quote::quote;
use syn::Result;

use super::test_derive;

#[test]
fn ignore_foreign_attribute() -> Result<()> {
	test_derive(
		quote! {
			#[derive_where(Default; T)]
			#[foreign(default)]
			enum Test<T> {
				#[foreign(default)]
				A { field: T },
				#[derive_where(default)]
				B { field: T },
			}
		},
		quote! {
			#[automatically_derived]
			impl<T> ::core::default::Default for Test<T>
			where T: ::core::default::Default
			{
				fn default() -> Self {
					Test::B { field: ::core::default::Default::default() }
				}
			}
		},
	)
}
