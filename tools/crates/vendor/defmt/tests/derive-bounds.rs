#![allow(dead_code)]
extern crate defmt as defmt2;

fn main() {
    let baz: Baz<Qux> = Default::default();
    defmt::info!("{}", baz);
}

trait Foo {
    type Bar;
}
#[derive(defmt::Format, Default)]
struct Baz<T: Foo> {
    field: T::Bar,
    field2: Quux<T>,
}
#[derive(defmt::Format, Default)]
struct Qux;
impl Foo for Qux {
    type Bar = Qux;
}
#[allow(dead_code)]
#[derive(defmt::Format, Default)]
enum Quux<T: Foo> {
    #[default]
    None,
    Variant1(T),
    Variant2 {
        f: T::Bar,
    },
    Variant3(T::Bar),
}

#[allow(dead_code)]
#[derive(defmt::Format)]
#[defmt(crate = defmt2)]
struct Quz;

#[derive(defmt::Format)]
#[defmt(transparent)]
#[allow(dead_code)]
enum TransparentEnum<T: Foo> {
    Quz(Quz),
    Quux(Quux<T>),
    Baz(Baz<T>),
    U16(u16),
}

#[derive(defmt::Format)]
#[defmt(transparent)]
#[allow(dead_code)]
struct Transparent<T: Foo>(Quux<T>);

#[derive(defmt::Format)]
#[defmt(transparent)]
#[defmt(transparent, crate = defmt4)]
#[defmt(crate = defmt3, crate = defmt2)]
#[allow(dead_code)]
struct Variations<T: Foo>(Quux<T>);

struct Flavor;

trait FlavorT {
    type List<'a, T: 'a + ::defmt::Format>: ::defmt::Format;
}

impl FlavorT for Flavor {
    type List<'a, T: 'a + ::defmt::Format> = &'a [T];
}

#[derive(defmt::Format)]
#[defmt(bound())] // fixes the compile error from `ui/derive_bound_overflow.rs`
enum Flavored<F: FlavorT + 'static> {
    Str(F::List<'static, Self>),
}

const _: () = {
    const fn implements_format<T: defmt::Format>() {}

    implements_format::<Flavored<Flavor>>();
};
