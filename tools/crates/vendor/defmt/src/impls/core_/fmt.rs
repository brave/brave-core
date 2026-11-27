use super::*;

impl Format for core::fmt::Error {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "fmt::Error")
    }
}
