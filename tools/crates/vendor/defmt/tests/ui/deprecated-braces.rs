#![deny(deprecated)]

fn main() {
    defmt::info!("}{{}");
    defmt::println!("}{{}");
}

struct S;

impl defmt::Format for S {
    fn format(&self, f: defmt::Formatter) {
        defmt::write!(f, "}{{}");
    }
}

defmt::timestamp!("}{{}{=u32}", 0);
