//! The variable length hash function used in the Argon2 algorithm.

use crate::{Error, Result};

use blake2::{
    digest::{self, Digest, VariableOutput},
    Blake2b512, Blake2bVar,
};

use core::convert::TryFrom;

pub fn blake2b_long(inputs: &[&[u8]], out: &mut [u8]) -> Result<()> {
    if out.is_empty() {
        return Err(Error::OutputTooShort);
    }

    let len_bytes = u32::try_from(out.len())
        .map(|v| v.to_le_bytes())
        .map_err(|_| Error::OutputTooLong)?;

    // Use blake2b directly if the output is small enough.
    if out.len() <= Blake2b512::output_size() {
        let mut digest = Blake2bVar::new(out.len()).map_err(|_| Error::OutputTooLong)?;

        // Conflicting method name from `Digest` and `Update` traits
        digest::Update::update(&mut digest, &len_bytes);

        for input in inputs {
            digest::Update::update(&mut digest, input);
        }

        digest
            .finalize_variable(out)
            .map_err(|_| Error::OutputTooLong)?;

        return Ok(());
    }

    // Calculate longer hashes by first calculating a full 64 byte hash
    let half_hash_len = Blake2b512::output_size() / 2;
    let mut digest = Blake2b512::new();

    digest.update(len_bytes);
    for input in inputs {
        digest.update(input);
    }
    let mut last_output = digest.finalize();

    // Then we write the first 32 bytes of this hash to the output
    out[..half_hash_len].copy_from_slice(&last_output[..half_hash_len]);

    // Next, we write a number of 32 byte blocks to the output.
    // Each block is the first 32 bytes of the hash of the last block.
    // The very last block of the output is excluded, and has a variable
    // length in range [1, 32].
    let mut counter = 0;
    let out_len = out.len();
    for chunk in out[half_hash_len..]
        .chunks_exact_mut(half_hash_len)
        .take_while(|_| {
            counter += half_hash_len;
            out_len - counter > 64
        })
    {
        last_output = Blake2b512::digest(last_output);
        chunk.copy_from_slice(&last_output[..half_hash_len]);
    }

    // Calculate the last block with VarBlake2b.
    let last_block_size = out.len() - counter;
    let mut digest = Blake2bVar::new(last_block_size).map_err(|_| Error::OutputTooLong)?;

    digest::Update::update(&mut digest, &last_output);
    digest
        .finalize_variable(&mut out[counter..])
        .expect("invalid Blake2bVar out length");

    Ok(())
}
