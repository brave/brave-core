#![doc = include_str!("../README.md")]

use proc_macro::TokenStream;
use proc_macro2::TokenStream as TokenStream2;
use syn::spanned::Spanned;

macro_rules! unsupported_if_some {
    ($value:expr) => {
        if let Some(value) = $value {
            return syn::Error::new(value.span(), "unsupported by #[unsafe_target_feature(...)]")
                .into_compile_error()
                .into();
        }
    };
}

macro_rules! unsupported {
    ($value: expr) => {
        return syn::Error::new(
            $value.span(),
            "unsupported by #[unsafe_target_feature(...)]",
        )
        .into_compile_error()
        .into()
    };
}

mod kw {
    syn::custom_keyword!(conditional);
}

enum SpecializeArg {
    LitStr(syn::LitStr),
    Conditional(Conditional),
}

impl SpecializeArg {
    fn lit(&self) -> &syn::LitStr {
        match self {
            SpecializeArg::LitStr(lit) => lit,
            SpecializeArg::Conditional(conditional) => &conditional.lit,
        }
    }

    fn condition(&self) -> Option<&TokenStream2> {
        match self {
            SpecializeArg::LitStr(..) => None,
            SpecializeArg::Conditional(conditional) => Some(&conditional.attr),
        }
    }
}

struct Conditional {
    lit: syn::LitStr,
    attr: TokenStream2,
}

impl syn::parse::Parse for Conditional {
    fn parse(input: syn::parse::ParseStream) -> syn::Result<Self> {
        let lit = input.parse()?;
        input.parse::<syn::Token![,]>()?;
        let attr = input.parse()?;

        Ok(Conditional { lit, attr })
    }
}

impl syn::parse::Parse for SpecializeArg {
    fn parse(input: syn::parse::ParseStream) -> syn::Result<Self> {
        let lookahead = input.lookahead1();
        if lookahead.peek(kw::conditional) {
            input.parse::<kw::conditional>()?;

            let content;
            syn::parenthesized!(content in input);

            let conditional = content.parse()?;
            Ok(SpecializeArg::Conditional(conditional))
        } else {
            Ok(SpecializeArg::LitStr(input.parse()?))
        }
    }
}

struct SpecializeArgs(syn::punctuated::Punctuated<SpecializeArg, syn::Token![,]>);

impl syn::parse::Parse for SpecializeArgs {
    fn parse(input: syn::parse::ParseStream) -> syn::Result<Self> {
        Ok(Self(syn::punctuated::Punctuated::parse_terminated(input)?))
    }
}

#[proc_macro_attribute]
pub fn unsafe_target_feature(attributes: TokenStream, input: TokenStream) -> TokenStream {
    let attributes = syn::parse_macro_input!(attributes as syn::LitStr);
    let item = syn::parse_macro_input!(input as syn::Item);
    process_item(&attributes, item, true)
}

#[proc_macro_attribute]
pub fn unsafe_target_feature_specialize(
    attributes: TokenStream,
    input: TokenStream,
) -> TokenStream {
    let attributes = syn::parse_macro_input!(attributes as SpecializeArgs);
    let item_mod = syn::parse_macro_input!(input as syn::ItemMod);

    let mut out = Vec::new();
    for attributes in attributes.0 {
        let features: Vec<_> = attributes
            .lit()
            .value()
            .split(',')
            .map(|feature| feature.replace(' ', ""))
            .collect();
        let name = format!("{}_{}", item_mod.ident, features.join("_"));
        let ident = syn::Ident::new(&name, item_mod.ident.span());
        let mut attrs = item_mod.attrs.clone();
        if let Some(condition) = attributes.condition() {
            attrs.push(syn::Attribute {
                pound_token: Default::default(),
                style: syn::AttrStyle::Outer,
                bracket_token: Default::default(),
                meta: syn::Meta::List(syn::MetaList {
                    path: syn::Ident::new("cfg", attributes.lit().span()).into(),
                    delimiter: syn::MacroDelimiter::Paren(Default::default()),
                    tokens: condition.clone(),
                }),
            });
        }

        let item_mod = process_mod(
            attributes.lit(),
            syn::ItemMod {
                attrs,
                ident,
                ..item_mod.clone()
            },
            Some(features),
        );

        out.push(item_mod);
    }

    quote::quote! {
        #(#out)*
    }
    .into()
}

fn process_item(attributes: &syn::LitStr, item: syn::Item, strict: bool) -> TokenStream {
    match item {
        syn::Item::Fn(function) => process_function(attributes, function, None),
        syn::Item::Impl(item_impl) => process_impl(attributes, item_impl),
        syn::Item::Mod(item_mod) => process_mod(attributes, item_mod, None).into(),
        item => {
            if strict {
                unsupported!(item)
            } else {
                quote::quote! { #item }.into()
            }
        }
    }
}

fn process_mod(
    attributes: &syn::LitStr,
    mut item_mod: syn::ItemMod,
    spec_features: Option<Vec<String>>,
) -> TokenStream2 {
    if let Some((_, ref mut content)) = item_mod.content {
        'next_item: for item in content {
            if let Some(ref spec_features) = spec_features {
                match item {
                    syn::Item::Const(syn::ItemConst { ref mut attrs, .. })
                    | syn::Item::Enum(syn::ItemEnum { ref mut attrs, .. })
                    | syn::Item::ExternCrate(syn::ItemExternCrate { ref mut attrs, .. })
                    | syn::Item::Fn(syn::ItemFn { ref mut attrs, .. })
                    | syn::Item::ForeignMod(syn::ItemForeignMod { ref mut attrs, .. })
                    | syn::Item::Impl(syn::ItemImpl { ref mut attrs, .. })
                    | syn::Item::Macro(syn::ItemMacro { ref mut attrs, .. })
                    | syn::Item::Mod(syn::ItemMod { ref mut attrs, .. })
                    | syn::Item::Static(syn::ItemStatic { ref mut attrs, .. })
                    | syn::Item::Struct(syn::ItemStruct { ref mut attrs, .. })
                    | syn::Item::Trait(syn::ItemTrait { ref mut attrs, .. })
                    | syn::Item::TraitAlias(syn::ItemTraitAlias { ref mut attrs, .. })
                    | syn::Item::Type(syn::ItemType { ref mut attrs, .. })
                    | syn::Item::Union(syn::ItemUnion { ref mut attrs, .. })
                    | syn::Item::Use(syn::ItemUse { ref mut attrs, .. }) => {
                        let mut index = 0;
                        while index < attrs.len() {
                            let attr = &attrs[index];
                            if matches!(attr.style, syn::AttrStyle::Outer) {
                                match attr.meta {
                                    syn::Meta::List(ref list)
                                        if is_path_eq(&list.path, "for_target_feature") =>
                                    {
                                        let feature: syn::LitStr = match list.parse_args() {
                                            Ok(feature) => feature,
                                            Err(error) => {
                                                return error.into_compile_error();
                                            }
                                        };

                                        let feature = feature.value();
                                        if !spec_features
                                            .iter()
                                            .any(|enabled_feature| feature == *enabled_feature)
                                        {
                                            *item = syn::Item::Verbatim(Default::default());
                                            continue 'next_item;
                                        }

                                        attrs.remove(index);
                                        continue;
                                    }
                                    _ => {}
                                }
                            }

                            index += 1;
                            continue;
                        }
                    }
                    _ => {
                        unsupported!(item_mod);
                    }
                }
            }

            *item = syn::Item::Verbatim(
                process_item(
                    attributes,
                    std::mem::replace(item, syn::Item::Verbatim(Default::default())),
                    false,
                )
                .into(),
            );
        }
    }

    quote::quote! {
        #item_mod
    }
}

fn process_impl(attributes: &syn::LitStr, mut item_impl: syn::ItemImpl) -> TokenStream {
    unsupported_if_some!(item_impl.defaultness);
    unsupported_if_some!(item_impl.unsafety);

    let mut items = Vec::new();
    for item in item_impl.items.drain(..) {
        match item {
            syn::ImplItem::Fn(function) => {
                unsupported_if_some!(function.defaultness);
                let function = syn::ItemFn {
                    attrs: function.attrs,
                    vis: function.vis,
                    sig: function.sig,
                    block: Box::new(function.block),
                };
                let output_item = process_function(
                    attributes,
                    function,
                    Some((item_impl.generics.clone(), item_impl.self_ty.clone())),
                );
                items.push(syn::ImplItem::Verbatim(output_item.into()));
            }
            item => items.push(item),
        }
    }

    item_impl.items = items;
    quote::quote! {
        #item_impl
    }
    .into()
}

fn is_path_eq(path: &syn::Path, ident: &str) -> bool {
    let segments: Vec<_> = ident.split("::").collect();
    path.segments.len() == segments.len()
        && path
            .segments
            .iter()
            .zip(segments.iter())
            .all(|(segment, expected)| segment.ident == expected && segment.arguments.is_none())
}

fn process_function(
    attributes: &syn::LitStr,
    function: syn::ItemFn,
    outer: Option<(syn::Generics, Box<syn::Type>)>,
) -> TokenStream {
    if function.sig.unsafety.is_some() {
        return quote::quote! {
            #[target_feature(enable = #attributes)]
            #function
        }
        .into();
    }

    unsupported_if_some!(function.sig.constness);
    unsupported_if_some!(function.sig.asyncness);
    unsupported_if_some!(function.sig.abi);
    unsupported_if_some!(function.sig.variadic);

    let function_visibility = function.vis;
    let function_name = function.sig.ident;
    let function_return = function.sig.output;
    let function_inner_name =
        syn::Ident::new(&format!("_impl_{}", function_name), function_name.span());
    let function_args = function.sig.inputs;
    let function_body = function.block;
    let mut function_call_args = Vec::new();
    let mut function_args_outer = Vec::new();
    let mut function_args_inner = Vec::new();
    for (index, arg) in function_args.iter().enumerate() {
        match arg {
            syn::FnArg::Receiver(receiver) => {
                unsupported_if_some!(receiver.attrs.first());
                unsupported_if_some!(receiver.colon_token);

                if outer.is_none() {
                    return syn::Error::new(receiver.span(), "unsupported by #[unsafe_target_feature(...)]; put the attribute on the outer `impl`").into_compile_error().into();
                }

                function_args_inner.push(syn::FnArg::Receiver(receiver.clone()));
                function_args_outer.push(syn::FnArg::Receiver(receiver.clone()));
                function_call_args.push(syn::Ident::new("self", receiver.self_token.span()));
            }
            syn::FnArg::Typed(ty) => {
                unsupported_if_some!(ty.attrs.first());

                match &*ty.pat {
                    syn::Pat::Ident(pat_ident) => {
                        unsupported_if_some!(pat_ident.attrs.first());

                        function_args_inner.push(arg.clone());
                        function_args_outer.push(syn::FnArg::Typed(syn::PatType {
                            attrs: Vec::new(),
                            pat: Box::new(syn::Pat::Ident(syn::PatIdent {
                                attrs: Vec::new(),
                                by_ref: None,
                                mutability: None,
                                ident: pat_ident.ident.clone(),
                                subpat: None,
                            })),
                            colon_token: ty.colon_token,
                            ty: ty.ty.clone(),
                        }));
                        function_call_args.push(pat_ident.ident.clone());
                    }
                    syn::Pat::Wild(pat_wild) => {
                        unsupported_if_some!(pat_wild.attrs.first());

                        let ident = syn::Ident::new(
                            &format!("__arg_{}__", index),
                            pat_wild.underscore_token.span(),
                        );
                        function_args_inner.push(arg.clone());
                        function_args_outer.push(syn::FnArg::Typed(syn::PatType {
                            attrs: Vec::new(),
                            pat: Box::new(syn::Pat::Ident(syn::PatIdent {
                                attrs: Vec::new(),
                                by_ref: None,
                                mutability: None,
                                ident: ident.clone(),
                                subpat: None,
                            })),
                            colon_token: ty.colon_token,
                            ty: ty.ty.clone(),
                        }));
                        function_call_args.push(ident);
                    }
                    _ => unsupported!(arg),
                }
            }
        }
    }

    let mut maybe_inline = quote::quote! {};
    let mut maybe_outer_attributes = Vec::new();
    let mut maybe_cfg = quote::quote! {};
    for attribute in function.attrs {
        match &attribute.meta {
            syn::Meta::Path(path) if is_path_eq(path, "inline") => {
                maybe_inline = quote::quote! { #[inline] };
            }
            syn::Meta::Path(path) if is_path_eq(path, "test") => {
                maybe_outer_attributes.push(attribute);
                maybe_cfg = quote::quote! { #[cfg(target_feature = #attributes)] };
            }
            syn::Meta::List(syn::MetaList { path, tokens, .. })
                if is_path_eq(path, "inline") && tokens.to_string() == "always" =>
            {
                maybe_inline = quote::quote! { #[inline] };
            }
            syn::Meta::NameValue(syn::MetaNameValue { path, .. }) if is_path_eq(path, "doc") => {
                maybe_outer_attributes.push(attribute);
            }
            syn::Meta::List(syn::MetaList { path, .. })
                if is_path_eq(path, "cfg")
                    || is_path_eq(path, "allow")
                    || is_path_eq(path, "deny") =>
            {
                maybe_outer_attributes.push(attribute);
            }
            syn::Meta::Path(path) if is_path_eq(path, "rustfmt::skip") => {
                maybe_outer_attributes.push(attribute);
            }
            _ => unsupported!(attribute),
        }
    }

    let (fn_impl_generics, fn_ty_generics, fn_where_clause) =
        function.sig.generics.split_for_impl();
    let fn_call_generics = fn_ty_generics.as_turbofish();

    if let Some((generics, self_ty)) = outer {
        let (outer_impl_generics, outer_ty_generics, outer_where_clause) =
            generics.split_for_impl();
        let trait_ident =
            syn::Ident::new(&format!("__Impl_{}__", function_name), function_name.span());
        let item_trait = quote::quote! {
            #[allow(non_camel_case_types)]
            trait #trait_ident #outer_impl_generics #outer_where_clause {
                unsafe fn #function_inner_name #fn_impl_generics (#(#function_args_outer),*) #function_return #fn_where_clause;
            }
        };

        let item_trait_impl = quote::quote! {
            impl #outer_impl_generics #trait_ident #outer_ty_generics for #self_ty #outer_where_clause {
                #[target_feature(enable = #attributes)]
                #maybe_inline
                unsafe fn #function_inner_name #fn_impl_generics (#(#function_args_inner),*) #function_return #fn_where_clause #function_body
            }
        };

        quote::quote! {
            #[inline(always)]
            #(#maybe_outer_attributes)*
            #function_visibility fn #function_name #fn_impl_generics (#(#function_args_outer),*) #function_return #fn_where_clause {
                #item_trait
                #item_trait_impl
                unsafe {
                    <Self as #trait_ident #outer_ty_generics> ::#function_inner_name #fn_call_generics (#(#function_call_args),*)
                }
            }
        }.into()
    } else {
        quote::quote! {
            #[inline(always)]
            #maybe_cfg
            #(#maybe_outer_attributes)*
            #function_visibility fn #function_name #fn_impl_generics (#(#function_args_outer),*) #function_return #fn_where_clause {
                #[target_feature(enable = #attributes)]
                #maybe_inline
                unsafe fn #function_inner_name #fn_impl_generics (#(#function_args_inner),*) #function_return #fn_where_clause #function_body
                unsafe {
                    #function_inner_name #fn_call_generics (#(#function_call_args),*)
                }
            }
        }.into()
    }
}
