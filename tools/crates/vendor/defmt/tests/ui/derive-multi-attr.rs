#[derive(defmt::Format)]
struct S {
    #[defmt(Debug2Format)]
    #[defmt()]
    f: bool,
}

fn main() {}
