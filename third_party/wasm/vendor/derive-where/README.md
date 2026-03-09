# derive-where

[![Crates.io Version](https://img.shields.io/crates/v/derive-where.svg)](https://crates.io/crates/derive-where)
[![Live Build Status](https://img.shields.io/github/actions/workflow/status/ModProg/derive-where/test.yml?branch=main)](https://github.com/ModProg/derive-where/actions/workflows/test.yml)
[![Docs.rs Documentation](https://img.shields.io/docsrs/derive-where)](https://docs.rs/crate/derive-where)

## Description

Attribute proc-macro to simplify deriving standard and other traits with
custom generic type bounds.

## Usage

The [`derive_where`] attribute can be used just like
std's `#[derive(...)]` statements:

```rust
#[derive_where(Clone, Debug)]
struct Example<T>(PhantomData<T>);
```

This will generate trait implementations for `Example` for any `T`,
as opposed to std's derives, which would only implement these traits with
`T: Trait` bound to the corresponding trait.

Multiple [`derive_where`] attributes can be added to an
item, but only the first one must use any path qualifications.

```rust
#[derive_where::derive_where(Clone, Debug)]
#[derive_where(Eq, PartialEq)]
struct Example1<T>(PhantomData<T>);
```

If using a different package name, you must specify this:

```rust
#[derive_where(crate = derive_where_)]
#[derive_where(Clone, Debug)]
struct Example<T>(PhantomData<T>);
```

In addition, the following convenience options are available:

### Generic type bounds

Separated from the list of traits with a semi-colon, types to bind to can be
specified. This example will restrict the implementation for `Example` to
`T: Clone`:

```rust
#[derive_where(Clone, Debug; T)]
struct Example<T, U>(T, PhantomData<U>);
```

It is also possible to specify the bounds to be applied. This will
bind implementation for `Example` to `T: Super`:

```rust
trait Super: Clone + Debug {}

#[derive_where(Clone, Debug; T: Super)]
struct Example<T>(PhantomData<T>);
```

But more complex trait bounds are possible as well.
The example below will restrict the [`Clone`] implementation for `Example`
to `T::Type: Clone`:

```rust
trait Trait {
	type Type;
}

struct Impl;

impl Trait for Impl {
	type Type = i32;
}

#[derive_where(Clone, Debug; T::Type)]
struct Example<T: Trait>(T::Type);
```

Any combination of options listed here can be used to satisfy a
specific constrain. It is also possible to use multiple separate
constrain specifications when required:

```rust
#[derive_where(Clone, Debug; T)]
#[derive_where(Eq, PartialEq; U)]
struct Example<T, U>(PhantomData<T>, PhantomData<U>);
```

### Enum default

Since Rust 1.62 deriving [`Default`] on an enum is possible with the
`#[default]` attribute. Derive-where allows this with a
`#[derive_where(default)]` attribute:

```rust
#[derive_where(Clone, Default)]
enum Example<T> {
	#[derive_where(default)]
	A(PhantomData<T>),
}
```

### Skipping fields

With a `skip` or `skip_inner` attribute fields can be skipped for traits
that allow it, which are: [`Debug`], [`Hash`], [`Ord`], [`PartialOrd`],
[`PartialEq`], [`Zeroize`] and [`ZeroizeOnDrop`].

```rust
#[derive_where(Debug, PartialEq; T)]
struct Example<T>(#[derive_where(skip)] T);

assert_eq!(format!("{:?}", Example(42)), "Example");
assert_eq!(Example(42), Example(0));
```

It is also possible to skip all fields in an item or variant if desired:

```rust
#[derive_where(Debug, PartialEq)]
#[derive_where(skip_inner)]
struct StructExample<T>(T);

assert_eq!(format!("{:?}", StructExample(42)), "StructExample");
assert_eq!(StructExample(42), StructExample(0));

#[derive_where(Debug, PartialEq)]
enum EnumExample<T> {
	#[derive_where(skip_inner)]
	A(T),
}

assert_eq!(format!("{:?}", EnumExample::A(42)), "A");
assert_eq!(EnumExample::A(42), EnumExample::A(0));
```

Selective skipping of fields for certain traits is also an option, both in
`skip` and `skip_inner`. To prevent breaking invariants defined for these
traits, some of them can only be skipped in groups. The following groups are
available:
- [`Debug`]
- `EqHashOrd`: Skips [`Eq`], [`Hash`], [`Ord`], [`PartialOrd`] and
  [`PartialEq`].
- [`Hash`]
- `Zeroize`: Skips [`Zeroize`] and [`ZeroizeOnDrop`].

```rust
#[derive_where(Debug, PartialEq)]
#[derive_where(skip_inner(Debug))]
struct Example<T>(i32, PhantomData<T>);

assert_eq!(format!("{:?}", Example(42, PhantomData::<()>)), "Example");
assert_ne!(
	Example(42, PhantomData::<()>),
	Example(0, PhantomData::<()>)
);
```

### Incomparable variants/items

Similar to the `skip` attribute, `incomparable` can be used to skip variants
or items in [`PartialEq`] and [`PartialOrd`] trait implementations, meaning
they will always yield `false` for `eq` and `None` for `partial_cmp`. This
results in all comparisons but `!=`, i.e. `==`, `<`, `<=`, `>=` and `>`,
with the marked variant or struct evaluating to `false`.

```rust
# use derive_where::derive_where;
#[derive(Debug)]
#[derive_where(PartialEq, PartialOrd)]
enum EnumExample {
	#[derive_where(incomparable)]
	Incomparable,
	Comparable,
}
assert_eq!(EnumExample::Comparable, EnumExample::Comparable);
assert_ne!(EnumExample::Incomparable, EnumExample::Incomparable);
assert!(!(EnumExample::Comparable >= EnumExample::Incomparable));
assert!(!(EnumExample::Comparable <= EnumExample::Incomparable));
assert!(!(EnumExample::Incomparable >= EnumExample::Incomparable));
assert!(!(EnumExample::Incomparable <= EnumExample::Incomparable));

#[derive(Debug)]
#[derive_where(PartialEq, PartialOrd)]
#[derive_where(incomparable)]
struct StructExample;

assert_ne!(StructExample, StructExample);
assert!(!(StructExample >= StructExample));
assert!(!(StructExample <= StructExample));
```

Note that it is not possible to use `incomparable` with [`Eq`] or [`Ord`] as
that would break their invariants.

### `Zeroize` options

`Zeroize` has two options:
- `crate`: an item-level option which specifies a path to the [`zeroize`]
  crate in case of a re-export or rename.
- `fqs`: a field-level option which will use fully-qualified-syntax instead
  of calling the [`zeroize`][method@zeroize] method on `self` directly. This
  is to avoid ambiguity between another method also called `zeroize`.

```rust
#[derive_where(Zeroize(crate = zeroize_))]
struct Example(#[derive_where(Zeroize(fqs))] i32);

impl Example {
	// If we didn't specify the `fqs` option, this would lead to a compile
	// error because of method ambiguity.
	fn zeroize(&mut self) {
		self.0 = 1;
	}
}

let mut test = Example(42);

// Will call the struct method.
test.zeroize();
assert_eq!(test.0, 1);

// WIll call the `Zeroize::zeroize` method.
Zeroize::zeroize(&mut test);
assert_eq!(test.0, 0);
```

### `ZeroizeOnDrop` options

If the `zeroize-on-drop` feature is enabled, it implements [`ZeroizeOnDrop`]
and can be implemented without [`Zeroize`], otherwise it only implements
[`Drop`] and requires [`Zeroize`] to be implemented.

[`ZeroizeOnDrop`] has one option:
- `crate`: an item-level option which specifies a path to the [`zeroize`]
  crate in case of a re-export or rename.

```rust
#[derive_where(ZeroizeOnDrop(crate = zeroize_))]
struct Example(i32);

assert!(core::mem::needs_drop::<Example>());
```

### Supported traits

The following traits can be derived with derive-where:
- [`Clone`]
- [`Copy`]
- [`Debug`]
- [`Default`]
- [`Eq`]
- [`Hash`]
- [`Ord`]
- [`PartialEq`]
- [`PartialOrd`]
- [`Zeroize`]: Only available with the `zeroize` crate feature.
- [`ZeroizeOnDrop`]: Only available with the `zeroize` crate feature. If the
  `zeroize-on-drop` feature is enabled, it implements [`ZeroizeOnDrop`],
  otherwise it only implements [`Drop`].

### Supported items

Structs, tuple structs, unions and enums are supported. Derive-where tries
it's best to discourage usage that could be covered by std's `derive`. For
example unit structs and enums only containing unit variants aren't
supported.

Unions only support [`Clone`] and [`Copy`].

[`PartialOrd`] and [`Ord`] need to determine the discriminant type to
function correctly. To protect against a potential future change to the
default discriminant type, some compile-time validation is inserted to
ascertain that the type remains `isize`.

### `no_std` support

`no_std` support is provided by default.

## Crate features

- `nightly`: Implements [`Ord`] and [`PartialOrd`] with the help of
  [`core::intrinsics::discriminant_value`], which is what Rust does by
  default too. This requires a nightly version of the Rust compiler.
- `safe`: `safe`: Uses only safe ways to access the discriminant of the enum
  for [`Ord`] and [`PartialOrd`]. It also replaces all cases of
  [`core::hint::unreachable_unchecked`] in [`Ord`], [`PartialEq`] and
  [`PartialOrd`], which is what std uses, with [`unreachable`].
- `zeroize`: Allows deriving [`Zeroize`] and [`zeroize`][method@zeroize] on
  [`Drop`].
- `zeroize-on-drop`: Allows deriving [`Zeroize`] and [`ZeroizeOnDrop`] and
  requires [`zeroize`] v1.5.

## MSRV

The current MSRV is 1.57 and is being checked by the CI. A change will be
accompanied by a minor version bump. If MSRV is important to you, use
`derive-where = "~1.x"` to pin a specific minor version to your crate.

## Alternatives

- [derivative](https://crates.io/crates/derivative) [![Crates.io](https://img.shields.io/crates/v/derivative.svg)](https://crates.io/crates/derivative)
  is a great alternative with many options. Notably it doesn't support
  `no_std` and requires an extra `#[derive(Derivative)]` to use.
- [derive_bounded](https://crates.io/crates/derive_bounded) [![Crates.io](https://img.shields.io/crates/v/derive_bounded.svg)](https://crates.io/crates/derive_bounded)
  is a new alternative still in development.

## Changelog

See the [CHANGELOG] file for details.

## License

Licensed under either of

- Apache License, Version 2.0 ([LICENSE-APACHE] or <http://www.apache.org/licenses/LICENSE-2.0>)
- MIT license ([LICENSE-MIT] or <http://opensource.org/licenses/MIT>)

at your option.

### Contribution

Unless you explicitly state otherwise, any contribution intentionally
submitted for inclusion in the work by you, as defined in the Apache-2.0
license, shall be dual licensed as above, without any additional terms or
conditions.

[CHANGELOG]: https://github.com/ModProg/derive-where/blob/main/CHANGELOG.md
[LICENSE-MIT]: https://github.com/ModProg/derive-where/blob/main/LICENSE-MIT
[LICENSE-APACHE]: https://github.com/ModProg/derive-where/blob/main/LICENSE-APACHE
[`Debug`]: https://doc.rust-lang.org/core/fmt/trait.Debug.html
[`Default`]: https://doc.rust-lang.org/core/default/trait.Default.html
[`Hash`]: https://doc.rust-lang.org/core/hash/trait.Hash.html
[`zeroize`]: https://docs.rs/zeroize
[`Zeroize`]: https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html
[`ZeroizeOnDrop`]: https://docs.rs/zeroize/latest/zeroize/trait.ZeroizeOnDrop.html
[method@zeroize]: https://docs.rs/zeroize/latest/zeroize/trait.Zeroize.html#tymethod.zeroize

[`Clone`]: https://doc.rust-lang.org/core/clone/trait.Clone.html
[`Copy`]: https://doc.rust-lang.org/core/marker/trait.Copy.html
[`core::hint::unreachable_unchecked`]: https://doc.rust-lang.org/core/hint/fn.unreachable_unchecked.html
[`core::intrinsics::discriminant_value`]: https://doc.rust-lang.org/core/intrinsics/fn.discriminant_value.html
[`derive_where`]: https://docs.rs/derive-where/latest/derive_where/attr.derive_where.html
[`Discriminant`]: https://doc.rust-lang.org/core/mem/struct.Discriminant.html
[`Drop`]: https://doc.rust-lang.org/core/ops/trait.Drop.html
[`Eq`]: https://doc.rust-lang.org/core/cmp/trait.Eq.html
[`i32`]: https://doc.rust-lang.org/core/primitive.i32.html
[`isize`]: https://doc.rust-lang.org/core/primitive.isize.html
[`Ord`]: https://doc.rust-lang.org/core/cmp/trait.Ord.html
[`PartialEq`]: https://doc.rust-lang.org/core/cmp/trait.PartialEq.html
[`PartialOrd`]: https://doc.rust-lang.org/core/cmp/trait.PartialOrd.html
[`transmute`]: https://doc.rust-lang.org/core/mem/fn.transmute.html
[`unreachable`]: https://doc.rust-lang.org/core/macro.unreachable.html
