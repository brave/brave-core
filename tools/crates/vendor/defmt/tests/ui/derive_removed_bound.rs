struct Flavor;

trait FlavorT {
    type Str; 
}

impl FlavorT for Flavor {
    type Str = &'static str;
}

#[derive(defmt::Format)]
#[defmt(bound())]
#[defmt(bound(F: Sized))]
enum Flavored<F: FlavorT> {
    Str(F::Str),
}

const _: () = {
    const fn implements_format<T: defmt::Format>() {}

    implements_format::<Flavored<Flavor>>();
};

fn main() {}
