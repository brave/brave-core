use core::str;

use super::*;

impl Format for str::Utf8Error {
    fn format(&self, fmt: Formatter) {
        if let Some(error_len) = self.error_len() {
            crate::write!(
                fmt,
                "invalid utf-8 sequence of {=usize} bytes from index {=usize}",
                error_len,
                self.valid_up_to(),
            );
        } else {
            crate::write!(
                fmt,
                "incomplete utf-8 byte sequence from index {=usize}",
                self.valid_up_to(),
            );
        }
    }
}

impl Format for str::ParseBoolError {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "provided string was not `true` or `false`");
    }
}
