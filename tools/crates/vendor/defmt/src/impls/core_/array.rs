use core::array;

use super::*;

impl Format for array::TryFromSliceError {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "TryFromSliceError(())");
    }
}
