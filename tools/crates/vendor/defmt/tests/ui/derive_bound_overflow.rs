struct Flavor;

trait FlavorT {
    type List<'a, T: 'a + ::defmt::Format>: ::defmt::Format; 
}

impl FlavorT for Flavor {
    type List<'a, T: 'a + ::defmt::Format> = &'a [T];
}

#[derive(defmt::Format)]
// #[defmt(bound())] // fixes the compile error
enum Flavored<F: FlavorT + 'static> {
    Str(F::List<'static, Self>),
}

const _: () = {
    const fn implements_format<T: defmt::Format>() {}

    implements_format::<Flavored<Flavor>>(); // overflow by the compiler, can only be fixed by removing the bound
};

fn main() {}
