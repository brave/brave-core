use quote::quote;
use syn::Result;

use super::compiles;

#[test]
fn struct_skip() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		struct Test(#[derive_where(skip)] u8);
	})
}

#[test]
fn struct_skip_multiple() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		struct Test(#[derive_where(skip)] u8, u8);
	})
}

#[test]
fn struct_skip_trait() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		struct Test(#[derive_where(skip(Debug))] u8);
	})
}

#[test]
fn struct_skip_trait_multiple() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		struct Test(#[derive_where(skip(Debug))] u8, u8);
	})
}

#[test]
fn struct_skip_inner() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		#[derive_where(skip_inner)]
		struct Test(u8);
	})
}

#[test]
fn struct_skip_inner_trait() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		#[derive_where(skip_inner(Debug))]
		struct Test(u8);
	})
}
#[test]
fn enum_skip() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		enum Test {
			A(#[derive_where(skip)] u8),
		}
	})
}

#[test]
fn enum_skip_multiple() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		enum Test {
			A(#[derive_where(skip)] u8),
			B,
		}
	})
}

#[test]
fn enum_skip_trait() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		enum Test {
			A(#[derive_where(skip(Debug))] u8),
		}
	})
}

#[test]
fn enum_skip_trait_multiple() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		enum Test {
			A(#[derive_where(skip(Debug))] u8),
			B,
		}
	})
}

#[test]
fn enum_skip_inner() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		enum Test {
			#[derive_where(skip_inner)]
			A(u8),
		}
	})
}

#[test]
fn enum_skip_inner_trait() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug)]
		enum Test {
			#[derive_where(skip_inner(Debug))]
			A(u8),
		}
	})
}

#[test]
fn default() -> Result<()> {
	compiles(quote! {
		#[derive_where(Default)]
		enum Test {
			#[derive_where(default)]
			A,
		}
	})
}

#[test]
#[cfg(feature = "zeroize")]
fn zeroize_crate() -> Result<()> {
	compiles(quote! {
		#[derive_where(Zeroize(crate = zeroize_))]
		struct Test(u8);
	})
}

#[test]
#[cfg(feature = "zeroize")]
fn zeroize_fqs() -> Result<()> {
	compiles(quote! {
		#[derive_where(Zeroize)]
		struct Test(#[derive_where(Zeroize(fqs))] u8);
	})
}

#[test]
fn custom_bound() -> Result<()> {
	compiles(quote! {
		#[derive_where(Debug; T: Clone)]
		struct Test<T>(T);
	})
}
