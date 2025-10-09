/*!
[apply]: apply
[derive]: derive
[`derive_alias!`]: derive_alias
[`macro_rules_attribute`]: macro_rules_attribute
[`macro_rules_derive`]: macro_rules_derive
*/
#![cfg_attr(feature = "better-docs",
    cfg_attr(all(), doc = include_str!("../README.md"))
)]
#![cfg_attr(feature = "better-docs",
    feature(doc_auto_cfg),
)]
#![no_std]
#![forbid(unsafe_code)]

/// Legacy name for what is currently named <code>#\[[apply]]</code>
///
/// Despite being a slightly clearer name (than `#[apply]` is) w.r.t. what it
/// does, `#[macro_rules_attribute]` had the big drawback of being a mouthful.
///
/// Hence the `#[apply]` alias being born, and now even superseding
/// `#[macro_rules_attribute]` altogether as the author-deemed "idiomatic"
/// name to favor.
pub use ::macro_rules_attribute_proc_macro::macro_rules_attribute;

/// Applies the given `macro_rules!` macro to the decorated item.
///
/// This, as with any `proc_macro_attribute`, **consumes** the item it
/// decorates: it is the `macro_rules!` macro job to generate it (_it is thus
/// able to modify it_!).
///
/// For a version with "read-only" access to the item it decorates, see
/// [`macro_rules_derive`][`macro@macro_rules_derive`].
///
/// ## Examples
///
/// ### Deriving getters for a (non-generic) `struct`
///
/// Imagine having define the following handy `make_getters!` (`macro_rules!`)
/// macro:
///
/** ```rust
macro_rules! make_getters {(
    $(#[$struct_meta:meta])*
    $struct_vis:vis
    struct $StructName:ident {
        $(
            $(#[$field_meta:meta])*
            $field_vis:vis // this visibility will be applied to the getters instead
            $field_name:ident : $field_ty:ty
        ),* $(,)?
    }
) => (
    // First, generate the struct definition we have been given, but with
    // private fields instead.
    $(#[$struct_meta])*
    $struct_vis
    struct $StructName {
        $(
            $(#[$field_meta])*
            // notice the lack of visibility => private fields
            $field_name: $field_ty,
        )*
    }

    // Then, implement the getters:
    impl $StructName {
        $(
            #[inline]
            $field_vis
            fn $field_name (self: &'_ Self)
              -> &'_ $field_ty
            {
                &self.$field_name
            }
        )*
    }
)}
``` */
///
/// Basically allowing you to write:
///
/** ```rust ,compile_fail
use example::Person;
mod example {
    make_getters! {
        /// The macro handles meta attributes such as docstrings
        pub
        struct Person {
            pub
            name: String,

            pub
            age: u8,
        }
    }
}

fn is_new_born (person: &'_ mut Person)
  -> bool
{
    // Reading the value through the getter is fine…
    return *person.age() == 0;
    // But trying to mutate it by skipping the getter is not 💪
    person.age = 0;
 // ^ error[E0616]: field `age` of struct `example::Person` is private
}
``` */
///
/// This is fine, _etc._, but that rightward drift on `make_getters! {` syntax
/// problematic:
///
///   - Incurs in extra rightward drift and thus, noise.
///
///   - Worse, **it leads to a non-escalable / composable pattern**: if we had a
///     second macro, say `make_setters!`, our syntax is unable to handle both
///     macros being called on the same type definition.
///
/// Hence `::macro_rules_attribute`'s <code>#\[[apply]\]</code> (formerly called
/// `#[macro_rules_attribute]` itself) helper:
///
/** ```rust
# fn main () {}
#[macro_use]
extern crate macro_rules_attribute;

use example::Person;
mod example {
    #[apply(make_getters!)] // or `#[apply(make_getters)]`: the final `!` is not mandatory
    /// The macro handles meta attributes such as docstrings
    pub
    struct Person {
        pub
        name: String,

        pub
        age: u8,
    }
    # // where;
    # macro_rules! make_getters {(
    #     $(#[$struct_meta:meta])*
    #     $struct_vis:vis
    #     struct $StructName:ident {
    #         $(
    #             $(#[$field_meta:meta])*
    #             $field_vis:vis // this visibility will be applied to the getters instead
    #             $field_name:ident : $field_ty:ty
    #         ),* $(,)?
    #     }
    # ) => (
    #     // First, generate the struct definition we have been given, but with
    #     // private fields instead.
    #     $(#[$struct_meta])*
    #     $struct_vis
    #     struct $StructName {
    #         $(
    #             $(#[$field_meta])*
    #             // notice the lack of visibility => private fields
    #             $field_name: $field_ty,
    #         )*
    #     }

    #     // Then, implement the getters:
    #     impl $StructName {
    #         $(
    #             #[inline]
    #             $field_vis
    #             fn $field_name (self: &'_ Self)
    #                 -> &'_ $field_ty
    #             {
    #                 &self.$field_name
    #             }
    #         )*
    #     }
    # )} use make_getters;
}

fn is_new_born (person: &'_ Person)
  -> bool
{
    // Reading the value through the getter is fine…
    *person.age() == 0
    // But trying to mutate it by skipping the getter is not 💪
    // person.age == 0
    // ^ error[E0616]: field `age` of struct `example::Person` is private
}
``` */
pub use ::macro_rules_attribute_proc_macro::macro_rules_attribute as apply;

/// Applies the given `macro_rules!` macro to the decorated item.
///
/// This, as with any `#[derive(...)]`, **does not consume** the item it
/// decorates: instead, it only generates code on top of it.
///
/// Also derives [`Custom`] to allow using `#[custom(...)]` and `#[derive_args(...)]`
/// as derive helpers.
///
/// ## Examples
///
/// ### Implementing `Into<Int>` for a given `#[repr(Int)]` `enum`:
///
/** ```rust
#[macro_use]
extern crate macro_rules_attribute;

macro_rules! ToInteger {(
    #[repr($Int:ident)]
    $(#[$enum_meta:meta])*
    $pub:vis
    enum $Enum:ident {
        $(
            $Variant:ident $(= $value:expr)?
        ),* $(,)?
    }
) => (
    impl ::core::convert::From<$Enum> for $Int {
        #[inline]
        fn from (x: $Enum)
          -> Self
        {
            x as _
        }
    }
)}

#[macro_rules_derive(ToInteger)] // or `#[macro_rules_derive(ToInteger!)]`
#[repr(u32)]
enum Bool {
    False,
    True,
}

fn main ()
{
    assert_eq!(u32::from(Bool::False), 0);
    assert_eq!(u32::from(Bool::True), 1);
    // assert_eq!(u8::from(Bool::False), 0);
    // ^ error[E0277]: the trait bound `u8: std::convert::From<main::Bool>` is not satisfied
}
``` */
///
/// ## Difference with <code>#\[[derive]\]</code>
///
/// <code>#\[[macro_rules_derive]\]</code> is specifically intended to be used
/// with `macro_rules!`-based derives:
///
///   - it won't accept classic derives;
///
///   - thanks to that, the trailing `!` on the macro name is not mandatory.
///
/// For <code>#\[[derive]\]</code>, it's exactly the opposite.
pub use ::macro_rules_attribute_proc_macro::macro_rules_derive;

/// Convenience macro to define new derive aliases.
///
/// The so-defined macros are intended to be used by
/// <code>#\[[macro_rules_derive]]</code> or this crate's
/// <code>#\[[derive]]</code>.
///
/// ## Examples
///
/** ```rust
# fn main () {}
#[macro_use]
extern crate macro_rules_attribute;

derive_alias! {
    #[derive(Copy!)] = #[derive(Clone, Copy)];
    #[derive(Eq!)] = #[derive(PartialEq, Eq)];
    #[derive(Ord!)] = #[derive(Eq!, PartialOrd, Ord)];
}

#[derive(Debug, Copy!, Ord!)]
struct Foo {
    // …
}

// Note: this defines `Copy!`, `Eq!` and `Ord!` as properly scoped
// `crate`-local macros.
mod example {
    use super::Copy;

    #[derive(Copy!, super::Eq!)]
    struct Bar;
}
``` */
///
/** ```rust
# fn main () {}
#[macro_use]
extern crate macro_rules_attribute;

use ::core::{fmt::Debug, hash::Hash};

/// Trait alias pattern: `T : TheUsualSuspects ⇔ T : Debug + Copy + Ord + Hash`.
trait TheUsualSuspects
where // `⇒` direction
    Self : Debug + Copy + Ord + Hash,
{}
impl<T : ?Sized> TheUsualSuspects for T
where // `⇐` direction
    Self : Debug + Copy + Ord + Hash,
{}

derive_alias! {
    #[derive(TheUsualSuspects!)] = #[derive(
        Debug,
        Copy,   Clone,
        Ord,    PartialOrd, Eq, PartialEq,
        Hash,
    )];
}

#[derive(TheUsualSuspects!)]
struct KeyserSöze;

const _: () = {
    fn compile_time_assert_impls<T : ?Sized> ()
    where
        T : TheUsualSuspects,
    {}

    let _ = compile_time_assert_impls::<KeyserSöze>;
};
``` */
///
/// ### Caveat regarding derive helpers (inert-made attributes)
///
/// <details><summary>Click to see</summary>
///
/// Some derive attributes (such as `{De,}Serialize`), can involve helper
/// attributes (such as `#[serde]`).
/// This yields
/// <a href="https://doc.rust-lang.org/1.60.0/reference/attributes.html#active-and-inert-attributes" target="_blank">inert</a>
/// derive-<a href="https://doc.rust-lang.org/1.60.0/reference/procedural-macros.html#derive-macro-helper-attributes" target="_blank">helper-attributes</a>,
/// which represent a _semantic_ aspect of the derive that
/// **non-compiler-blessed macros such as this one cannot possibly know about**.
///
/// This makes aliasing such derives problematic, **since the `derive` aliases
/// won't be able to handle the helper attributes**.
///
/** ```rust ,compile_fail
# fn main () {}
#[macro_use]
extern crate macro_rules_attribute;

derive_alias! {
    #[derive(Serde!)] = #[derive(::serde::Deserialize, ::serde::Serialize)];
}

#[derive(Serde!)]
#[serde(rename_all = "snake_case")] // Error, unknown `#[serde]` attribute
struct Mejrs {
    swaginess: u8,
}
``` */
///
/// The above, for instance, yields something along the lines of:
///
/** ```rust
# #[cfg(any())] macro_rules! ignore {
 error: cannot find attribute "serde" in this scope
   --> src/lib.rs:11:3
    |
 11 | #[serde(rename_all = "snake_case")]
    |   ^^^^^
    |
    = note: "serde" is in scope, but it is a crate, not an attribute
# }
``` */
///
/// The only solution is to forgo the niceties of a `derive_alias!`, and define
/// your own <code>#\[[apply]\]</code>-able `macro_rules_attribute` that aliases
/// the `#[derive(…)]` attribute as a whole. [`attribute_alias!`] can come in
/// handy in such situations:
///
/** ```rust
# fn main () {}
#[macro_use]
extern crate macro_rules_attribute;

attribute_alias! {
    #[apply(derive_Serde)] = #[derive(::serde::Deserialize, ::serde::Serialize)];
}

#[apply(derive_Serde)]
#[serde(rename_all = "snake_case")] // OK
struct Mejrs {
    swaginess: u8,
}
``` */
///
/// ___
///
/// </details>
#[macro_export]
macro_rules! derive_alias {(
    $(
        #[derive($MacroName:ident !)] = #[derive($($derives:tt)*)];
    )*
) => (
    $crate::ඞ_with_dollar! {( $_:tt ) => (
        $crate::ඞ::paste! {
            $(
                // To avoid ambiguities with what the re-export
                // refers to, let's use a hopefully unused name.
                //
                // Indeed, eponymous derive macros in scope such as those
                // from the prelude would otherwise cause trouble with the
                // re-export line.
                #[allow(nonstandard_style)]
                macro_rules! [< $MacroName __derive_macro >] {(
                    $_($item:tt)*
                ) => (
                    $crate::ඞ_nested_derive! {
                        #[derive($($derives)*)]
                        $_($item)*
                    }
                )}
                #[allow(unused_imports)]
                pub(in crate) use [< $MacroName __derive_macro >] as $MacroName;
            )*
        }
    )}
)}


/// Convenience macro to define new attribute aliases.
///
/// The so-defined macros are intended to be used by <code>#\[[apply]]</code>.
///
/// ## Examples
///
/** ```rust
# fn main () {}
#[macro_use]
extern crate macro_rules_attribute;

attribute_alias! {
    #[apply(complex_cfg)] = #[cfg(
        any(
            test,
            doc,
            all(
                feature = "some very complex cfg",
                target_arch = "…",
            ),
        )
    )];

    #[apply(NOT_PART_OF_THE_PUBLIC_API!)] =
        /// Not part of the public API
        #[doc(hidden)]
    ;
}

#[apply(complex_cfg)]
struct Foo {
    // …
}

#[apply(NOT_PART_OF_THE_PUBLIC_API!)]
pub mod __macro_internals {
    // …
}
``` */
///
#[macro_export]
macro_rules! attribute_alias {(
    $(
        #[apply($name:ident $(!)?)] = $( #[$($attrs:tt)*] )+;
    )*
) => (
    $(
        $crate::ඞ_with_dollar! {( $_:tt ) => (
            // Let's not do the paste + module + re-export dance here since it
            // is less likely for an attribute name to collide with a prelude item.
            #[allow(nonstandard_style)]
            macro_rules! $name {( $_($item:tt)* ) => (
             $( #[$($attrs)*] )+
                $_($item)*
            )}
            #[allow(unused_imports)]
            pub(in crate) use $name;
        )}
    )*
)}

#[doc(hidden)] /** Not part of the public API*/ #[macro_export]
macro_rules! ඞ_with_dollar {( $($rules:tt)* ) => (
    macro_rules! __emit__ { $($rules)* }
    __emit__! { $ }
)}

/// Like <code>#\[[macro_rules_derive]\]</code>, but for allowing to be
/// used to shadow [the "built-in" `#[derive]` attribute][1] (on Rust ≥ 1.57.0).
///
/// [1]: https://doc.rust-lang.org/stable/core/prelude/v1/macro.derive.html
///
/// That is, it is made a bit more lenient to allow for things such as:
/** ```rust
#[macro_use]
extern crate macro_rules_attribute;

derive_alias! {
    #[derive(Eq!)] = #[derive(PartialEq, Eq)];
}

#[derive(Debug, Eq!)]
struct Foo;

fn main ()
{
    assert_eq!(Foo, Foo);
}
``` */
///
/// This is achieved thanks to **checking for the presence of a terminating `!`
/// (or lack thereof)** to determine whether the given derive macro is a classic
/// procedural macro one or a `macro_rules!` one.
///
/// Also derives [`Custom`] to allow using `#[custom(...)]` and `#[derive_args(...)]`
/// as derive helpers.
pub use ::macro_rules_attribute_proc_macro::derive;

/// No-op macro that is automatically derived with [`derive`] or [`macro_rules_derive`]
/// to allow using the `#[custom(...)]` and `#[derive_args(...)]` attribute.
///
/// See <https://github.com/danielhenrymantilla/macro_rules_attribute-rs/issues/9> for more info.
pub use ::macro_rules_attribute_proc_macro::Custom;

attribute_alias! {
    #[apply(this_macro_is_private!)] =
        #[doc(hidden)]
        /// Not part of the public API
        #[macro_export]
    ;
}

mod nested_derive {
    //! Inlined mini-version of `::nested_derive`.
    #[crate::apply(this_macro_is_private!)]
    macro_rules! ඞ_nested_derive {
        (
            #[derive( $($Derives:tt)* )]
            $($rest:tt)*
        ) => (
            #[$crate::derive( $($Derives)* )]
            #[$crate::apply($crate::ඞ_dalek_EXTERMINATE!)]
            $($rest)*
        );
    }

    // Ideally this would have been `dalek_☃_EXTERMINATE`, since the snowman
    // ressembles a Dalek more, which is a paramount aspect of this hidden macro
    // but for some reason Rust despises snowmen even though there are
    // ඞ-infected identifiers among (s)us…
    #[crate::apply(this_macro_is_private!)]
    macro_rules! ඞ_dalek_EXTERMINATE {( $it:item ) => ()}
}

#[doc(hidden)] /** Not part of the public API */ pub
mod ඞ {
    pub use {
        ::paste::paste,
    };
}
