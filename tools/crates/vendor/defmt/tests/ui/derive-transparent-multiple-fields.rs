#[derive(defmt::Format)]
#[defmt(transparent)]
enum A {
    Invalid { foo: u8, bar: i8 },
}

#[derive(defmt::Format)]
#[defmt(transparent)]
struct Foo {
    foo: u8,
    bar: i8,
}

fn main() {}
