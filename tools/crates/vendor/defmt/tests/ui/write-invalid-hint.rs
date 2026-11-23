struct S;

impl defmt::Format for S {
    fn format(&self, f: defmt::Formatter) {
        defmt::write!(f, "{=u8:dunno}", 42)
    }
}

fn main() {}
