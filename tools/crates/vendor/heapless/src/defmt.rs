//! Defmt implementations for heapless types
//!

use crate::Vec;
use defmt::Formatter;

impl<T, const N: usize> defmt::Format for Vec<T, N>
where
    T: defmt::Format,
{
    fn format(&self, fmt: Formatter<'_>) {
        defmt::write!(fmt, "{=[?]}", self.as_slice())
    }
}

impl<const N: usize> defmt::Format for crate::String<N>
where
    u8: defmt::Format,
{
    fn format(&self, fmt: Formatter<'_>) {
        defmt::write!(fmt, "{=str}", self.as_str());
    }
}
