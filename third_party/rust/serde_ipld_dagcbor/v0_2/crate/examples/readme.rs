// NOTE: This file should be kept in sync with README.md

use serde_derive::{Deserialize, Serialize};
#[cfg(feature = "std")]
use std::error::Error;
#[cfg(feature = "std")]
use std::fs::File;
#[cfg(feature = "std")]
use std::io::BufReader;

// Types annotated with `Serialize` can be stored as DAG-CBOR.
// To be able to load them again add `Deserialize`.
#[derive(Debug, Serialize, Deserialize)]
struct Mascot {
    name: String,
    species: String,
    year_of_birth: u32,
}

#[cfg(feature = "std")]
fn main() -> Result<(), Box<dyn Error>> {
    let ferris = Mascot {
        name: "Ferris".to_owned(),
        species: "crab".to_owned(),
        year_of_birth: 2015,
    };

    let ferris_file = File::create("examples/ferris.cbor")?;
    // Write Ferris to the given file.
    // Instead of a file you can use any type that implements `io::Write`
    // like a HTTP body, database connection etc.
    serde_ipld_dagcbor::to_writer(ferris_file, &ferris)?;

    let tux_file = File::open("examples/tux.cbor")?;
    let tux_reader = BufReader::new(tux_file);
    // Load Tux from a file.
    // Serde IPLD DAG-CBOR performs roundtrip serialization meaning that
    // the data will not change in any way.
    let tux: Mascot = serde_ipld_dagcbor::from_reader(tux_reader)?;

    println!("{:?}", tux);
    // prints: Mascot { name: "Tux", species: "penguin", year_of_birth: 1996 }

    Ok(())
}

#[cfg(not(feature = "std"))]
fn main() {}
