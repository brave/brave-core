/*!
Getset, we're ready to go!

A procedural macro for generating the most basic getters and setters on fields.

Getters are generated as `fn field(&self) -> &type`, while setters are generated as `fn field(&mut self, val: type)`.

These macros are not intended to be used on fields which require custom logic inside of their setters and getters. Just write your own in that case!

```rust
use std::sync::Arc;

use getset::{CloneGetters, CopyGetters, Getters, MutGetters, Setters, WithSetters};

#[derive(Getters, Setters, WithSetters, MutGetters, CopyGetters, CloneGetters, Default)]
pub struct Foo<T>
where
    T: Copy + Clone + Default,
{
    /// Doc comments are supported!
    /// Multiline, even.
    #[getset(get, set, get_mut, set_with)]
    private: T,

    /// Doc comments are supported!
    /// Multiline, even.
    #[getset(get_copy = "pub", set = "pub", get_mut = "pub", set_with = "pub")]
    public: T,

    /// Arc supported through CloneGetters
    #[getset(get_clone = "pub", set = "pub", get_mut = "pub", set_with = "pub")]
    arc: Arc<u16>,
}
```

You can use `cargo-expand` to generate the output. Here are the functions that the above generates (Replicate with `cargo expand --example simple`):

```rust,ignore
use std::sync::Arc;
use getset::{CloneGetters, CopyGetters, Getters, MutGetters, Setters, WithSetters};
pub struct Foo<T>
where
    T: Copy + Clone + Default,
{
    /// Doc comments are supported!
    /// Multiline, even.
    #[getset(get, set, get_mut, set_with)]
    private: T,
    /// Doc comments are supported!
    /// Multiline, even.
    #[getset(get_copy = "pub", set = "pub", get_mut = "pub", set_with = "pub")]
    public: T,
    /// Arc supported through CloneGetters
    #[getset(get_clone = "pub", set = "pub", get_mut = "pub", set_with = "pub")]
    arc: Arc<u16>,
}
impl<T> Foo<T>
where
    T: Copy + Clone + Default,
{
    /// Doc comments are supported!
    /// Multiline, even.
    #[inline(always)]
    fn private(&self) -> &T {
        &self.private
    }
}
impl<T> Foo<T>
where
    T: Copy + Clone + Default,
{
    /// Doc comments are supported!
    /// Multiline, even.
    #[inline(always)]
    fn set_private(&mut self, val: T) -> &mut Self {
        self.private = val;
        self
    }
    /// Doc comments are supported!
    /// Multiline, even.
    #[inline(always)]
    pub fn set_public(&mut self, val: T) -> &mut Self {
        self.public = val;
        self
    }
    /// Arc supported through CloneGetters
    #[inline(always)]
    pub fn set_arc(&mut self, val: Arc<u16>) -> &mut Self {
        self.arc = val;
        self
    }
}
impl<T> Foo<T>
where
    T: Copy + Clone + Default,
{
    /// Doc comments are supported!
    /// Multiline, even.
    #[inline(always)]
    fn with_private(mut self, val: T) -> Self {
        self.private = val;
        self
    }
    /// Doc comments are supported!
    /// Multiline, even.
    #[inline(always)]
    pub fn with_public(mut self, val: T) -> Self {
        self.public = val;
        self
    }
    /// Arc supported through CloneGetters
    #[inline(always)]
    pub fn with_arc(mut self, val: Arc<u16>) -> Self {
        self.arc = val;
        self
    }
}
impl<T> Foo<T>
where
    T: Copy + Clone + Default,
{
    /// Doc comments are supported!
    /// Multiline, even.
    #[inline(always)]
    fn private_mut(&mut self) -> &mut T {
        &mut self.private
    }
    /// Doc comments are supported!
    /// Multiline, even.
    #[inline(always)]
    pub fn public_mut(&mut self) -> &mut T {
        &mut self.public
    }
    /// Arc supported through CloneGetters
    #[inline(always)]
    pub fn arc_mut(&mut self) -> &mut Arc<u16> {
        &mut self.arc
    }
}
impl<T> Foo<T>
where
    T: Copy + Clone + Default,
{
    /// Doc comments are supported!
    /// Multiline, even.
    #[inline(always)]
    pub fn public(&self) -> T {
        self.public
    }
}
impl<T> Foo<T>
where
    T: Copy + Clone + Default,
{
    /// Arc supported through CloneGetters
    #[inline(always)]
    pub fn arc(&self) -> Arc<u16> {
        self.arc.clone()
    }
}
```

Attributes can be set on struct level for all fields in struct as well. Field level attributes take
precedence.

```rust
mod submodule {
    use getset::{Getters, MutGetters, CopyGetters, Setters, WithSetters};
    #[derive(Getters, CopyGetters, Default)]
    #[getset(get_copy = "pub")] // By default add a pub getting for all fields.
    pub struct Foo {
        public: i32,
        #[getset(get_copy)] // Override as private
        private: i32,
    }
    fn demo() {
        let mut foo = Foo::default();
        foo.private();
    }
}

let mut foo = submodule::Foo::default();
foo.public();
```

For some purposes, it's useful to have the `get_` prefix on the getters for
either legacy of compatibility reasons. It is done with `with_prefix`.

```rust
use getset::{Getters, MutGetters, CopyGetters, Setters, WithSetters};

#[derive(Getters, Default)]
pub struct Foo {
    #[getset(get = "pub with_prefix")]
    field: bool,
}


let mut foo = Foo::default();
let val = foo.get_field();
```

Skipping setters and getters generation for a field when struct level attribute is used
is possible with `#[getset(skip)]`.

```rust
use getset::{CopyGetters, Setters, WithSetters};

#[derive(CopyGetters, Setters, WithSetters)]
#[getset(get_copy, set, set_with)]
pub struct Foo {
    // If the field was not skipped, the compiler would complain about moving
    // a non-copyable type in copy getter.
    #[getset(skip)]
    skipped: String,

    field1: usize,
    field2: usize,
}

impl Foo {
    // It is possible to write getters and setters manually,
    // possibly with a custom logic.
    fn skipped(&self) -> &str {
        &self.skipped
    }

    fn set_skipped(&mut self, val: &str) -> &mut Self {
        self.skipped = val.to_string();
        self
    }

    fn with_skipped(mut self, val: &str) -> Self {
        self.skipped = val.to_string();
        self
    }
}
```

For a unary struct (a tuple struct with a single field),
the macro generates the `get`, `get_mut`, and `set` functions to
provide a getter, a mutable getter, and a setter, respectively.

```rust
use getset::{Getters, MutGetters, CopyGetters, Setters};

#[derive(Setters, Getters, MutGetters)]
struct UnaryTuple(#[getset(set, get, get_mut)] i32);

let mut tup = UnaryTuple(42);
assert_eq!(tup.get(), &42);
assert_eq!(tup.get_mut(), &mut 42);
tup.set(43);
assert_eq!(tup.get(), &43);

#[derive(CopyGetters)]
struct CopyUnaryTuple(#[getset(get_copy)] i32);

let tup = CopyUnaryTuple(42);
```
*/

#[macro_use]
extern crate quote;

use proc_macro::TokenStream;
use proc_macro2::TokenStream as TokenStream2;
use proc_macro_error2::{abort, abort_call_site, proc_macro_error};
use syn::{parse_macro_input, spanned::Spanned, DataStruct, DeriveInput, Meta};

use crate::generate::{GenMode, GenParams};

mod generate;

#[proc_macro_derive(Getters, attributes(get, with_prefix, getset))]
#[proc_macro_error]
pub fn getters(input: TokenStream) -> TokenStream {
    let ast = parse_macro_input!(input as DeriveInput);
    let params = GenParams {
        mode: GenMode::Get,
        global_attr: parse_global_attr(&ast.attrs, GenMode::Get),
    };

    produce(&ast, &params).into()
}

#[proc_macro_derive(CloneGetters, attributes(get_clone, with_prefix, getset))]
#[proc_macro_error]
pub fn clone_getters(input: TokenStream) -> TokenStream {
    let ast = parse_macro_input!(input as DeriveInput);
    let params = GenParams {
        mode: GenMode::GetClone,
        global_attr: parse_global_attr(&ast.attrs, GenMode::GetClone),
    };

    produce(&ast, &params).into()
}

#[proc_macro_derive(CopyGetters, attributes(get_copy, with_prefix, getset))]
#[proc_macro_error]
pub fn copy_getters(input: TokenStream) -> TokenStream {
    let ast = parse_macro_input!(input as DeriveInput);
    let params = GenParams {
        mode: GenMode::GetCopy,
        global_attr: parse_global_attr(&ast.attrs, GenMode::GetCopy),
    };

    produce(&ast, &params).into()
}

#[proc_macro_derive(MutGetters, attributes(get_mut, getset))]
#[proc_macro_error]
pub fn mut_getters(input: TokenStream) -> TokenStream {
    let ast = parse_macro_input!(input as DeriveInput);
    let params = GenParams {
        mode: GenMode::GetMut,
        global_attr: parse_global_attr(&ast.attrs, GenMode::GetMut),
    };

    produce(&ast, &params).into()
}

#[proc_macro_derive(Setters, attributes(set, getset))]
#[proc_macro_error]
pub fn setters(input: TokenStream) -> TokenStream {
    let ast = parse_macro_input!(input as DeriveInput);
    let params = GenParams {
        mode: GenMode::Set,
        global_attr: parse_global_attr(&ast.attrs, GenMode::Set),
    };

    produce(&ast, &params).into()
}

#[proc_macro_derive(WithSetters, attributes(set_with, getset))]
#[proc_macro_error]
pub fn with_setters(input: TokenStream) -> TokenStream {
    let ast = parse_macro_input!(input as DeriveInput);
    let params = GenParams {
        mode: GenMode::SetWith,
        global_attr: parse_global_attr(&ast.attrs, GenMode::SetWith),
    };

    produce(&ast, &params).into()
}

fn parse_global_attr(attrs: &[syn::Attribute], mode: GenMode) -> Option<Meta> {
    attrs.iter().filter_map(|v| parse_attr(v, mode)).next_back()
}

fn parse_attr(attr: &syn::Attribute, mode: GenMode) -> Option<syn::Meta> {
    use syn::{punctuated::Punctuated, Token};

    if attr.path().is_ident("getset") {
        let meta_list =
            match attr.parse_args_with(Punctuated::<syn::Meta, Token![,]>::parse_terminated) {
                Ok(list) => list,
                Err(e) => abort!(attr.span(), "Failed to parse getset attribute: {}", e),
            };

        let (last, skip, mut collected) = meta_list
            .into_iter()
            .inspect(|meta| {
                if !(meta.path().is_ident("get")
                    || meta.path().is_ident("get_clone")
                    || meta.path().is_ident("get_copy")
                    || meta.path().is_ident("get_mut")
                    || meta.path().is_ident("set")
                    || meta.path().is_ident("set_with")
                    || meta.path().is_ident("skip"))
                {
                    abort!(meta.path().span(), "unknown setter or getter")
                }
            })
            .fold(
                (None, None, Vec::new()),
                |(last, skip, mut collected), meta| {
                    if meta.path().is_ident(mode.name()) {
                        (Some(meta), skip, collected)
                    } else if meta.path().is_ident("skip") {
                        (last, Some(meta), collected)
                    } else {
                        collected.push(meta);
                        (last, skip, collected)
                    }
                },
            );

        if skip.is_some() {
            // Check if there is any setter or getter used with skip, which is
            // forbidden.
            if last.is_none() && collected.is_empty() {
                skip
            } else {
                abort!(
                    last.or_else(|| collected.pop()).unwrap().path().span(),
                    "use of setters and getters with skip is invalid"
                );
            }
        } else {
            last
        }
    } else if attr.path().is_ident(mode.name()) {
        // If skip is not used, return the last occurrence of matching
        // setter/getter, if there is any.
        attr.meta.clone().into()
    } else {
        None
    }
}

fn produce(ast: &DeriveInput, params: &GenParams) -> TokenStream2 {
    let name = &ast.ident;
    let generics = &ast.generics;
    let (impl_generics, ty_generics, where_clause) = generics.split_for_impl();

    // Is it a struct?
    if let syn::Data::Struct(DataStruct { ref fields, .. }) = ast.data {
        // Handle unary struct
        if matches!(fields, syn::Fields::Unnamed(_)) {
            if fields.len() != 1 {
                abort_call_site!("Only support unary struct!");
            }
            // This unwrap is safe because we know there is exactly one field
            let field = fields.iter().next().unwrap();
            let generated = generate::implement_for_unnamed(field, params);

            quote! {
                impl #impl_generics #name #ty_generics #where_clause {
                    #generated
                }
            }
        } else {
            let generated = fields.iter().map(|f| generate::implement(f, params));

            quote! {
                impl #impl_generics #name #ty_generics #where_clause {
                    #(#generated)*
                }
            }
        }
    } else {
        // Nope. This is an Enum. We cannot handle these!
        abort_call_site!("#[derive(Getters)] is only defined for structs, not for enums!");
    }
}
