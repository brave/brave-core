#![deny(missing_docs)]
//! Some docs

#[macro_use]
extern crate derive_more;

fn main() {}

/// Some docs
#[derive(
    Add,
    AddAssign,
    Constructor,
    Display,
    From,
    FromStr,
    Into,
    Mul,
    MulAssign,
    Not
)]
pub struct MyInt(i32);

/// Some docs
#[derive(Deref, DerefMut)]
pub struct MyBoxedInt(Box<i32>);

/// Some docs
#[derive(Index, IndexMut)]
pub struct MyVec(Vec<i32>);

/// Some docs
#[allow(dead_code)]
#[derive(Clone, Copy, TryInto)]
#[derive(IsVariant)]
enum MixedInts {
    SmallInt(i32),
    NamedBigInt { int: i64 },
}
