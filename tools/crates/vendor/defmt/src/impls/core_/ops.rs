use super::*;

impl<Idx> Format for core::ops::Range<Idx>
where
    Idx: Format,
{
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "{}..{}", self.start, self.end)
    }
}

impl<Idx> Format for core::ops::RangeFrom<Idx>
where
    Idx: Format,
{
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "{}..", self.start)
    }
}

impl Format for core::ops::RangeFull {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "..",)
    }
}

impl<Idx> Format for core::ops::RangeInclusive<Idx>
where
    Idx: Format,
{
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "{}..={}", self.start(), self.end())
    }
}

impl<Idx> Format for core::ops::RangeTo<Idx>
where
    Idx: Format,
{
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "..{}", self.end)
    }
}

impl<Idx> Format for core::ops::RangeToInclusive<Idx>
where
    Idx: Format,
{
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "..={}", self.end)
    }
}
