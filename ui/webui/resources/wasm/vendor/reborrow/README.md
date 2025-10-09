Emulate reborrowing for user types.

A generalized reference is a type that has reference semantics, without actually being
a native Rust reference (like `&T` and `&mut T`). e.g.:

```rust
struct MyRefMut<'borrow> {
    a: &'borrow mut i32,
    b: &'borrow mut i32,
}
```

Given a `&'a` \[mut\] reference of a `&'b` view over some owned object,
reborrowing it means getting an active `&'a` view over the owned object,
which renders the original reference inactive until it's dropped, at which point
the original reference becomes active again.

This crate defines traits to make generalized references more ergonomic to use by giving the
ability to borrow and reborrow them.

# Features

`derive`: This imports a derive macro helper for implementing reborrow for user
types. It can be used with a `Ref/RefMut` pair of structs/tuple structs with
the same member names, one containing shared references and the other mutable
references. The shared variant must be `Copy`, and the macro is used on the
mutable variant and generates the relevant traits for both types.

# Examples

This fails to compile since we can't use a non-`Copy` value after it's moved.

```rust
fn takes_mut_option(o: Option<&mut i32>) {}

let mut x = 0;
let o = Some(&mut x);
takes_mut_option(o); // `o` is moved here,
takes_mut_option(o); // so it can't be used here.
```

This can be worked around by unwrapping the option, reborrowing it, and then wrapping it again.

```rust
fn takes_mut_option(o: Option<&mut i32>) {}

let mut x = 0;
let mut o = Some(&mut x);
takes_mut_option(o.as_mut().map(|r| &mut **r)); // "Reborrowing" the `Option`
takes_mut_option(o.as_mut().map(|r| &mut **r)); // allows us to use it later on.
drop(o); // can still be used here
```

Using this crate, this can be shortened to

```rust
use reborrow::ReborrowMut;

fn takes_mut_option(o: Option<&mut i32>) {}

let mut x = 0;
let mut o = Some(&mut x);
takes_mut_option(o.rb_mut()); // "Reborrowing" the `Option`
takes_mut_option(o.rb_mut()); // allows us to use it later on.
drop(o); // can still be used here
```

The derive macro can be used with structs or tuple structs, and generates
the trait definitions for `Reborrow` and `ReborrowMut`.

```rust
use reborrow::{ReborrowCopyTraits, ReborrowTraits};

#[derive(ReborrowCopyTraits)]
pub struct I32Ref<'a, 'b> {
    pub i: i32,
    pub j: &'a i32,
    pub k: &'b i32,
}

#[derive(ReborrowCopyTraits)]
pub struct I32TupleRef<'a, 'b>(pub i32, pub &'a i32, pub &'b i32);

#[derive(ReborrowTraits)]
#[Const(I32Ref)]
struct I32RefMut<'a, 'b> {
    i: i32,
    #[reborrow]
    j: &'a mut i32,
    #[reborrow]
    k: &'b mut i32,
}

#[derive(ReborrowTraits)]
#[Const(I32TupleRef)]
pub struct I32TupleRefMut<'a, 'b>(
    i32,
    #[reborrow] &'a mut i32,
    #[reborrow] &'b mut i32,
);
```
