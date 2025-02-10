//! **Why bother writing similar code twice for blocking and async code?**
//!
//! [![Build Status](https://github.com/fMeow/maybe-async-rs/workflows/CI%20%28Linux%29/badge.svg?branch=main)](https://github.com/fMeow/maybe-async-rs/actions)
//! [![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)
//! [![Latest Version](https://img.shields.io/crates/v/maybe-async.svg)](https://crates.io/crates/maybe-async)
//! [![maybe-async](https://docs.rs/maybe-async/badge.svg)](https://docs.rs/maybe-async)
//!
//! When implementing both sync and async versions of API in a crate, most API
//! of the two version are almost the same except for some async/await keyword.
//!
//! `maybe-async` help unifying async and sync implementation by **procedural
//! macro**.
//! - Write async code with normal `async`, `await`, and let `maybe_async`
//!   handles
//! those `async` and `await` when you need a blocking code.
//! - Switch between sync and async by toggling `is_sync` feature gate in
//!   `Cargo.toml`.
//! - use `must_be_async` and `must_be_sync` to keep code in specified version
//! - use `async_impl` and `sync_impl` to only compile code block on specified
//!   version
//! - A handy macro to unify unit test code is also provided.
//!
//! These procedural macros can be applied to the following codes:
//! - trait item declaration
//! - trait implementation
//! - function definition
//! - struct definition
//!
//! **RECOMMENDATION**: Enable **resolver ver2** in your crate, which is
//! introduced in Rust 1.51. If not, two crates in dependency with conflict
//! version (one async and another blocking) can fail compilation.
//!
//!
//! ## Motivation
//!
//! The async/await language feature alters the async world of rust.
//! Comparing with the map/and_then style, now the async code really resembles
//! sync version code.
//!
//! In many crates, the async and sync version of crates shares the same API,
//! but the minor difference that all async code must be awaited prevent the
//! unification of async and sync code. In other words, we are forced to write
//! an async and a sync implementation respectively.
//!
//! ## Macros in Detail
//!
//! `maybe-async` offers 4 set of attribute macros: `maybe_async`,
//! `sync_impl`/`async_impl`, `must_be_sync`/`must_be_async`,  and `test`.
//!
//! To use `maybe-async`, we must know which block of codes is only used on
//! blocking implementation, and which on async. These two implementation should
//! share the same function signatures except for async/await keywords, and use
//! `sync_impl` and `async_impl` to mark these implementation.
//!
//! Use `maybe_async` macro on codes that share the same API on both async and
//! blocking code except for async/await keywords. And use feature gate
//! `is_sync` in `Cargo.toml` to toggle between async and blocking code.
//!
//! - `maybe_async`
//!
//!     Offers a unified feature gate to provide sync and async conversion on
//!     demand by feature gate `is_sync`, with **async first** policy.
//!
//!     Want to keep async code? add `maybe_async` in dependencies with default
//!     features, which means `maybe_async` is the same as `must_be_async`:
//!
//!     ```toml
//!     [dependencies]
//!     maybe_async = "0.2"
//!     ```
//!
//!     Want to convert async code to sync? Add `maybe_async` to dependencies with
//!     an `is_sync` feature gate. In this way, `maybe_async` is the same as
//!     `must_be_sync`:
//!
//!     ```toml
//!     [dependencies]
//!     maybe_async = { version = "0.2", features = ["is_sync"] }
//!     ```
//!
//!     There are three usage variants for `maybe_async` attribute usage:
//!     - `#[maybe_async]` or `#[maybe_async(Send)]`
//!
//!        In this mode, `#[async_trait::async_trait]` is added to trait declarations and trait implementations
//!        to support async fn in traits.
//!
//!     - `#[maybe_async(?Send)]`
//!
//!        Not all async traits need futures that are `dyn Future + Send`.
//!        In this mode, `#[async_trait::async_trait(?Send)]` is added to trait declarations and trait implementations,
//!        to avoid having "Send" and "Sync" bounds placed on the async trait
//!        methods.
//!
//!     - `#[maybe_async(AFIT)]`
//!
//!        AFIT is acronym for **a**sync **f**unction **i**n **t**rait, stabilized from rust 1.74
//!
//!     For compatibility reasons, the `async fn` in traits is supported via a verbose `AFIT` flag. This will become
//!     the default mode for the next major release.
//!
//! - `must_be_async`
//!
//!     **Keep async**.
//!
//!     There are three usage variants for `must_be_async` attribute usage:
//!     - `#[must_be_async]` or `#[must_be_async(Send)]`
//!     - `#[must_be_async(?Send)]`
//!     - `#[must_be_async(AFIT)]`
//!
//! - `must_be_sync`
//!
//!     **Convert to sync code**. Convert the async code into sync code by
//!     removing all `async move`, `async` and `await` keyword
//!
//!
//! - `sync_impl`
//!
//!     A sync implementation should compile on blocking implementation and
//!     must simply disappear when we want async version.
//!
//!     Although most of the API are almost the same, there definitely come to a
//!     point when the async and sync version should differ greatly. For
//!     example, a MongoDB client may use the same API for async and sync
//!     version, but the code to actually send reqeust are quite different.
//!
//!     Here, we can use `sync_impl` to mark a synchronous implementation, and a
//!     sync implementation should disappear when we want async version.
//!
//! - `async_impl`
//!
//!     An async implementation should on compile on async implementation and
//!     must simply disappear when we want sync version.
//!
//!     There are three usage variants for `async_impl` attribute usage:
//!     - `#[async_impl]` or `#[async_impl(Send)]`
//!     - `#[async_impl(?Send)]`
//!     - `#[async_impl(AFIT)]`
//!
//! - `test`
//!
//!     Handy macro to unify async and sync **unit and e2e test** code.
//!
//!     You can specify the condition to compile to sync test code
//!     and also the conditions to compile to async test code with given test
//!     macro, e.x. `tokio::test`, `async_std::test`, etc. When only sync
//!     condition is specified,the test code only compiles when sync condition
//!     is met.
//!
//!     ```rust
//!     # #[maybe_async::maybe_async]
//!     # async fn async_fn() -> bool {
//!     #    true
//!     # }
//!
//!     ##[maybe_async::test(
//!         feature="is_sync",
//!         async(
//!             all(not(feature="is_sync"), feature="async_std"),
//!             async_std::test
//!         ),
//!         async(
//!             all(not(feature="is_sync"), feature="tokio"),
//!             tokio::test
//!         )
//!     )]
//!     async fn test_async_fn() {
//!         let res = async_fn().await;
//!         assert_eq!(res, true);
//!     }
//!     ```
//!
//! ## What's Under the Hook
//!
//! `maybe-async` compiles your code in different way with the `is_sync` feature
//! gate. It removes all `await` and `async` keywords in your code under
//! `maybe_async` macro and conditionally compiles codes under `async_impl` and
//! `sync_impl`.
//!
//! Here is a detailed example on what's going on whe the `is_sync` feature
//! gate set or not.
//!
//! ```rust
//! #[maybe_async::maybe_async(AFIT)]
//! trait A {
//!     async fn async_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//!     fn sync_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//! }
//!
//! struct Foo;
//!
//! #[maybe_async::maybe_async(AFIT)]
//! impl A for Foo {
//!     async fn async_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//!     fn sync_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//! }
//!
//! #[maybe_async::maybe_async]
//! async fn maybe_async_fn() -> Result<(), ()> {
//!     let a = Foo::async_fn_name().await?;
//!
//!     let b = Foo::sync_fn_name()?;
//!     Ok(())
//! }
//! ```
//!
//! When `maybe-async` feature gate `is_sync` is **NOT** set, the generated code
//! is async code:
//!
//! ```rust
//! // Compiled code when `is_sync` is toggled off.
//! trait A {
//!     async fn maybe_async_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//!     fn sync_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//! }
//!
//! struct Foo;
//!
//! impl A for Foo {
//!     async fn maybe_async_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//!     fn sync_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//! }
//!
//! async fn maybe_async_fn() -> Result<(), ()> {
//!     let a = Foo::maybe_async_fn_name().await?;
//!     let b = Foo::sync_fn_name()?;
//!     Ok(())
//! }
//! ```
//!
//! When `maybe-async` feature gate `is_sync` is set, all async keyword is
//! ignored and yields a sync version code:
//!
//! ```rust
//! // Compiled code when `is_sync` is toggled on.
//! trait A {
//!     fn maybe_async_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//!     fn sync_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//! }
//!
//! struct Foo;
//!
//! impl A for Foo {
//!     fn maybe_async_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//!     fn sync_fn_name() -> Result<(), ()> {
//!         Ok(())
//!     }
//! }
//!
//! fn maybe_async_fn() -> Result<(), ()> {
//!     let a = Foo::maybe_async_fn_name()?;
//!     let b = Foo::sync_fn_name()?;
//!     Ok(())
//! }
//! ```
//!
//! ## Examples
//!
//! ### rust client for services
//!
//! When implementing rust client for any services, like awz3. The higher level
//! API of async and sync version is almost the same, such as creating or
//! deleting a bucket, retrieving an object, etc.
//!
//! The example `service_client` is a proof of concept that `maybe_async` can
//! actually free us from writing almost the same code for sync and async. We
//! can toggle between a sync AWZ3 client and async one by `is_sync` feature
//! gate when we add `maybe-async` to dependency.
//!
//!
//! # License
//! MIT

extern crate proc_macro;

use proc_macro::TokenStream;

use proc_macro2::{Span, TokenStream as TokenStream2};
use syn::{
    ext::IdentExt,
    parenthesized,
    parse::{ParseStream, Parser},
    parse_macro_input, token, Ident, ImplItem, LitStr, Meta, Result, Token, TraitItem,
};

use quote::quote;

use crate::{parse::Item, visit::AsyncAwaitRemoval};

mod parse;
mod visit;
enum AsyncTraitMode {
    Send,
    NotSend,
    Off,
}

fn convert_async(input: &mut Item, async_trait_mode: AsyncTraitMode) -> TokenStream2 {
    match input {
        Item::Trait(item) => match async_trait_mode {
            AsyncTraitMode::Send => quote!(#[async_trait::async_trait]#item),
            AsyncTraitMode::NotSend => quote!(#[async_trait::async_trait(?Send)]#item),
            AsyncTraitMode::Off => quote!(#item),
        },
        Item::Impl(item) => {
            let async_trait_mode = item
                .trait_
                .as_ref()
                .map_or(AsyncTraitMode::Off, |_| async_trait_mode);
            match async_trait_mode {
                AsyncTraitMode::Send => quote!(#[async_trait::async_trait]#item),
                AsyncTraitMode::NotSend => quote!(#[async_trait::async_trait(?Send)]#item),
                AsyncTraitMode::Off => quote!(#item),
            }
        }
        Item::Fn(item) => quote!(#item),
        Item::Static(item) => quote!(#item),
    }
}

fn convert_sync(input: &mut Item) -> TokenStream2 {
    match input {
        Item::Impl(item) => {
            for inner in &mut item.items {
                if let ImplItem::Fn(ref mut method) = inner {
                    if method.sig.asyncness.is_some() {
                        method.sig.asyncness = None;
                    }
                }
            }
            AsyncAwaitRemoval.remove_async_await(quote!(#item))
        }
        Item::Trait(item) => {
            for inner in &mut item.items {
                if let TraitItem::Fn(ref mut method) = inner {
                    if method.sig.asyncness.is_some() {
                        method.sig.asyncness = None;
                    }
                }
            }
            AsyncAwaitRemoval.remove_async_await(quote!(#item))
        }
        Item::Fn(item) => {
            if item.sig.asyncness.is_some() {
                item.sig.asyncness = None;
            }
            AsyncAwaitRemoval.remove_async_await(quote!(#item))
        }
        Item::Static(item) => AsyncAwaitRemoval.remove_async_await(quote!(#item)),
    }
}

fn async_mode(arg: &str) -> Result<AsyncTraitMode> {
    match arg {
        "" | "Send" => Ok(AsyncTraitMode::Send),
        "?Send" => Ok(AsyncTraitMode::NotSend),
        // acronym for Async Function in Trait,
        // TODO make AFIT as default in future release
        "AFIT" => Ok(AsyncTraitMode::Off),
        _ => Err(syn::Error::new(
            Span::call_site(),
            "Only accepts `Send`, `?Send` or `AFIT` (native async function in trait)",
        )),
    }
}

/// maybe_async attribute macro
///
/// Can be applied to trait item, trait impl, functions and struct impls.
#[proc_macro_attribute]
pub fn maybe_async(args: TokenStream, input: TokenStream) -> TokenStream {
    let mode = match async_mode(args.to_string().replace(" ", "").as_str()) {
        Ok(m) => m,
        Err(e) => return e.to_compile_error().into(),
    };
    let mut item = parse_macro_input!(input as Item);

    let token = if cfg!(feature = "is_sync") {
        convert_sync(&mut item)
    } else {
        convert_async(&mut item, mode)
    };
    token.into()
}

/// convert marked async code to async code with `async-trait`
#[proc_macro_attribute]
pub fn must_be_async(args: TokenStream, input: TokenStream) -> TokenStream {
    let mode = match async_mode(args.to_string().replace(" ", "").as_str()) {
        Ok(m) => m,
        Err(e) => return e.to_compile_error().into(),
    };
    let mut item = parse_macro_input!(input as Item);
    convert_async(&mut item, mode).into()
}

/// convert marked async code to sync code
#[proc_macro_attribute]
pub fn must_be_sync(_args: TokenStream, input: TokenStream) -> TokenStream {
    let mut item = parse_macro_input!(input as Item);
    convert_sync(&mut item).into()
}

/// mark sync implementation
///
/// only compiled when `is_sync` feature gate is set.
/// When `is_sync` is not set, marked code is removed.
#[proc_macro_attribute]
pub fn sync_impl(_args: TokenStream, input: TokenStream) -> TokenStream {
    let input = TokenStream2::from(input);
    let token = if cfg!(feature = "is_sync") {
        quote!(#input)
    } else {
        quote!()
    };
    token.into()
}

/// mark async implementation
///
/// only compiled when `is_sync` feature gate is not set.
/// When `is_sync` is set, marked code is removed.
#[proc_macro_attribute]
pub fn async_impl(args: TokenStream, _input: TokenStream) -> TokenStream {
    let mode = match async_mode(args.to_string().replace(" ", "").as_str()) {
        Ok(m) => m,
        Err(e) => return e.to_compile_error().into(),
    };
    let token = if cfg!(feature = "is_sync") {
        quote!()
    } else {
        let mut item = parse_macro_input!(_input as Item);
        convert_async(&mut item, mode)
    };
    token.into()
}

fn parse_nested_meta_or_str(input: ParseStream) -> Result<TokenStream2> {
    if let Some(s) = input.parse::<Option<LitStr>>()? {
        let tokens = s.value().parse()?;
        Ok(tokens)
    } else {
        let meta: Meta = input.parse()?;
        Ok(quote!(#meta))
    }
}

/// Handy macro to unify test code of sync and async code
///
/// Since the API of both sync and async code are the same,
/// with only difference that async functions must be awaited.
/// So it's tedious to write unit sync and async respectively.
///
/// This macro helps unify the sync and async unit test code.
/// Pass the condition to treat test code as sync as the first
/// argument. And specify the condition when to treat test code
/// as async and the lib to run async test, e.x. `async-std::test`,
/// `tokio::test`, or any valid attribute macro.
///
/// **ATTENTION**: do not write await inside a assert macro
///
/// - Examples
///
/// ```rust
/// #[maybe_async::maybe_async]
/// async fn async_fn() -> bool {
///     true
/// }
///
/// #[maybe_async::test(
///     // when to treat the test code as sync version
///     feature="is_sync",
///     // when to run async test
///     async(all(not(feature="is_sync"), feature="async_std"), async_std::test),
///     // you can specify multiple conditions for different async runtime
///     async(all(not(feature="is_sync"), feature="tokio"), tokio::test)
/// )]
/// async fn test_async_fn() {
///     let res = async_fn().await;
///     assert_eq!(res, true);
/// }
///
/// // Only run test in sync version
/// #[maybe_async::test(feature = "is_sync")]
/// async fn test_sync_fn() {
///     let res = async_fn().await;
///     assert_eq!(res, true);
/// }
/// ```
///
/// The above code is transcripted to the following code:
///
/// ```rust
/// # use maybe_async::{must_be_async, must_be_sync, sync_impl};
/// # #[maybe_async::maybe_async]
/// # async fn async_fn() -> bool { true }
///
/// // convert to sync version when sync condition is met, keep in async version when corresponding
/// // condition is met
/// #[cfg_attr(feature = "is_sync", must_be_sync, test)]
/// #[cfg_attr(
///     all(not(feature = "is_sync"), feature = "async_std"),
///     must_be_async,
///     async_std::test
/// )]
/// #[cfg_attr(
///     all(not(feature = "is_sync"), feature = "tokio"),
///     must_be_async,
///     tokio::test
/// )]
/// async fn test_async_fn() {
///     let res = async_fn().await;
///     assert_eq!(res, true);
/// }
///
/// // force converted to sync function, and only compile on sync condition
/// #[cfg(feature = "is_sync")]
/// #[test]
/// fn test_sync_fn() {
///     let res = async_fn();
///     assert_eq!(res, true);
/// }
/// ```
#[proc_macro_attribute]
pub fn test(args: TokenStream, input: TokenStream) -> TokenStream {
    match parse_test_cfg.parse(args) {
        Ok(test_cfg) => [test_cfg.into(), input].into_iter().collect(),
        Err(err) => err.to_compile_error().into(),
    }
}

fn parse_test_cfg(input: ParseStream) -> Result<TokenStream2> {
    if input.is_empty() {
        return Err(syn::Error::new(
            Span::call_site(),
            "Arguments cannot be empty, at least specify the condition for sync code",
        ));
    }

    // The first attributes indicates sync condition
    let sync_cond = input.call(parse_nested_meta_or_str)?;
    let mut ts = quote!(#[cfg_attr(#sync_cond, maybe_async::must_be_sync, test)]);

    // The rest attributes indicates async condition and async test macro
    // only accepts in the forms of `async(cond, test_macro)`, but `cond` and
    // `test_macro` can be either meta attributes or string literal
    let mut async_conditions = Vec::new();
    while !input.is_empty() {
        input.parse::<Token![,]>()?;
        if input.is_empty() {
            break;
        }

        if !input.peek(Ident::peek_any) {
            return Err(
                input.error("Must be list of metas like: `async(condition, async_test_macro)`")
            );
        }
        let name = input.call(Ident::parse_any)?;
        if name != "async" {
            return Err(syn::Error::new(
                name.span(),
                format!("Unknown path: `{}`, must be `async`", name),
            ));
        }

        if !input.peek(token::Paren) {
            return Err(
                input.error("Must be list of metas like: `async(condition, async_test_macro)`")
            );
        }

        let nested;
        parenthesized!(nested in input);
        let list = nested.parse_terminated(parse_nested_meta_or_str, Token![,])?;
        let len = list.len();
        let mut iter = list.into_iter();
        let (Some(async_cond), Some(async_test), None) = (iter.next(), iter.next(), iter.next())
        else {
            let msg = format!(
                "Must pass two metas or string literals like `async(condition, \
                 async_test_macro)`, you passed {len} metas.",
            );
            return Err(syn::Error::new(name.span(), msg));
        };

        let attr = quote!(
            #[cfg_attr(#async_cond, maybe_async::must_be_async, #async_test)]
        );
        async_conditions.push(async_cond);
        ts.extend(attr);
    }

    Ok(if !async_conditions.is_empty() {
        quote! {
            #[cfg(any(#sync_cond, #(#async_conditions),*))]
            #ts
        }
    } else {
        quote! {
            #[cfg(#sync_cond)]
            #ts
        }
    })
}
