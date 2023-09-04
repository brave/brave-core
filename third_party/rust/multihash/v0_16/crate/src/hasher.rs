/// Trait implemented by a hash function implementation.
pub trait Hasher {
    /// Consume input and update internal state.
    fn update(&mut self, input: &[u8]);

    /// Returns the final digest.
    fn finalize(&mut self) -> &[u8];

    /// Reset the internal hasher state.
    fn reset(&mut self);
}
