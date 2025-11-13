# `::macro_rules_attribute`

Use declarative macros in attribute or derive position.

```rust ,ignore
macro_rules! my_fancy_decorator { /* … */ }

#[apply(my_fancy_decorator!)]
struct Foo { /* … */ }
```

```rust ,ignore
macro_rules! MyFancyDerive { /* … */ }

#[derive(MyFancyDerive!)]  /* using this crate's `#[derive]` attribute */
struct Foo { /* … */ }
```


[![Repository](https://img.shields.io/badge/repository-GitHub-brightgreen.svg)](
https://github.com/danielhenrymantilla/macro_rules_attribute-rs)
[![Latest version](https://img.shields.io/crates/v/macro_rules_attribute.svg)](
https://crates.io/crates/macro_rules_attribute)
[![Documentation](https://docs.rs/macro_rules_attribute/badge.svg)](
https://docs.rs/macro_rules_attribute)
[![MSRV](https://img.shields.io/badge/MSRV-1.78.0-white)](
https://gist.github.com/danielhenrymantilla/9b59de4db8e5f2467ed008b3c450527b)
[![unsafe forbidden](https://img.shields.io/badge/unsafe-forbidden-success.svg)](
https://github.com/rust-secure-code/safety-dance/)
[![License](https://img.shields.io/crates/l/macro_rules_attribute.svg)](
https://github.com/danielhenrymantilla/macro_rules_attribute-rs/blob/master/LICENSE-ZLIB)
[![CI](https://github.com/danielhenrymantilla/macro_rules_attribute-rs/workflows/CI/badge.svg)](
https://github.com/danielhenrymantilla/macro_rules_attribute-rs/actions)

<!-- Templated by `cargo-generate` using https://github.com/danielhenrymantilla/proc-macro-template -->

## Motivation

<details><summary>Click to see</summary>

`macro_rules!` macros can be extremely powerful, but their call-site ergonomics
are sometimes not great, especially when decorating item definitions.

Indeed, compare:

```rust ,ignore
foo! {
    struct Struct {
        some_field: SomeType,
    }
}
```

to:

```rust ,ignore
#[foo]
struct Struct {
    some_field: SomeType,
}
```

 1. The former does not scale well, since it leads to **rightward drift and
"excessive" braces**.

 1. But on the other hand, the latter requires setting up a dedicated crate for
    the compiler, a `proc-macro` crate. And 99% of the time this will pull the
    [`::syn`] and [`::quote`] dependencies, which have **a
    non-negligible compile-time overhead** (the first time they are compiled).

     - note: these crates are a wonderful piece of technology, and can lead to
       extremely powerful macros. When the logic of the macro is so complicated
       that it requires a recursive `tt` muncher when implemented as a
       `macro_rules!` macro, it is definitely time to be using a `proc`edural
       macro.

       Anything involving `ident` generation / derivation, for instance, will very
       often require `proc`edural macros, unless it is simple enough for
       [`::paste`] to handle it.

___

## Solution

</details>

With this crate's <code>#\[[apply]\]</code> and <code>#\[[derive]\]</code>
attributes, it is now possible to use `proc_macro_attribute` syntax to apply a
`macro_rules!` macro:


```rust
#[macro_use]
extern crate macro_rules_attribute;

macro_rules! foo {
    // …
    # ( $($tt:tt)* ) => ()
}

macro_rules! Bar {
    // …
    # ( $($tt:tt)* ) => ()
}

#[apply(foo)]
#[derive(Debug, Bar!)]
struct Struct {
    some_field: SomeType,
}
#
# fn main() {}
```

without even depending on [`::quote`], [`::syn`] or [`::proc-macro2`], for
**fast compile times**.

[`::paste`]: https://docs.rs/paste
[`::proc-macro2`]: https://docs.rs/proc_macro2
[`::syn`]: https://docs.rs/syn
[`::quote`]: https://docs.rs/quote
[`::pin-project`]: https://docs.rs/pin-project
[`::pin-project-lite`]: https://docs.rs/pin-project-lite

# Examples

<details><summary>Click to see</summary>

### Nicer derives

```rust
#[macro_use]
extern crate macro_rules_attribute;

// Easily define shorthand aliases for "derive groups"
derive_alias! {
    #[derive(Eq!)] = #[derive(Eq, PartialEq)];
    #[derive(Ord!)] = #[derive(Ord, PartialOrd, Eq!)];
    #[derive(Copy!)] = #[derive(Copy, Clone)];
    #[derive(StdDerives!)] = #[derive(Debug, Copy!, Default, Ord!, Hash)];
}

/// Strongly-typed newtype wrapper around a `usize`, to be used for `PlayerId`s.
#[derive(StdDerives!, Into!, From!)]
pub
struct PlayerId /* = */ (
    pub usize,
);

// You can also fully define your own derives using `macro_rules!` syntax
// (handling generic type definitions may be the only finicky thing, though…)
macro_rules! Into {(
    $( #[$attr:meta] )*
    $pub:vis
    struct $NewType:ident (
        $(#[$field_attr:meta])*
        $field_pub:vis
        $Inner:ty $(,

        $($rest:tt)* )?
    );
) => (
    impl ::core::convert::Into<$Inner> for $NewType {
        #[inline]
        fn into (self: $NewType)
          -> $Inner
        {
            self.0
        }
    }
)} use Into;

macro_rules! From {(
    $( #[$attr:meta] )*
    $pub:vis
    struct $NewType:ident (
        $(#[$field_attr:meta])*
        $field_pub:vis
        $Inner:ty $(,

        $(#[$other_field_attr:meta])*
        $other_field_pub:vis
        $Rest:ty )* $(,)?
    );
) => (
    impl ::core::convert::From<$Inner> for $NewType {
        #[inline]
        fn from (inner: $Inner)
          -> Self
        {
            Self(inner, $($Rest::default),*)
        }
    }
)} use From;
#
# fn main() {}
```

### Have a `-lite` version of a proc-macro dependency that thus requires unergonomic `macro_rules!`?

Say you are writing a (pervasive and yet) tiny dependency within the `async`
ecosystem.

  - By virtue of working with `async`, you'll most probably need to deal with
    pin-projections, and thence, with [`::pin-project`].

  - But by virtue of being (pervasive and yet) tiny, you don't want to depend
    on the `quote / proc-macro2 / syn` heavyweight[^only_full_syn_is_heavy]
    troika/trinity/triumvirate of more advanced proc-macro crates.

[^only_full_syn_is_heavy]: (note that only `syn` with the `"full"` features would be the truly heavyweight party)

Hence why you may reach for something such as [`::pin-project-lite`], and its
`pin_project!` `macro_rules!`-based polyfill of the former's `#[pin_project]`
attribute.

But this suddenly hinders the ergonomics of your type definitions, and, worse,
would not be composable whenever the pattern were to be repeated for some other
functionality (_e.g._, say a `cell_project!` similar macro).

Say no more! Time to <code>#\[[apply]\]</code> our neat trick:

```rust
#[macro_use]
extern crate macro_rules_attribute;

use {
    ::core::pin::{
        Pin,
    },
    ::pin_project_lite::{
        pin_project,
    },
};

#[apply(pin_project!)]
struct Struct<T, U> {
    #[pin]
    pinned: T,
    unpinned: U,
}

impl<T, U> Struct<T, U> {
    fn method(self: Pin<&mut Self>) {
        let this = self.project();
        let _: Pin<&mut T> = this.pinned; // Pinned reference to the field
        let _: &mut U = this.unpinned; // Normal reference to the field
    }
}
#
# fn main() {}
```

### More ergonomic `lazy_static!`s

Say you had something like:

```rust
# use Sync as Logic;
#
static MY_GLOBAL: &dyn Logic = &Vec::<i32>::new();
```

and now you want to change the value of that `MY_GLOBAL` to something that isn't
`const`-constructible, and yet would like to minimize the churn in doing so.

```rust ,compile_fail
// (For those unaware of it, leaking memory to initialize a lazy static is
// a completely fine pattern, since it only occurs once, and thus, a bounded
// amount of times).
static MY_GLOBAL: &dyn Logic = Box::leak(Box::new(vec![42, 27])); // Error: not `const`!
```

You could _directly_ use a `lazy_static!` or a `OnceCell`, but then the
definition of your `static` will now appear noisier than it needs be. It's time
for attribute-position polish!

First, define the helper around, say, `OnceCell`'s `Lazy` type:

```rust
macro_rules! lazy_init {(
    $( #[$attrs:meta] )*
    $pub:vis
    static $NAME:ident: $Ty:ty = $init_value:expr ;
) => (
    $( #[$attrs] )*
    $pub
    static $NAME : ::once_cell::sync::Lazy<$Ty> =
        ::once_cell::sync::Lazy::new(|| $init_value)
    ;
)} pub(in crate) use lazy_init;
```

and now it is time to use it!:

```rust
# use Sync as Logic;
#
#[macro_use]
extern crate macro_rules_attribute;

#[apply(lazy_init)]
static MY_GLOBAL: &dyn Logic = Box::leak(Box::new(vec![42, 27]));
#
# macro_rules! lazy_init {(
#     $( #[$attrs:meta] )*
#     $pub:vis
#     static $NAME:ident : $Ty:ty = $init_value:expr ;
# ) => (
#     $( #[$attrs] )*
#     $pub
#     static $NAME : ::once_cell::sync::Lazy<$Ty> =
#         ::once_cell::sync::Lazy::new(|| $init_value)
#     ;
# )} use lazy_init;
#
# fn main() {}
```

</details>

# Debugging

An optional compilation feature, `"verbose-expansions"` can be used to print at
compile-time the exact output of each macro invocation from this crate:

```toml
[dependencies]
macro_rules_attribute.version = "..."
macro_rules_attribute.features = ["verbose-expansions"]
```

# Features

### `derive` aliases

```rust
# fn main() {}
#[macro_use]
extern crate macro_rules_attribute;

derive_alias! {
    #[derive(Ord!)] = #[derive(PartialEq, Eq, PartialOrd, Ord)];
}

#[derive(Debug, Clone, Copy, Ord!)]
struct Foo {
    // …
}
```

  - See [`derive_alias!`] and <code>#\[[derive]\]</code> for more info.

### `cfg` aliases

<details><summary>Click to see</summary>

```rust
# fn main() {}
#[macro_use]
extern crate macro_rules_attribute;

attribute_alias! {
    #[apply(complex_cfg!)] = #[cfg(
        any(
            any(
                foo,
                feature = "bar",
            ),
            all(
                target_os = "fenestrations",
                not(target_arch = "Pear"),
            ),
        ),
    )];
}

#[apply(complex_cfg!)]
mod some_item { /* … */ }
```

</details>

### Not using `#[macro_use] extern crate macro_rules_attribute`

<details><summary>Click to see</summary>

If you are allergic to `#[macro_use]` unscoped / globally-preluded semantics,
you may not be fond of having to use:

```rust
#[macro_use]
extern crate macro_rules_attribute;
# fn main() {}
```

like this documentation pervasively does.

In that case, know that you may very well stick to using `use` imports:

```rust
use ::macro_rules_attribute::{derive, derive_alias, /* … */};
// or even
use ::macro_rules_attribute::*;

derive_alias! {
    #[derive(Copy!)] = #[derive(Clone, Copy)];
}

#[derive(Copy!)]
struct Foo;
```

or even inlining the fully qualified paths (but note that the `…_alias!` macros
still take unqualified paths inside the definitions):

```rust
::macro_rules_attribute::derive_alias! {
    #[derive(Copy!)] = #[derive(Clone, Copy)];
}

#[::macro_rules_attribute::derive(Copy!)]
struct Foo;
```

I personally find these approaches too noisy to be worth it, despite the so
gained "namespace purity", hence my not using that pattern across the rest of
the examples.

</details>

[apply]: https://docs.rs/macro_rules_attribute/0.1.*/macro_rules_attribute/attr.apply.html
[derive]: https://docs.rs/macro_rules_attribute/0.1.*/macro_rules_attribute/attr.derive.html
[`derive_alias!`]: https://docs.rs/macro_rules_attribute/0.1.*/macro_rules_attribute/macro.derive_alias.html
