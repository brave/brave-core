fn main() {}

#[derive(defmt::Format)]
#[defmt(transparent)]
enum Empty {}

#[derive(defmt::Format)]
#[defmt(transparent)]
struct Unit;

#[derive(defmt::Format)]
#[defmt(transparent)]
enum UnitVariants {
    Foo,
    Bar,
}
