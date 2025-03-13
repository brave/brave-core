// Copyright 2019 The Fuchsia Authors
//
// Licensed under a BSD-style license <LICENSE-BSD>, Apache License, Version 2.0
// <LICENSE-APACHE or https://www.apache.org/licenses/LICENSE-2.0>, or the MIT
// license <LICENSE-MIT or https://opensource.org/licenses/MIT>, at your option.
// This file may not be copied, modified, or distributed except according to
// those terms.

//! Derive macros for [zerocopy]'s traits.
//!
//! [zerocopy]: https://docs.rs/zerocopy

// Sometimes we want to use lints which were added after our MSRV.
// `unknown_lints` is `warn` by default and we deny warnings in CI, so without
// this attribute, any unknown lint would cause a CI failure when testing with
// our MSRV.
#![allow(unknown_lints)]
#![deny(renamed_and_removed_lints)]
#![deny(clippy::all, clippy::missing_safety_doc, clippy::undocumented_unsafe_blocks)]
#![deny(
    rustdoc::bare_urls,
    rustdoc::broken_intra_doc_links,
    rustdoc::invalid_codeblock_attributes,
    rustdoc::invalid_html_tags,
    rustdoc::invalid_rust_codeblocks,
    rustdoc::missing_crate_level_docs,
    rustdoc::private_intra_doc_links
)]
#![recursion_limit = "128"]

mod r#enum;
mod ext;
#[cfg(test)]
mod output_tests;
mod repr;

use proc_macro2::{TokenStream, TokenTree};
use quote::ToTokens;

use {
    proc_macro2::Span,
    quote::quote,
    syn::{
        parse_quote, Data, DataEnum, DataStruct, DataUnion, DeriveInput, Error, Expr, ExprLit,
        ExprUnary, GenericParam, Ident, Lit, Path, Type, UnOp, WherePredicate,
    },
};

use {crate::ext::*, crate::repr::*};

// TODO(https://github.com/rust-lang/rust/issues/54140): Some errors could be
// made better if we could add multiple lines of error output like this:
//
// error: unsupported representation
//   --> enum.rs:28:8
//    |
// 28 | #[repr(transparent)]
//    |
// help: required by the derive of FromBytes
//
// Instead, we have more verbose error messages like "unsupported representation
// for deriving FromZeros, FromBytes, IntoBytes, or Unaligned on an enum"
//
// This will probably require Span::error
// (https://doc.rust-lang.org/nightly/proc_macro/struct.Span.html#method.error),
// which is currently unstable. Revisit this once it's stable.

/// Defines a derive function named `$outer` which parses its input
/// `TokenStream` as a `DeriveInput` and then invokes the `$inner` function.
///
/// Note that the separate `$outer` parameter is required - proc macro functions
/// are currently required to live at the crate root, and so the caller must
/// specify the name in order to avoid name collisions.
macro_rules! derive {
    ($trait:ident => $outer:ident => $inner:ident) => {
        #[proc_macro_derive($trait)]
        pub fn $outer(ts: proc_macro::TokenStream) -> proc_macro::TokenStream {
            let ast = syn::parse_macro_input!(ts as DeriveInput);
            $inner(&ast, Trait::$trait).into_ts().into()
        }
    };
}

trait IntoTokenStream {
    fn into_ts(self) -> TokenStream;
}

impl IntoTokenStream for TokenStream {
    fn into_ts(self) -> TokenStream {
        self
    }
}

impl IntoTokenStream for Result<TokenStream, Error> {
    fn into_ts(self) -> TokenStream {
        match self {
            Ok(ts) => ts,
            Err(err) => err.to_compile_error(),
        }
    }
}

derive!(KnownLayout => derive_known_layout => derive_known_layout_inner);
derive!(Immutable => derive_no_cell => derive_no_cell_inner);
derive!(TryFromBytes => derive_try_from_bytes => derive_try_from_bytes_inner);
derive!(FromZeros => derive_from_zeros => derive_from_zeros_inner);
derive!(FromBytes => derive_from_bytes => derive_from_bytes_inner);
derive!(IntoBytes => derive_into_bytes => derive_into_bytes_inner);
derive!(Unaligned => derive_unaligned => derive_unaligned_inner);
derive!(ByteHash => derive_hash => derive_hash_inner);
derive!(ByteEq => derive_eq => derive_eq_inner);

/// Deprecated: prefer [`FromZeros`] instead.
#[deprecated(since = "0.8.0", note = "`FromZeroes` was renamed to `FromZeros`")]
#[doc(hidden)]
#[proc_macro_derive(FromZeroes)]
pub fn derive_from_zeroes(ts: proc_macro::TokenStream) -> proc_macro::TokenStream {
    derive_from_zeros(ts)
}

/// Deprecated: prefer [`IntoBytes`] instead.
#[deprecated(since = "0.8.0", note = "`AsBytes` was renamed to `IntoBytes`")]
#[doc(hidden)]
#[proc_macro_derive(AsBytes)]
pub fn derive_as_bytes(ts: proc_macro::TokenStream) -> proc_macro::TokenStream {
    derive_into_bytes(ts)
}

fn derive_known_layout_inner(ast: &DeriveInput, _top_level: Trait) -> Result<TokenStream, Error> {
    let is_repr_c_struct = match &ast.data {
        Data::Struct(..) => {
            let repr = StructUnionRepr::from_attrs(&ast.attrs)?;
            if repr.is_c() {
                Some(repr)
            } else {
                None
            }
        }
        Data::Enum(..) | Data::Union(..) => None,
    };

    let fields = ast.data.fields();

    let (self_bounds, inner_extras, outer_extras) = if let (
        Some(repr),
        Some((trailing_field, leading_fields)),
    ) = (is_repr_c_struct, fields.split_last())
    {
        let (_vis, trailing_field_name, trailing_field_ty) = trailing_field;
        let leading_fields_tys = leading_fields.iter().map(|(_vis, _name, ty)| ty);

        let core_path = quote!(::zerocopy::util::macro_util::core_reexport);
        let repr_align = repr
            .get_align()
            .map(|align| {
                let align = align.t.get();
                quote!(#core_path::num::NonZeroUsize::new(#align as usize))
            })
            .unwrap_or_else(|| quote!(#core_path::option::Option::None));
        let repr_packed = repr
            .get_packed()
            .map(|packed| {
                let packed = packed.get();
                quote!(#core_path::num::NonZeroUsize::new(#packed as usize))
            })
            .unwrap_or_else(|| quote!(#core_path::option::Option::None));

        let make_methods = |trailing_field_ty| {
            quote! {
                // SAFETY:
                // - The returned pointer has the same address and provenance as
                //   `bytes`:
                //   - The recursive call to `raw_from_ptr_len` preserves both
                //     address and provenance.
                //   - The `as` cast preserves both address and provenance.
                //   - `NonNull::new_unchecked` preserves both address and
                //     provenance.
                // - If `Self` is a slice DST, the returned pointer encodes
                //   `elems` elements in the trailing slice:
                //   - This is true of the recursive call to `raw_from_ptr_len`.
                //   - `trailing.as_ptr() as *mut Self` preserves trailing slice
                //     element count [1].
                //   - `NonNull::new_unchecked` preserves trailing slice element
                //     count.
                //
                // [1] Per https://doc.rust-lang.org/reference/expressions/operator-expr.html#pointer-to-pointer-cast:
                //
                //   `*const T`` / `*mut T` can be cast to `*const U` / `*mut U`
                //   with the following behavior:
                //     ...
                //     - If `T` and `U` are both unsized, the pointer is also
                //       returned unchanged. In particular, the metadata is
                //       preserved exactly.
                //
                //       For instance, a cast from `*const [T]` to `*const [U]`
                //       preserves the number of elements. ... The same holds
                //       for str and any compound type whose unsized tail is a
                //       slice type, such as struct `Foo(i32, [u8])` or `(u64, Foo)`.
                #[inline(always)]
                fn raw_from_ptr_len(
                    bytes: ::zerocopy::util::macro_util::core_reexport::ptr::NonNull<u8>,
                    meta: Self::PointerMetadata,
                ) -> ::zerocopy::util::macro_util::core_reexport::ptr::NonNull<Self> {
                    use ::zerocopy::KnownLayout;
                    let trailing = <#trailing_field_ty as KnownLayout>::raw_from_ptr_len(bytes, meta);
                    let slf = trailing.as_ptr() as *mut Self;
                    // SAFETY: Constructed from `trailing`, which is non-null.
                    unsafe { ::zerocopy::util::macro_util::core_reexport::ptr::NonNull::new_unchecked(slf) }
                }

                #[inline(always)]
                fn pointer_to_metadata(ptr: *mut Self) -> Self::PointerMetadata {
                    <#trailing_field_ty>::pointer_to_metadata(ptr as *mut _)
                }
            }
        };

        let inner_extras = {
            let leading_fields_tys = leading_fields_tys.clone();
            let methods = make_methods(*trailing_field_ty);
            let (_, ty_generics, _) = ast.generics.split_for_impl();

            quote!(
                type PointerMetadata = <#trailing_field_ty as ::zerocopy::KnownLayout>::PointerMetadata;

                type MaybeUninit = __ZerocopyKnownLayoutMaybeUninit #ty_generics;

                // SAFETY: `LAYOUT` accurately describes the layout of `Self`.
                // The layout of `Self` is reflected using a sequence of
                // invocations of `DstLayout::{new_zst,extend,pad_to_align}`.
                // The documentation of these items vows that invocations in
                // this manner will acurately describe a type, so long as:
                //
                //  - that type is `repr(C)`,
                //  - its fields are enumerated in the order they appear,
                //  - the presence of `repr_align` and `repr_packed` are correctly accounted for.
                //
                // We respect all three of these preconditions here. This
                // expansion is only used if `is_repr_c_struct`, we enumerate
                // the fields in order, and we extract the values of `align(N)`
                // and `packed(N)`.
                const LAYOUT: ::zerocopy::DstLayout = {
                    use ::zerocopy::util::macro_util::core_reexport::num::NonZeroUsize;
                    use ::zerocopy::{DstLayout, KnownLayout};

                    let repr_align = #repr_align;
                    let repr_packed = #repr_packed;

                    DstLayout::new_zst(repr_align)
                        #(.extend(DstLayout::for_type::<#leading_fields_tys>(), repr_packed))*
                        .extend(<#trailing_field_ty as KnownLayout>::LAYOUT, repr_packed)
                        .pad_to_align()
                };

                #methods
            )
        };

        let outer_extras = {
            let ident = &ast.ident;
            let vis = &ast.vis;
            let params = &ast.generics.params;
            let (impl_generics, ty_generics, where_clause) = ast.generics.split_for_impl();

            let predicates = if let Some(where_clause) = where_clause {
                where_clause.predicates.clone()
            } else {
                Default::default()
            };

            // Generate a valid ident for a type-level handle to a field of a
            // given `name`.
            let field_index =
                |name| Ident::new(&format!("__Zerocopy_Field_{}", name), ident.span());

            let field_indices: Vec<_> =
                fields.iter().map(|(_vis, name, _ty)| field_index(name)).collect();

            // Define the collection of type-level field handles.
            let field_defs = field_indices.iter().zip(&fields).map(|(idx, (vis, _, _))| {
                quote! {
                    #[allow(non_camel_case_types)]
                    #vis struct #idx;
                }
            });

            let field_impls = field_indices.iter().zip(&fields).map(|(idx, (_, _, ty))| quote! {
                // SAFETY: `#ty` is the type of `#ident`'s field at `#idx`.
                unsafe impl #impl_generics ::zerocopy::util::macro_util::Field<#idx> for #ident #ty_generics
                where
                    #predicates
                {
                    type Type = #ty;
                }
            });

            let trailing_field_index = field_index(trailing_field_name);
            let leading_field_indices =
                leading_fields.iter().map(|(_vis, name, _ty)| field_index(name));

            let trailing_field_ty = quote! {
                <#ident #ty_generics as
                    ::zerocopy::util::macro_util::Field<#trailing_field_index>
                >::Type
            };

            let methods = make_methods(&parse_quote! {
                <#trailing_field_ty as ::zerocopy::KnownLayout>::MaybeUninit
            });

            quote! {
                #(#field_defs)*

                #(#field_impls)*

                // SAFETY: This has the same layout as the derive target type,
                // except that it admits uninit bytes. This is ensured by using
                // the same repr as the target type, and by using field types
                // which have the same layout as the target type's fields,
                // except that they admit uninit bytes. We indirect through
                // `Field` to ensure that occurrences of `Self` resolve to
                // `#ty`, not `__ZerocopyKnownLayoutMaybeUninit` (see #2116).
                #repr
                #[doc(hidden)]
                // Required on some rustc versions due to a lint that is only
                // triggered when `derive(KnownLayout)` is applied to `repr(C)`
                // structs that are generated by macros. See #2177 for details.
                #[allow(private_bounds)]
                #vis struct __ZerocopyKnownLayoutMaybeUninit<#params> (
                    #(::zerocopy::util::macro_util::core_reexport::mem::MaybeUninit<
                        <#ident #ty_generics as
                            ::zerocopy::util::macro_util::Field<#leading_field_indices>
                        >::Type
                    >,)*
                    // NOTE(#2302): We wrap in `ManuallyDrop` here in case the
                    // type we're operating on is both generic and
                    // `repr(packed)`. In that case, Rust needs to know that the
                    // type is *either* `Sized` or has a trivial `Drop`.
                    // `ManuallyDrop` has a trivial `Drop`, and so satisfies
                    // this requirement.
                    ::zerocopy::util::macro_util::core_reexport::mem::ManuallyDrop<
                        <#trailing_field_ty as ::zerocopy::KnownLayout>::MaybeUninit
                    >
                )
                where
                    #trailing_field_ty: ::zerocopy::KnownLayout,
                    #predicates;

                // SAFETY: We largely defer to the `KnownLayout` implementation on
                // the derive target type (both by using the same tokens, and by
                // deferring to impl via type-level indirection). This is sound,
                // since  `__ZerocopyKnownLayoutMaybeUninit` is guaranteed to
                // have the same layout as the derive target type, except that
                // `__ZerocopyKnownLayoutMaybeUninit` admits uninit bytes.
                unsafe impl #impl_generics ::zerocopy::KnownLayout for __ZerocopyKnownLayoutMaybeUninit #ty_generics
                where
                    #trailing_field_ty: ::zerocopy::KnownLayout,
                    #predicates
                {
                    #[allow(clippy::missing_inline_in_public_items)]
                    fn only_derive_is_allowed_to_implement_this_trait() {}

                    type PointerMetadata = <#ident #ty_generics as ::zerocopy::KnownLayout>::PointerMetadata;

                    type MaybeUninit = Self;

                    const LAYOUT: ::zerocopy::DstLayout = <#ident #ty_generics as ::zerocopy::KnownLayout>::LAYOUT;

                    #methods
                }
            }
        };

        (SelfBounds::None, inner_extras, Some(outer_extras))
    } else {
        // For enums, unions, and non-`repr(C)` structs, we require that
        // `Self` is sized, and as a result don't need to reason about the
        // internals of the type.
        (
            SelfBounds::SIZED,
            quote!(
                type PointerMetadata = ();
                type MaybeUninit =
                    ::zerocopy::util::macro_util::core_reexport::mem::MaybeUninit<Self>;

                // SAFETY: `LAYOUT` is guaranteed to accurately describe the
                // layout of `Self`, because that is the documented safety
                // contract of `DstLayout::for_type`.
                const LAYOUT: ::zerocopy::DstLayout = ::zerocopy::DstLayout::for_type::<Self>();

                // SAFETY: `.cast` preserves address and provenance.
                //
                // TODO(#429): Add documentation to `.cast` that promises that
                // it preserves provenance.
                #[inline(always)]
                fn raw_from_ptr_len(
                    bytes: ::zerocopy::util::macro_util::core_reexport::ptr::NonNull<u8>,
                    _meta: (),
                ) -> ::zerocopy::util::macro_util::core_reexport::ptr::NonNull<Self>
                {
                    bytes.cast::<Self>()
                }

                #[inline(always)]
                fn pointer_to_metadata(_ptr: *mut Self) -> () {}
            ),
            None,
        )
    };

    Ok(match &ast.data {
        Data::Struct(strct) => {
            let require_trait_bound_on_field_types = if self_bounds == SelfBounds::SIZED {
                FieldBounds::None
            } else {
                FieldBounds::TRAILING_SELF
            };

            // A bound on the trailing field is required, since structs are
            // unsized if their trailing field is unsized. Reflecting the layout
            // of an usized trailing field requires that the field is
            // `KnownLayout`.
            impl_block(
                ast,
                strct,
                Trait::KnownLayout,
                require_trait_bound_on_field_types,
                self_bounds,
                None,
                Some(inner_extras),
                outer_extras,
            )
        }
        Data::Enum(enm) => {
            // A bound on the trailing field is not required, since enums cannot
            // currently be unsized.
            impl_block(
                ast,
                enm,
                Trait::KnownLayout,
                FieldBounds::None,
                SelfBounds::SIZED,
                None,
                Some(inner_extras),
                outer_extras,
            )
        }
        Data::Union(unn) => {
            // A bound on the trailing field is not required, since unions
            // cannot currently be unsized.
            impl_block(
                ast,
                unn,
                Trait::KnownLayout,
                FieldBounds::None,
                SelfBounds::SIZED,
                None,
                Some(inner_extras),
                outer_extras,
            )
        }
    })
}

fn derive_no_cell_inner(ast: &DeriveInput, _top_level: Trait) -> TokenStream {
    match &ast.data {
        Data::Struct(strct) => impl_block(
            ast,
            strct,
            Trait::Immutable,
            FieldBounds::ALL_SELF,
            SelfBounds::None,
            None,
            None,
            None,
        ),
        Data::Enum(enm) => impl_block(
            ast,
            enm,
            Trait::Immutable,
            FieldBounds::ALL_SELF,
            SelfBounds::None,
            None,
            None,
            None,
        ),
        Data::Union(unn) => impl_block(
            ast,
            unn,
            Trait::Immutable,
            FieldBounds::ALL_SELF,
            SelfBounds::None,
            None,
            None,
            None,
        ),
    }
}

fn derive_try_from_bytes_inner(ast: &DeriveInput, top_level: Trait) -> Result<TokenStream, Error> {
    match &ast.data {
        Data::Struct(strct) => derive_try_from_bytes_struct(ast, strct, top_level),
        Data::Enum(enm) => derive_try_from_bytes_enum(ast, enm, top_level),
        Data::Union(unn) => Ok(derive_try_from_bytes_union(ast, unn, top_level)),
    }
}

fn derive_from_zeros_inner(ast: &DeriveInput, top_level: Trait) -> Result<TokenStream, Error> {
    let try_from_bytes = derive_try_from_bytes_inner(ast, top_level)?;
    let from_zeros = match &ast.data {
        Data::Struct(strct) => derive_from_zeros_struct(ast, strct),
        Data::Enum(enm) => derive_from_zeros_enum(ast, enm)?,
        Data::Union(unn) => derive_from_zeros_union(ast, unn),
    };
    Ok(IntoIterator::into_iter([try_from_bytes, from_zeros]).collect())
}

fn derive_from_bytes_inner(ast: &DeriveInput, top_level: Trait) -> Result<TokenStream, Error> {
    let from_zeros = derive_from_zeros_inner(ast, top_level)?;
    let from_bytes = match &ast.data {
        Data::Struct(strct) => derive_from_bytes_struct(ast, strct),
        Data::Enum(enm) => derive_from_bytes_enum(ast, enm)?,
        Data::Union(unn) => derive_from_bytes_union(ast, unn),
    };

    Ok(IntoIterator::into_iter([from_zeros, from_bytes]).collect())
}

fn derive_into_bytes_inner(ast: &DeriveInput, _top_level: Trait) -> Result<TokenStream, Error> {
    match &ast.data {
        Data::Struct(strct) => derive_into_bytes_struct(ast, strct),
        Data::Enum(enm) => derive_into_bytes_enum(ast, enm),
        Data::Union(unn) => derive_into_bytes_union(ast, unn),
    }
}

fn derive_unaligned_inner(ast: &DeriveInput, _top_level: Trait) -> Result<TokenStream, Error> {
    match &ast.data {
        Data::Struct(strct) => derive_unaligned_struct(ast, strct),
        Data::Enum(enm) => derive_unaligned_enum(ast, enm),
        Data::Union(unn) => derive_unaligned_union(ast, unn),
    }
}

fn derive_hash_inner(ast: &DeriveInput, _top_level: Trait) -> Result<TokenStream, Error> {
    // This doesn't delegate to `impl_block` because `impl_block` assumes it is deriving a
    // `zerocopy`-defined trait, and these trait impls share a common shape that `Hash` does not.
    // In particular, `zerocopy` traits contain a method that only `zerocopy_derive` macros
    // are supposed to implement, and `impl_block` generating this trait method is incompatible
    // with `Hash`.
    let type_ident = &ast.ident;
    let (impl_generics, ty_generics, where_clause) = ast.generics.split_for_impl();
    let where_predicates = where_clause.map(|clause| &clause.predicates);
    Ok(quote! {
        // TODO(#553): Add a test that generates a warning when
        // `#[allow(deprecated)]` isn't present.
        #[allow(deprecated)]
        // While there are not currently any warnings that this suppresses (that
        // we're aware of), it's good future-proofing hygiene.
        #[automatically_derived]
        impl #impl_generics ::zerocopy::util::macro_util::core_reexport::hash::Hash for #type_ident #ty_generics
        where
            Self: ::zerocopy::IntoBytes + ::zerocopy::Immutable,
            #where_predicates
        {
            fn hash<H>(&self, state: &mut H)
            where
                H: ::zerocopy::util::macro_util::core_reexport::hash::Hasher,
            {
                ::zerocopy::util::macro_util::core_reexport::hash::Hasher::write(
                    state,
                    ::zerocopy::IntoBytes::as_bytes(self)
                )
            }

            fn hash_slice<H>(data: &[Self], state: &mut H)
            where
                H: ::zerocopy::util::macro_util::core_reexport::hash::Hasher,
            {
                ::zerocopy::util::macro_util::core_reexport::hash::Hasher::write(
                    state,
                    ::zerocopy::IntoBytes::as_bytes(data)
                )
            }
        }
    })
}

fn derive_eq_inner(ast: &DeriveInput, _top_level: Trait) -> Result<TokenStream, Error> {
    // This doesn't delegate to `impl_block` because `impl_block` assumes it is deriving a
    // `zerocopy`-defined trait, and these trait impls share a common shape that `Eq` does not.
    // In particular, `zerocopy` traits contain a method that only `zerocopy_derive` macros
    // are supposed to implement, and `impl_block` generating this trait method is incompatible
    // with `Eq`.
    let type_ident = &ast.ident;
    let (impl_generics, ty_generics, where_clause) = ast.generics.split_for_impl();
    let where_predicates = where_clause.map(|clause| &clause.predicates);
    Ok(quote! {
        // TODO(#553): Add a test that generates a warning when
        // `#[allow(deprecated)]` isn't present.
        #[allow(deprecated)]
        // While there are not currently any warnings that this suppresses (that
        // we're aware of), it's good future-proofing hygiene.
        #[automatically_derived]
        impl #impl_generics ::zerocopy::util::macro_util::core_reexport::cmp::PartialEq for #type_ident #ty_generics
        where
            Self: ::zerocopy::IntoBytes + ::zerocopy::Immutable,
            #where_predicates
        {
            fn eq(&self, other: &Self) -> bool {
                ::zerocopy::util::macro_util::core_reexport::cmp::PartialEq::eq(
                    ::zerocopy::IntoBytes::as_bytes(self),
                    ::zerocopy::IntoBytes::as_bytes(other),
                )
            }
        }

        // TODO(#553): Add a test that generates a warning when
        // `#[allow(deprecated)]` isn't present.
        #[allow(deprecated)]
        // While there are not currently any warnings that this suppresses (that
        // we're aware of), it's good future-proofing hygiene.
        #[automatically_derived]
        impl #impl_generics ::zerocopy::util::macro_util::core_reexport::cmp::Eq for #type_ident #ty_generics
        where
            Self: ::zerocopy::IntoBytes + ::zerocopy::Immutable,
            #where_predicates
        {
        }
    })
}

/// A struct is `TryFromBytes` if:
/// - all fields are `TryFromBytes`
fn derive_try_from_bytes_struct(
    ast: &DeriveInput,
    strct: &DataStruct,
    top_level: Trait,
) -> Result<TokenStream, Error> {
    let extras = try_gen_trivial_is_bit_valid(ast, top_level).unwrap_or_else(|| {
        let fields = strct.fields();
        let field_names = fields.iter().map(|(_vis, name, _ty)| name);
        let field_tys = fields.iter().map(|(_vis, _name, ty)| ty);
        quote!(
            // SAFETY: We use `is_bit_valid` to validate that each field is
            // bit-valid, and only return `true` if all of them are. The bit
            // validity of a struct is just the composition of the bit
            // validities of its fields, so this is a sound implementation of
            // `is_bit_valid`.
            fn is_bit_valid<___ZerocopyAliasing>(
                mut candidate: ::zerocopy::Maybe<Self, ___ZerocopyAliasing>,
            ) -> ::zerocopy::util::macro_util::core_reexport::primitive::bool
            where
                ___ZerocopyAliasing: ::zerocopy::pointer::invariant::Reference,
            {
                true #(&& {
                    // SAFETY:
                    // - `project` is a field projection, and so it addresses a
                    //   subset of the bytes addressed by `slf`
                    // - ..., and so it preserves provenance
                    // - ..., and `*slf` is a struct, so `UnsafeCell`s exist at
                    //   the same byte ranges in the returned pointer's referent
                    //   as they do in `*slf`
                    let field_candidate = unsafe {
                        let project = |slf: *mut Self|
                            ::zerocopy::util::macro_util::core_reexport::ptr::addr_of_mut!((*slf).#field_names);

                        candidate.reborrow().cast_unsized_unchecked(project)
                    };

                    <#field_tys as ::zerocopy::TryFromBytes>::is_bit_valid(field_candidate)
                })*
            }
        )
    });
    Ok(impl_block(
        ast,
        strct,
        Trait::TryFromBytes,
        FieldBounds::ALL_SELF,
        SelfBounds::None,
        None,
        Some(extras),
        None,
    ))
}

/// A union is `TryFromBytes` if:
/// - all of its fields are `TryFromBytes` and `Immutable`
fn derive_try_from_bytes_union(
    ast: &DeriveInput,
    unn: &DataUnion,
    top_level: Trait,
) -> TokenStream {
    // TODO(#5): Remove the `Immutable` bound.
    let field_type_trait_bounds =
        FieldBounds::All(&[TraitBound::Slf, TraitBound::Other(Trait::Immutable)]);
    let extras = try_gen_trivial_is_bit_valid(ast, top_level).unwrap_or_else(|| {
        let fields = unn.fields();
        let field_names = fields.iter().map(|(_vis, name, _ty)| name);
        let field_tys = fields.iter().map(|(_vis, _name, ty)| ty);
        quote!(
            // SAFETY: We use `is_bit_valid` to validate that any field is
            // bit-valid; we only return `true` if at least one of them is. The
            // bit validity of a union is not yet well defined in Rust, but it
            // is guaranteed to be no more strict than this definition. See #696
            // for a more in-depth discussion.
            fn is_bit_valid<___ZerocopyAliasing>(
                mut candidate: ::zerocopy::Maybe<'_, Self,___ZerocopyAliasing>
            ) -> ::zerocopy::util::macro_util::core_reexport::primitive::bool
            where
                ___ZerocopyAliasing: ::zerocopy::pointer::invariant::Reference,
            {
                false #(|| {
                    // SAFETY:
                    // - `project` is a field projection, and so it addresses a
                    //   subset of the bytes addressed by `slf`
                    // - ..., and so it preserves provenance
                    // - Since `Self: Immutable` is enforced by
                    //   `self_type_trait_bounds`, neither `*slf` nor the
                    //   returned pointer's referent contain any `UnsafeCell`s
                    let field_candidate = unsafe {
                        let project = |slf: *mut Self|
                            ::zerocopy::util::macro_util::core_reexport::ptr::addr_of_mut!((*slf).#field_names);

                        candidate.reborrow().cast_unsized_unchecked(project)
                    };

                    <#field_tys as ::zerocopy::TryFromBytes>::is_bit_valid(field_candidate)
                })*
            }
        )
    });
    impl_block(
        ast,
        unn,
        Trait::TryFromBytes,
        field_type_trait_bounds,
        SelfBounds::None,
        None,
        Some(extras),
        None,
    )
}

fn derive_try_from_bytes_enum(
    ast: &DeriveInput,
    enm: &DataEnum,
    top_level: Trait,
) -> Result<TokenStream, Error> {
    let repr = EnumRepr::from_attrs(&ast.attrs)?;

    // If an enum has no fields, it has a well-defined integer representation,
    // and every possible bit pattern corresponds to a valid discriminant tag,
    // then it *could* be `FromBytes` (even if the user hasn't derived
    // `FromBytes`). This holds if, for `repr(uN)` or `repr(iN)`, there are 2^N
    // variants.
    let could_be_from_bytes = enum_size_from_repr(&repr)
        .map(|size| enm.fields().is_empty() && enm.variants.len() == 1usize << size)
        .unwrap_or(false);

    let trivial_is_bit_valid = try_gen_trivial_is_bit_valid(ast, top_level);
    let extra = match (trivial_is_bit_valid, could_be_from_bytes) {
        (Some(is_bit_valid), _) => is_bit_valid,
        // SAFETY: It would be sound for the enum to implement `FomBytes`, as
        // required by `gen_trivial_is_bit_valid_unchecked`.
        (None, true) => unsafe { gen_trivial_is_bit_valid_unchecked() },
        (None, false) => r#enum::derive_is_bit_valid(&ast.ident, &repr, &ast.generics, enm)?,
    };

    Ok(impl_block(
        ast,
        enm,
        Trait::TryFromBytes,
        FieldBounds::ALL_SELF,
        SelfBounds::None,
        None,
        Some(extra),
        None,
    ))
}

/// Attempts to generate a `TryFromBytes::is_bit_valid` instance that
/// unconditionally returns true.
///
/// This is possible when the `top_level` trait is `FromBytes` and there are no
/// generic type parameters. In this case, we know that compilation will succeed
/// only if the type is unconditionally `FromBytes`. Type parameters are not
/// supported because a type with type parameters could be `TryFromBytes` but
/// not `FromBytes` depending on its type parameters, and so deriving a trivial
/// `is_bit_valid` would be either unsound or, assuming we add a defensive
/// `Self: FromBytes` bound (as we currently do), overly restrictive. Consider,
/// for example, that `Foo<bool>` ought to be `TryFromBytes` but not `FromBytes`
/// in this example:
///
/// ```rust,ignore
/// #[derive(FromBytes)]
/// #[repr(transparent)]
/// struct Foo<T>(T);
/// ```
///
/// This should be used where possible. Using this impl is faster to codegen,
/// faster to compile, and is friendlier on the optimizer.
fn try_gen_trivial_is_bit_valid(
    ast: &DeriveInput,
    top_level: Trait,
) -> Option<proc_macro2::TokenStream> {
    // If the top-level trait is `FromBytes` and `Self` has no type parameters,
    // then the `FromBytes` derive will fail compilation if `Self` is not
    // actually soundly `FromBytes`, and so we can rely on that for our
    // `is_bit_valid` impl. It's plausible that we could make changes - or Rust
    // could make changes (such as the "trivial bounds" language feature) - that
    // make this no longer true. To hedge against these, we include an explicit
    // `Self: FromBytes` check in the generated `is_bit_valid`, which is
    // bulletproof.
    if top_level == Trait::FromBytes && ast.generics.params.is_empty() {
        Some(quote!(
            // SAFETY: See inline.
            fn is_bit_valid<___ZerocopyAliasing>(
                _candidate: ::zerocopy::Maybe<Self, ___ZerocopyAliasing>,
            ) -> ::zerocopy::util::macro_util::core_reexport::primitive::bool
            where
                ___ZerocopyAliasing: ::zerocopy::pointer::invariant::Reference,
            {
                if false {
                    fn assert_is_from_bytes<T>()
                    where
                        T: ::zerocopy::FromBytes,
                        T: ?::zerocopy::util::macro_util::core_reexport::marker::Sized,
                    {
                    }

                    assert_is_from_bytes::<Self>();
                }

                // SAFETY: The preceding code only compiles if `Self:
                // FromBytes`. Thus, this code only compiles if all initialized
                // byte sequences represent valid instances of `Self`.
                true
            }
        ))
    } else {
        None
    }
}

/// Generates a `TryFromBytes::is_bit_valid` instance that unconditionally
/// returns true.
///
/// This should be used where possible, (although `try_gen_trivial_is_bit_valid`
/// should be preferred over this for safety reasons). Using this impl is faster
/// to codegen, faster to compile, and is friendlier on the optimizer.
///
/// # Safety
///
/// The caller must ensure that all initialized bit patterns are valid for
/// `Self`.
unsafe fn gen_trivial_is_bit_valid_unchecked() -> proc_macro2::TokenStream {
    quote!(
        // SAFETY: The caller of `gen_trivial_is_bit_valid_unchecked` has
        // promised that all initialized bit patterns are valid for `Self`.
        fn is_bit_valid<___ZerocopyAliasing>(
            _candidate: ::zerocopy::Maybe<Self, ___ZerocopyAliasing>,
        ) -> ::zerocopy::util::macro_util::core_reexport::primitive::bool
        where
            ___ZerocopyAliasing: ::zerocopy::pointer::invariant::Reference,
        {
            true
        }
    )
}

/// A struct is `FromZeros` if:
/// - all fields are `FromZeros`
fn derive_from_zeros_struct(ast: &DeriveInput, strct: &DataStruct) -> TokenStream {
    impl_block(
        ast,
        strct,
        Trait::FromZeros,
        FieldBounds::ALL_SELF,
        SelfBounds::None,
        None,
        None,
        None,
    )
}

/// Returns `Ok(index)` if variant `index` of the enum has a discriminant of
/// zero. If `Err(bool)` is returned, the boolean is true if the enum has
/// unknown discriminants (e.g. discriminants set to const expressions which we
/// can't evaluate in a proc macro). If the enum has unknown discriminants, then
/// it might have a zero variant that we just can't detect.
fn find_zero_variant(enm: &DataEnum) -> Result<usize, bool> {
    // Discriminants can be anywhere in the range [i128::MIN, u128::MAX] because
    // the discriminant type may be signed or unsigned. Since we only care about
    // tracking the discriminant when it's less than or equal to zero, we can
    // avoid u128 -> i128 conversions and bounds checking by making the "next
    // discriminant" value implicitly negative.
    // Technically 64 bits is enough, but 128 is better for future compatibility
    // with https://github.com/rust-lang/rust/issues/56071
    let mut next_negative_discriminant = Some(0);

    // Sometimes we encounter explicit discriminants that we can't know the
    // value of (e.g. a constant expression that requires evaluation). These
    // could evaluate to zero or a negative number, but we can't assume that
    // they do (no false positives allowed!). So we treat them like strictly-
    // positive values that can't result in any zero variants, and track whether
    // we've encountered any unknown discriminants.
    let mut has_unknown_discriminants = false;

    for (i, v) in enm.variants.iter().enumerate() {
        match v.discriminant.as_ref() {
            // Implicit discriminant
            None => {
                match next_negative_discriminant.as_mut() {
                    Some(0) => return Ok(i),
                    // n is nonzero so subtraction is always safe
                    Some(n) => *n -= 1,
                    None => (),
                }
            }
            // Explicit positive discriminant
            Some((_, Expr::Lit(ExprLit { lit: Lit::Int(int), .. }))) => {
                match int.base10_parse::<u128>().ok() {
                    Some(0) => return Ok(i),
                    Some(_) => next_negative_discriminant = None,
                    None => {
                        // Numbers should never fail to parse, but just in case:
                        has_unknown_discriminants = true;
                        next_negative_discriminant = None;
                    }
                }
            }
            // Explicit negative discriminant
            Some((_, Expr::Unary(ExprUnary { op: UnOp::Neg(_), expr, .. }))) => match &**expr {
                Expr::Lit(ExprLit { lit: Lit::Int(int), .. }) => {
                    match int.base10_parse::<u128>().ok() {
                        Some(0) => return Ok(i),
                        // x is nonzero so subtraction is always safe
                        Some(x) => next_negative_discriminant = Some(x - 1),
                        None => {
                            // Numbers should never fail to parse, but just in
                            // case:
                            has_unknown_discriminants = true;
                            next_negative_discriminant = None;
                        }
                    }
                }
                // Unknown negative discriminant (e.g. const repr)
                _ => {
                    has_unknown_discriminants = true;
                    next_negative_discriminant = None;
                }
            },
            // Unknown discriminant (e.g. const expr)
            _ => {
                has_unknown_discriminants = true;
                next_negative_discriminant = None;
            }
        }
    }

    Err(has_unknown_discriminants)
}

/// An enum is `FromZeros` if:
/// - one of the variants has a discriminant of `0`
/// - that variant's fields are all `FromZeros`
fn derive_from_zeros_enum(ast: &DeriveInput, enm: &DataEnum) -> Result<TokenStream, Error> {
    let repr = EnumRepr::from_attrs(&ast.attrs)?;

    // We don't actually care what the repr is; we just care that it's one of
    // the allowed ones.
    match repr {
         Repr::Compound(
            Spanned { t: CompoundRepr::C | CompoundRepr::Primitive(_), span: _ },
            _,
        ) => {}
        Repr::Transparent(_)
        | Repr::Compound(Spanned { t: CompoundRepr::Rust, span: _ }, _) => return Err(Error::new(Span::call_site(), "must have #[repr(C)] or #[repr(Int)] attribute in order to guarantee this type's memory layout")),
    }

    let zero_variant = match find_zero_variant(enm) {
        Ok(index) => enm.variants.iter().nth(index).unwrap(),
        // Has unknown variants
        Err(true) => {
            return Err(Error::new_spanned(
                ast,
                "FromZeros only supported on enums with a variant that has a discriminant of `0`\n\
                help: This enum has discriminants which are not literal integers. One of those may \
                define or imply which variant has a discriminant of zero. Use a literal integer to \
                define or imply the variant with a discriminant of zero.",
            ));
        }
        // Does not have unknown variants
        Err(false) => {
            return Err(Error::new_spanned(
                ast,
                "FromZeros only supported on enums with a variant that has a discriminant of `0`",
            ));
        }
    };

    let explicit_bounds = zero_variant
        .fields
        .iter()
        .map(|field| {
            let ty = &field.ty;
            parse_quote! { #ty: ::zerocopy::FromZeros }
        })
        .collect::<Vec<WherePredicate>>();

    Ok(impl_block(
        ast,
        enm,
        Trait::FromZeros,
        FieldBounds::Explicit(explicit_bounds),
        SelfBounds::None,
        None,
        None,
        None,
    ))
}

/// Unions are `FromZeros` if
/// - all fields are `FromZeros` and `Immutable`
fn derive_from_zeros_union(ast: &DeriveInput, unn: &DataUnion) -> TokenStream {
    // TODO(#5): Remove the `Immutable` bound. It's only necessary for
    // compatibility with `derive(TryFromBytes)` on unions; not for soundness.
    let field_type_trait_bounds =
        FieldBounds::All(&[TraitBound::Slf, TraitBound::Other(Trait::Immutable)]);
    impl_block(
        ast,
        unn,
        Trait::FromZeros,
        field_type_trait_bounds,
        SelfBounds::None,
        None,
        None,
        None,
    )
}

/// A struct is `FromBytes` if:
/// - all fields are `FromBytes`
fn derive_from_bytes_struct(ast: &DeriveInput, strct: &DataStruct) -> TokenStream {
    impl_block(
        ast,
        strct,
        Trait::FromBytes,
        FieldBounds::ALL_SELF,
        SelfBounds::None,
        None,
        None,
        None,
    )
}

/// An enum is `FromBytes` if:
/// - Every possible bit pattern must be valid, which means that every bit
///   pattern must correspond to a different enum variant. Thus, for an enum
///   whose layout takes up N bytes, there must be 2^N variants.
/// - Since we must know N, only representations which guarantee the layout's
///   size are allowed. These are `repr(uN)` and `repr(iN)` (`repr(C)` implies an
///   implementation-defined size). `usize` and `isize` technically guarantee the
///   layout's size, but would require us to know how large those are on the
///   target platform. This isn't terribly difficult - we could emit a const
///   expression that could call `core::mem::size_of` in order to determine the
///   size and check against the number of enum variants, but a) this would be
///   platform-specific and, b) even on Rust's smallest bit width platform (32),
///   this would require ~4 billion enum variants, which obviously isn't a thing.
/// - All fields of all variants are `FromBytes`.
fn derive_from_bytes_enum(ast: &DeriveInput, enm: &DataEnum) -> Result<TokenStream, Error> {
    let repr = EnumRepr::from_attrs(&ast.attrs)?;

    let variants_required = 1usize << enum_size_from_repr(&repr)?;
    if enm.variants.len() != variants_required {
        return Err(Error::new_spanned(
            ast,
            format!(
                "FromBytes only supported on {} enum with {} variants",
                repr.repr_type_name(),
                variants_required
            ),
        ));
    }

    Ok(impl_block(
        ast,
        enm,
        Trait::FromBytes,
        FieldBounds::ALL_SELF,
        SelfBounds::None,
        None,
        None,
        None,
    ))
}

// Returns `None` if the enum's size is not guaranteed by the repr.
fn enum_size_from_repr(repr: &EnumRepr) -> Result<usize, Error> {
    use {CompoundRepr::*, PrimitiveRepr::*, Repr::*};
    match repr {
        Transparent(span)
        | Compound(
            Spanned { t: C | Rust | Primitive(U32 | I32 | U64 | I64 | Usize | Isize), span },
            _,
        ) => Err(Error::new(*span, "`FromBytes` only supported on enums with `#[repr(...)]` attributes `u8`, `i8`, `u16`, or `i16`")),
        Compound(Spanned { t: Primitive(U8 | I8), span: _ }, _align) => Ok(8),
        Compound(Spanned { t: Primitive(U16 | I16), span: _ }, _align) => Ok(16),
    }
}

/// Unions are `FromBytes` if
/// - all fields are `FromBytes` and `Immutable`
fn derive_from_bytes_union(ast: &DeriveInput, unn: &DataUnion) -> TokenStream {
    // TODO(#5): Remove the `Immutable` bound. It's only necessary for
    // compatibility with `derive(TryFromBytes)` on unions; not for soundness.
    let field_type_trait_bounds =
        FieldBounds::All(&[TraitBound::Slf, TraitBound::Other(Trait::Immutable)]);
    impl_block(
        ast,
        unn,
        Trait::FromBytes,
        field_type_trait_bounds,
        SelfBounds::None,
        None,
        None,
        None,
    )
}

fn derive_into_bytes_struct(ast: &DeriveInput, strct: &DataStruct) -> Result<TokenStream, Error> {
    let repr = StructUnionRepr::from_attrs(&ast.attrs)?;

    let is_transparent = repr.is_transparent();
    let is_c = repr.is_c();
    let is_packed_1 = repr.is_packed_1();
    let num_fields = strct.fields().len();

    let (padding_check, require_unaligned_fields) = if is_transparent || is_packed_1 {
        // No padding check needed.
        // - repr(transparent): The layout and ABI of the whole struct is the
        //   same as its only non-ZST field (meaning there's no padding outside
        //   of that field) and we require that field to be `IntoBytes` (meaning
        //   there's no padding in that field).
        // - repr(packed): Any inter-field padding bytes are removed, meaning
        //   that any padding bytes would need to come from the fields, all of
        //   which we require to be `IntoBytes` (meaning they don't have any
        //   padding). Note that this holds regardless of other `repr`
        //   attributes, including `repr(Rust)`. [1]
        //
        // [1] Per https://doc.rust-lang.org/1.81.0/reference/type-layout.html#the-alignment-modifiers:
        //
        //   An important consequence of these rules is that a type with
        //   `#[repr(packed(1))]`` (or `#[repr(packed)]``) will have no
        //   inter-field padding.
        (None, false)
    } else if is_c && !repr.is_align_gt_1() && num_fields <= 1 {
        // No padding check needed. A repr(C) struct with zero or one field has
        // no padding unless #[repr(align)] explicitly adds padding, which we
        // check for in this branch's condition.
        (None, false)
    } else if ast.generics.params.is_empty() {
        // Since there are no generics, we can emit a padding check. All reprs
        // guarantee that fields won't overlap [1], so the padding check is
        // sound. This is more permissive than the next case, which requires
        // that all field types implement `Unaligned`.
        //
        // [1] Per https://doc.rust-lang.org/1.81.0/reference/type-layout.html#the-rust-representation:
        //
        //   The only data layout guarantees made by [`repr(Rust)`] are those
        //   required for soundness. They are:
        //   ...
        //   2. The fields do not overlap.
        //   ...
        (Some(PaddingCheck::Struct), false)
    } else if is_c && !repr.is_align_gt_1() {
        // We can't use a padding check since there are generic type arguments.
        // Instead, we require all field types to implement `Unaligned`. This
        // ensures that the `repr(C)` layout algorithm will not insert any
        // padding unless #[repr(align)] explicitly adds padding, which we check
        // for in this branch's condition.
        //
        // TODO(#10): Support type parameters for non-transparent, non-packed
        // structs without requiring `Unaligned`.
        (None, true)
    } else {
        return Err(Error::new(Span::call_site(), "must have a non-align #[repr(...)] attribute in order to guarantee this type's memory layout"));
    };

    let field_bounds = if require_unaligned_fields {
        FieldBounds::All(&[TraitBound::Slf, TraitBound::Other(Trait::Unaligned)])
    } else {
        FieldBounds::ALL_SELF
    };

    Ok(impl_block(
        ast,
        strct,
        Trait::IntoBytes,
        field_bounds,
        SelfBounds::None,
        padding_check,
        None,
        None,
    ))
}

/// If the type is an enum:
/// - It must have a defined representation (`repr`s `C`, `u8`, `u16`, `u32`,
///   `u64`, `usize`, `i8`, `i16`, `i32`, `i64`, or `isize`).
/// - It must have no padding bytes.
/// - Its fields must be `IntoBytes`.
fn derive_into_bytes_enum(ast: &DeriveInput, enm: &DataEnum) -> Result<TokenStream, Error> {
    let repr = EnumRepr::from_attrs(&ast.attrs)?;
    if !repr.is_c() && !repr.is_primitive() {
        return Err(Error::new(Span::call_site(), "must have #[repr(C)] or #[repr(Int)] attribute in order to guarantee this type's memory layout"));
    }

    let tag_type_definition = r#enum::generate_tag_enum(&repr, enm);
    Ok(impl_block(
        ast,
        enm,
        Trait::IntoBytes,
        FieldBounds::ALL_SELF,
        SelfBounds::None,
        Some(PaddingCheck::Enum { tag_type_definition }),
        None,
        None,
    ))
}

/// A union is `IntoBytes` if:
/// - all fields are `IntoBytes`
/// - `repr(C)`, `repr(transparent)`, or `repr(packed)`
/// - no padding (size of union equals size of each field type)
fn derive_into_bytes_union(ast: &DeriveInput, unn: &DataUnion) -> Result<TokenStream, Error> {
    // See #1792 for more context.
    //
    // By checking for `zerocopy_derive_union_into_bytes` both here and in the
    // generated code, we ensure that `--cfg zerocopy_derive_union_into_bytes`
    // need only be passed *either* when compiling this crate *or* when
    // compiling the user's crate. The former is preferable, but in some
    // situations (such as when cross-compiling using `cargo build --target`),
    // it doesn't get propagated to this crate's build by default.
    let cfg_compile_error = if cfg!(zerocopy_derive_union_into_bytes) {
        quote!()
    } else {
        quote!(
            const _: () = {
                #[cfg(not(zerocopy_derive_union_into_bytes))]
                ::zerocopy::util::macro_util::core_reexport::compile_error!(
                    "requires --cfg zerocopy_derive_union_into_bytes;
please let us know you use this feature: https://github.com/google/zerocopy/discussions/1802"
                );
            };
        )
    };

    // TODO(#10): Support type parameters.
    if !ast.generics.params.is_empty() {
        return Err(Error::new(Span::call_site(), "unsupported on types with type parameters"));
    }

    // Because we don't support generics, we don't need to worry about
    // special-casing different reprs. So long as there is *some* repr which
    // guarantees the layout, our `PaddingCheck::Union` guarantees that there is
    // no padding.
    let repr = StructUnionRepr::from_attrs(&ast.attrs)?;
    if !repr.is_c() && !repr.is_transparent() && !repr.is_packed_1() {
        return Err(Error::new(
            Span::call_site(),
            "must be #[repr(C)], #[repr(packed)], or #[repr(transparent)]",
        ));
    }

    let impl_block = impl_block(
        ast,
        unn,
        Trait::IntoBytes,
        FieldBounds::ALL_SELF,
        SelfBounds::None,
        Some(PaddingCheck::Union),
        None,
        None,
    );
    Ok(quote!(#cfg_compile_error #impl_block))
}

/// A struct is `Unaligned` if:
/// - `repr(align)` is no more than 1 and either
///   - `repr(C)` or `repr(transparent)` and
///     - all fields `Unaligned`
///   - `repr(packed)`
fn derive_unaligned_struct(ast: &DeriveInput, strct: &DataStruct) -> Result<TokenStream, Error> {
    let repr = StructUnionRepr::from_attrs(&ast.attrs)?;
    repr.unaligned_validate_no_align_gt_1()?;

    let field_bounds = if repr.is_packed_1() {
        FieldBounds::None
    } else if repr.is_c() || repr.is_transparent() {
        FieldBounds::ALL_SELF
    } else {
        return Err(Error::new(Span::call_site(), "must have #[repr(C)], #[repr(transparent)], or #[repr(packed)] attribute in order to guarantee this type's alignment"));
    };

    Ok(impl_block(ast, strct, Trait::Unaligned, field_bounds, SelfBounds::None, None, None, None))
}

/// An enum is `Unaligned` if:
/// - No `repr(align(N > 1))`
/// - `repr(u8)` or `repr(i8)`
fn derive_unaligned_enum(ast: &DeriveInput, enm: &DataEnum) -> Result<TokenStream, Error> {
    let repr = EnumRepr::from_attrs(&ast.attrs)?;
    repr.unaligned_validate_no_align_gt_1()?;

    if !repr.is_u8() && !repr.is_i8() {
        return Err(Error::new(Span::call_site(), "must have #[repr(u8)] or #[repr(i8)] attribute in order to guarantee this type's alignment"));
    }

    Ok(impl_block(
        ast,
        enm,
        Trait::Unaligned,
        FieldBounds::ALL_SELF,
        SelfBounds::None,
        None,
        None,
        None,
    ))
}

/// Like structs, a union is `Unaligned` if:
/// - `repr(align)` is no more than 1 and either
///   - `repr(C)` or `repr(transparent)` and
///     - all fields `Unaligned`
///   - `repr(packed)`
fn derive_unaligned_union(ast: &DeriveInput, unn: &DataUnion) -> Result<TokenStream, Error> {
    let repr = StructUnionRepr::from_attrs(&ast.attrs)?;
    repr.unaligned_validate_no_align_gt_1()?;

    let field_type_trait_bounds = if repr.is_packed_1() {
        FieldBounds::None
    } else if repr.is_c() || repr.is_transparent() {
        FieldBounds::ALL_SELF
    } else {
        return Err(Error::new(Span::call_site(), "must have #[repr(C)], #[repr(transparent)], or #[repr(packed)] attribute in order to guarantee this type's alignment"));
    };

    Ok(impl_block(
        ast,
        unn,
        Trait::Unaligned,
        field_type_trait_bounds,
        SelfBounds::None,
        None,
        None,
        None,
    ))
}

/// This enum describes what kind of padding check needs to be generated for the
/// associated impl.
enum PaddingCheck {
    /// Check that the sum of the fields' sizes exactly equals the struct's
    /// size.
    Struct,
    /// Check that the size of each field exactly equals the union's size.
    Union,
    /// Check that every variant of the enum contains no padding.
    ///
    /// Because doing so requires a tag enum, this padding check requires an
    /// additional `TokenStream` which defines the tag enum as `___ZerocopyTag`.
    Enum { tag_type_definition: TokenStream },
}

impl PaddingCheck {
    /// Returns the ident of the macro to call in order to validate that a type
    /// passes the padding check encoded by `PaddingCheck`.
    fn validator_macro_ident(&self) -> Ident {
        let s = match self {
            PaddingCheck::Struct => "struct_has_padding",
            PaddingCheck::Union => "union_has_padding",
            PaddingCheck::Enum { .. } => "enum_has_padding",
        };

        Ident::new(s, Span::call_site())
    }

    /// Sometimes performing the padding check requires some additional
    /// "context" code. For enums, this is the definition of the tag enum.
    fn validator_macro_context(&self) -> Option<&TokenStream> {
        match self {
            PaddingCheck::Struct | PaddingCheck::Union => None,
            PaddingCheck::Enum { tag_type_definition } => Some(tag_type_definition),
        }
    }
}

#[derive(Copy, Clone, Debug, Eq, PartialEq)]
enum Trait {
    KnownLayout,
    Immutable,
    TryFromBytes,
    FromZeros,
    FromBytes,
    IntoBytes,
    Unaligned,
    Sized,
    ByteHash,
    ByteEq,
}

impl ToTokens for Trait {
    fn to_tokens(&self, tokens: &mut TokenStream) {
        // According to [1], the format of the derived `Debug`` output is not
        // stable and therefore not guaranteed to represent the variant names.
        // Indeed with the (unstable) `fmt-debug` compiler flag [2], it can
        // return only a minimalized output or empty string. To make sure this
        // code will work in the future and independet of the compiler flag, we
        // translate the variants to their names manually here.
        //
        // [1] https://doc.rust-lang.org/1.81.0/std/fmt/trait.Debug.html#stability
        // [2] https://doc.rust-lang.org/beta/unstable-book/compiler-flags/fmt-debug.html
        let s = match self {
            Trait::KnownLayout => "KnownLayout",
            Trait::Immutable => "Immutable",
            Trait::TryFromBytes => "TryFromBytes",
            Trait::FromZeros => "FromZeros",
            Trait::FromBytes => "FromBytes",
            Trait::IntoBytes => "IntoBytes",
            Trait::Unaligned => "Unaligned",
            Trait::Sized => "Sized",
            Trait::ByteHash => "ByteHash",
            Trait::ByteEq => "ByteEq",
        };
        let ident = Ident::new(s, Span::call_site());
        tokens.extend(core::iter::once(TokenTree::Ident(ident)));
    }
}

impl Trait {
    fn crate_path(&self) -> Path {
        match self {
            Self::Sized => parse_quote!(::zerocopy::util::macro_util::core_reexport::marker::#self),
            _ => parse_quote!(::zerocopy::#self),
        }
    }
}

#[derive(Debug, Eq, PartialEq)]
enum TraitBound {
    Slf,
    Other(Trait),
}

enum FieldBounds<'a> {
    None,
    All(&'a [TraitBound]),
    Trailing(&'a [TraitBound]),
    Explicit(Vec<WherePredicate>),
}

impl<'a> FieldBounds<'a> {
    const ALL_SELF: FieldBounds<'a> = FieldBounds::All(&[TraitBound::Slf]);
    const TRAILING_SELF: FieldBounds<'a> = FieldBounds::Trailing(&[TraitBound::Slf]);
}

#[derive(Debug, Eq, PartialEq)]
enum SelfBounds<'a> {
    None,
    All(&'a [Trait]),
}

// TODO(https://github.com/rust-lang/rust-clippy/issues/12908): This is a false positive.
// Explicit lifetimes are actually necessary here.
#[allow(clippy::needless_lifetimes)]
impl<'a> SelfBounds<'a> {
    const SIZED: Self = Self::All(&[Trait::Sized]);
}

/// Normalizes a slice of bounds by replacing [`TraitBound::Slf`] with `slf`.
fn normalize_bounds(slf: Trait, bounds: &[TraitBound]) -> impl '_ + Iterator<Item = Trait> {
    bounds.iter().map(move |bound| match bound {
        TraitBound::Slf => slf,
        TraitBound::Other(trt) => *trt,
    })
}

#[allow(clippy::too_many_arguments)]
fn impl_block<D: DataExt>(
    input: &DeriveInput,
    data: &D,
    trt: Trait,
    field_type_trait_bounds: FieldBounds,
    self_type_trait_bounds: SelfBounds,
    padding_check: Option<PaddingCheck>,
    inner_extras: Option<TokenStream>,
    outer_extras: Option<TokenStream>,
) -> TokenStream {
    // In this documentation, we will refer to this hypothetical struct:
    //
    //   #[derive(FromBytes)]
    //   struct Foo<T, I: Iterator>
    //   where
    //       T: Copy,
    //       I: Clone,
    //       I::Item: Clone,
    //   {
    //       a: u8,
    //       b: T,
    //       c: I::Item,
    //   }
    //
    // We extract the field types, which in this case are `u8`, `T`, and
    // `I::Item`. We re-use the existing parameters and where clauses. If
    // `require_trait_bound == true` (as it is for `FromBytes), we add where
    // bounds for each field's type:
    //
    //   impl<T, I: Iterator> FromBytes for Foo<T, I>
    //   where
    //       T: Copy,
    //       I: Clone,
    //       I::Item: Clone,
    //       T: FromBytes,
    //       I::Item: FromBytes,
    //   {
    //   }
    //
    // NOTE: It is standard practice to only emit bounds for the type parameters
    // themselves, not for field types based on those parameters (e.g., `T` vs
    // `T::Foo`). For a discussion of why this is standard practice, see
    // https://github.com/rust-lang/rust/issues/26925.
    //
    // The reason we diverge from this standard is that doing it that way for us
    // would be unsound. E.g., consider a type, `T` where `T: FromBytes` but
    // `T::Foo: !FromBytes`. It would not be sound for us to accept a type with
    // a `T::Foo` field as `FromBytes` simply because `T: FromBytes`.
    //
    // While there's no getting around this requirement for us, it does have the
    // pretty serious downside that, when lifetimes are involved, the trait
    // solver ties itself in knots:
    //
    //     #[derive(Unaligned)]
    //     #[repr(C)]
    //     struct Dup<'a, 'b> {
    //         a: PhantomData<&'a u8>,
    //         b: PhantomData<&'b u8>,
    //     }
    //
    //     error[E0283]: type annotations required: cannot resolve `core::marker::PhantomData<&'a u8>: zerocopy::Unaligned`
    //      --> src/main.rs:6:10
    //       |
    //     6 | #[derive(Unaligned)]
    //       |          ^^^^^^^^^
    //       |
    //       = note: required by `zerocopy::Unaligned`

    let type_ident = &input.ident;
    let trait_path = trt.crate_path();
    let fields = data.fields();
    let variants = data.variants();
    let tag = data.tag();

    fn bound_tt(ty: &Type, traits: impl Iterator<Item = Trait>) -> WherePredicate {
        let traits = traits.map(|t| t.crate_path());
        parse_quote!(#ty: #(#traits)+*)
    }
    let field_type_bounds: Vec<_> = match (field_type_trait_bounds, &fields[..]) {
        (FieldBounds::All(traits), _) => fields
            .iter()
            .map(|(_vis, _name, ty)| bound_tt(ty, normalize_bounds(trt, traits)))
            .collect(),
        (FieldBounds::None, _) | (FieldBounds::Trailing(..), []) => vec![],
        (FieldBounds::Trailing(traits), [.., last]) => {
            vec![bound_tt(last.2, normalize_bounds(trt, traits))]
        }
        (FieldBounds::Explicit(bounds), _) => bounds,
    };

    // Don't bother emitting a padding check if there are no fields.
    #[allow(unstable_name_collisions)] // See `BoolExt` below
    // Work around https://github.com/rust-lang/rust-clippy/issues/12280
    #[allow(clippy::incompatible_msrv)]
    let padding_check_bound =
        padding_check.and_then(|check| (!fields.is_empty()).then_some(check)).map(|check| {
            let variant_types = variants.iter().map(|var| {
                let types = var.iter().map(|(_vis, _name, ty)| ty);
                quote!([#(#types),*])
            });
            let validator_context = check.validator_macro_context();
            let validator_macro = check.validator_macro_ident();
            let t = tag.iter();
            parse_quote! {
                (): ::zerocopy::util::macro_util::PaddingFree<
                    Self,
                    {
                        #validator_context
                        ::zerocopy::#validator_macro!(Self, #(#t,)* #(#variant_types),*)
                    }
                >
            }
        });

    let self_bounds: Option<WherePredicate> = match self_type_trait_bounds {
        SelfBounds::None => None,
        SelfBounds::All(traits) => Some(bound_tt(&parse_quote!(Self), traits.iter().copied())),
    };

    let bounds = input
        .generics
        .where_clause
        .as_ref()
        .map(|where_clause| where_clause.predicates.iter())
        .into_iter()
        .flatten()
        .chain(field_type_bounds.iter())
        .chain(padding_check_bound.iter())
        .chain(self_bounds.iter());

    // The parameters with trait bounds, but without type defaults.
    let params = input.generics.params.clone().into_iter().map(|mut param| {
        match &mut param {
            GenericParam::Type(ty) => ty.default = None,
            GenericParam::Const(cnst) => cnst.default = None,
            GenericParam::Lifetime(_) => {}
        }
        quote!(#param)
    });

    // The identifiers of the parameters without trait bounds or type defaults.
    let param_idents = input.generics.params.iter().map(|param| match param {
        GenericParam::Type(ty) => {
            let ident = &ty.ident;
            quote!(#ident)
        }
        GenericParam::Lifetime(l) => {
            let ident = &l.lifetime;
            quote!(#ident)
        }
        GenericParam::Const(cnst) => {
            let ident = &cnst.ident;
            quote!({#ident})
        }
    });

    let impl_tokens = quote! {
        // TODO(#553): Add a test that generates a warning when
        // `#[allow(deprecated)]` isn't present.
        #[allow(deprecated)]
        // While there are not currently any warnings that this suppresses (that
        // we're aware of), it's good future-proofing hygiene.
        #[automatically_derived]
        unsafe impl < #(#params),* > #trait_path for #type_ident < #(#param_idents),* >
        where
            #(#bounds,)*
        {
            fn only_derive_is_allowed_to_implement_this_trait() {}

            #inner_extras
        }
    };

    if let Some(outer_extras) = outer_extras {
        // So that any items defined in `#outer_extras` don't conflict with
        // existing names defined in this scope.
        quote! {
            const _: () = {
                #impl_tokens

                #outer_extras
            };
        }
    } else {
        impl_tokens
    }
}

// A polyfill for `Option::then_some`, which was added after our MSRV.
//
// The `#[allow(unused)]` is necessary because, on sufficiently recent toolchain
// versions, `b.then_some(...)` resolves to the inherent method rather than to
// this trait, and so this trait is considered unused.
//
// TODO(#67): Remove this once our MSRV is >= 1.62.
#[allow(unused)]
trait BoolExt {
    fn then_some<T>(self, t: T) -> Option<T>;
}

impl BoolExt for bool {
    fn then_some<T>(self, t: T) -> Option<T> {
        if self {
            Some(t)
        } else {
            None
        }
    }
}
