#[derive(defmt::Format)]
#[defmt(crate = unresolved)]
struct S {}

#[derive(defmt::Format)]
#[defmt(crate = "not a path")]
struct S2 {}

#[derive(defmt::Format)]
#[defmt(crate(defmt))]
struct S3 {}

fn main() {}
