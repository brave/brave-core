extern crate proc_macro;

use proc_macro::TokenStream;
use proc_macro2::{Span, TokenStream as TokenStream2};
use proc_macro_hack::proc_macro_hack;
use quote::{quote, ToTokens};
use std::path::{Path, PathBuf};
use syn::{
	bracketed,
	parse::{Parse, ParseStream},
	parse_macro_input,
	punctuated::Punctuated,
	token::{Comma, Eq},
	Expr, Ident, LitStr,
};

mod utils;
use self::utils::{describe_cwd, git_dir_cwd};

macro_rules! error {
	($($args:tt)*) => {
		syn::Error::new(Span::call_site(), format!($($args)*))
	};
}

fn canonicalize_path(path: &Path) -> syn::Result<String> {
	Ok(path
		.canonicalize()
		.map_err(|e| error!("failed to canonicalize {}: {}", path.display(), e))?
		.into_os_string()
		.into_string()
		.map_err(|file| error!("invalid UTF-8 in path to {}", PathBuf::from(file).display()))?
	)
}

/// Create a token stream representing dependencies on the git state.
fn git_dependencies() -> syn::Result<TokenStream2> {
	let git_dir = git_dir_cwd().map_err(|e| error!("failed to determine .git directory: {}", e))?;

	let deps: Vec<_> = ["logs/HEAD", "index"].iter().flat_map(|&file| {
		canonicalize_path(&git_dir.join(file)).map(Some).unwrap_or_else(|e|  {
			eprintln!("Failed to add dependency on the git state: {}. Git state changes might not trigger a rebuild.", e);
			None
		})
	}).collect();

	Ok(quote! {
		#( include_bytes!(#deps); )*
	})
}

#[derive(Default)]
struct Args {
	git_args: Option<Punctuated<LitStr, Comma>>,
	prefix: Option<Expr>,
	suffix: Option<Expr>,
	cargo_prefix: Option<Expr>,
	cargo_suffix: Option<Expr>,
	fallback: Option<Expr>,
}

impl Parse for Args {
	fn parse(input: ParseStream) -> syn::Result<Self> {
		let mut result = Args::default();
		loop {
			if input.is_empty() { break; }
			let ident: Ident = input.parse()?;
			let _: Eq = input.parse()?;
			let check_dup = |dup: bool| {
				if dup {
					Err(error!("`{} = ` can only appear once", ident))
				} else {
					Ok(())
				}
			};
			match ident.to_string().as_str() {
				"args" => {
					check_dup(result.git_args.is_some())?;
					let content;
					bracketed!(content in input);
					result.git_args = Some(Punctuated::parse_terminated(&content)?);
				}
				"prefix" => {
					check_dup(result.prefix.is_some())?;
					result.prefix = Some(input.parse()?);
				}
				"suffix" => {
					check_dup(result.suffix.is_some())?;
					result.suffix = Some(input.parse()?);
				}
				"cargo_prefix" => {
					check_dup(result.cargo_prefix.is_some())?;
					result.cargo_prefix = Some(input.parse()?);
				}
				"cargo_suffix" => {
					check_dup(result.cargo_suffix.is_some())?;
					result.cargo_suffix = Some(input.parse()?);
				}
				"fallback" => {
					check_dup(result.fallback.is_some())?;
					result.fallback = Some(input.parse()?);
				}
				x => Err(error!("Unexpected argument name `{}`", x))?,
			}
			if input.is_empty() { break; }
			let _: Comma = input.parse()?;
		}
		Ok(result)
	}
}

#[proc_macro_hack]
pub fn git_version(input: TokenStream) -> TokenStream {
	let args = parse_macro_input!(input as Args);

	let tokens = match git_version_impl(args) {
		Ok(x) => x,
		Err(e) => e.to_compile_error(),
	};

	TokenStream::from(tokens)
}

fn git_version_impl(args: Args) -> syn::Result<TokenStream2> {
	let git_args = args.git_args.map_or_else(
		|| vec!["--always".to_string(), "--dirty=-modified".to_string()],
		|list| list.iter().map(|x| x.value()).collect()
	);

	let cargo_fallback = args.cargo_prefix.is_some() || args.cargo_suffix.is_some();

	match describe_cwd(&git_args) {
		Ok(version) => {
			let dependencies = git_dependencies()?;
			let prefix = args.prefix.iter();
			let suffix = args.suffix;
			Ok(quote!({
				#dependencies;
				concat!(#(#prefix,)* #version, #suffix)
			}))
		}
		Err(_) if cargo_fallback => {
			if let Ok(version) = std::env::var("CARGO_PKG_VERSION") {
				let prefix = args.cargo_prefix.iter();
				let suffix = args.cargo_suffix;
				Ok(quote!(
					concat!(#(#prefix,)* #version, #suffix)
				))
			} else if let Some(fallback) = args.fallback {
				Ok(fallback.to_token_stream())
			} else {
				Err(error!("Unable to get git or cargo version"))
			}
		}
		Err(_) if args.fallback.is_some() => {
			Ok(args.fallback.to_token_stream())
		}
		Err(e) => {
			Err(error!("{}", e))
		}
	}
}
