#![allow(dead_code)]

struct Foo {
    secret: u64,
}

opaque_debug::implement!(Foo);

struct FooGeneric<T> {
    secret: u64,
    generic: T,
}

opaque_debug::implement!(FooGeneric<T>);

struct FooManyGenerics<T, U, V> {
    secret: u64,
    generic1: T,
    generic2: U,
    generic3: V,
}

opaque_debug::implement!(FooManyGenerics<T, U, V>);

#[test]
fn debug_formatting() {
    let s = format!("{:?}", Foo { secret: 42 });
    assert_eq!(s, "Foo { ... }");
}

#[test]
fn debug_formatting_generic() {
    let s = format!(
        "{:?}",
        FooGeneric::<()> {
            secret: 42,
            generic: ()
        }
    );
    assert_eq!(s, "FooGeneric<()> { ... }");
}

#[test]
fn debug_formatting_many_generics() {
    let s = format!(
        "{:?}",
        FooManyGenerics::<(), u8, &str> {
            secret: 42,
            generic1: (),
            generic2: 0u8,
            generic3: "hello",
        }
    );
    assert_eq!(s, "FooManyGenerics<(), u8, &str> { ... }");
}
