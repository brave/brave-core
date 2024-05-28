//! Structs and methods for handling Zcash transactions.
pub mod components;

use std::io::{self, Read, Write};
use self::{
    components::{
        amount::{Amount},
    }
};

pub struct Transaction {
}

impl Transaction {
   fn read_amount<R: Read>(mut reader: R) -> io::Result<Amount> {
        let mut tmp = [0; 8];
        reader.read_exact(&mut tmp)?;
        Amount::from_i64_le_bytes(tmp)
            .map_err(|_| io::Error::new(io::ErrorKind::InvalidData, "valueBalance out of range"))
    }
}
