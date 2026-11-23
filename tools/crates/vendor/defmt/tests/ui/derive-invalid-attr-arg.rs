#[derive(defmt::Format)]
struct S {
    #[defmt(FooBar)]
    f: bool,
}

fn main() {}
