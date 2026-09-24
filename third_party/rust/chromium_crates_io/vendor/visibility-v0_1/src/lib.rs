#![forbid(unsafe_code)]
#![allow(nonstandard_style, unused_imports)]
#![cfg_attr(feature = "nightly",
    cfg_attr(all(), doc = include_str!("../README.md")),
)]

extern crate proc_macro;

use ::proc_macro::TokenStream;
use ::proc_macro2::{Span, TokenStream as TokenStream2};
use ::quote::{
    quote,
    quote_spanned,
    ToTokens,
};
use ::syn::{*,
    parse::Parse,
    Result,
};

/// **Override**s the visibility of the annotated item with the one given to
/// this attribute:
///
/// ## Example
///
/// ```rust
/// mod module {
///     #[visibility::make(pub)]
///     fn foo () {}
/// }
///
/// module::foo();
/// ```
///
/// ## To be used in conjunction with `#[cfg_attr]`
///
/// That's where this attribute really shines: it is not possible to
/// conditionally modify the visibility of an item, but it is possible to
/// conditionally apply an attribute. If the attribute then modifies the
/// visibility of the decorated item, we have achieved our goal:
///
/// ```rust
/// mod module {
///     #[cfg_attr(all(/* feature = "integration-tests" */),
///         visibility::make(pub),
///     )]
///     fn foo () {}
/// }
///
/// module::foo();
/// ```
#[proc_macro_attribute] pub
fn make (attrs: TokenStream, input: TokenStream)
  -> TokenStream
{
    let visibility: Visibility = parse_macro_input!(attrs);
    let mut input: Item = parse_macro_input!(input);

    match input {
        | Item::Const(ItemConst { ref mut vis, .. }) => *vis = visibility,
        | Item::Enum(ItemEnum { ref mut vis, .. }) => *vis = visibility,
        | Item::ExternCrate(ItemExternCrate { ref mut vis, .. }) => *vis = visibility,
        | Item::Fn(ItemFn { ref mut vis, .. }) => *vis = visibility,
        // | Item::ForeignMod(ItemForeignMod { ref mut vis, .. }) => *vis = visibility,
        // | Item::Impl(ItemImpl { ref mut vis, .. }) => *vis = visibility,
        // | Item::Macro(ItemMacro { ref mut vis, .. }) => *vis = visibility,
        | Item::Mod(ItemMod { ref mut vis, .. }) => *vis = visibility,
        | Item::Static(ItemStatic { ref mut vis, .. }) => *vis = visibility,
        | Item::Struct(ItemStruct { ref mut vis, .. }) => *vis = visibility,
        | Item::Trait(ItemTrait { ref mut vis, .. }) => *vis = visibility,
        | Item::TraitAlias(ItemTraitAlias { ref mut vis, .. }) => *vis = visibility,
        | Item::Type(ItemType { ref mut vis, .. }) => *vis = visibility,
        | Item::Union(ItemUnion { ref mut vis, .. }) => *vis = visibility,
        | Item::Use(ItemUse { ref mut vis, .. }) => *vis = visibility,
        // | Item::Verbatim(TokenStream { ref mut vis, .. }) => *vis = visibility,

        | _ => return Error::new_spanned(
            &input,
            "Cannot override the `#[visibility]` of this item",
        ).to_compile_error().into(),
    }

    input.into_token_stream().into()
}
