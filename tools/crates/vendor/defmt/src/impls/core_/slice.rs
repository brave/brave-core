use super::*;

impl<'a, T: 'a> Format for core::slice::ChunksExact<'a, T>
where
    T: Format,
{
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "ChunksExact(..)")
    }
}

impl<'a, T: 'a> Format for core::slice::Iter<'a, T>
where
    T: Format,
{
    fn format(&self, fmt: Formatter) {
        crate::write!(
            fmt,
            "Iter {{ slice: {=[?]}, position: ? }}",
            self.as_slice()
        )
    }
}

impl<'a, T: 'a> Format for core::slice::Windows<'a, T>
where
    T: Format,
{
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "Windows(..)")
    }
}
