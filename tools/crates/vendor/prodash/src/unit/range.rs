use std::{fmt, hash::Hasher};

use crate::{progress::Step, unit::DisplayValue};

/// A helper for formatting numbers representing ranges in renderers as in `2 of 5 steps`.
#[derive(Copy, Clone, Default, Eq, PartialEq, Ord, PartialOrd, Debug)]
pub struct Range {
    /// The name of the unit to be appended to the range.
    pub name: &'static str,
}

impl Range {
    /// A convenience method to create a new instance of `name`.
    pub fn new(name: &'static str) -> Self {
        Range { name }
    }
}

impl DisplayValue for Range {
    fn display_current_value(&self, w: &mut dyn fmt::Write, value: Step, _upper: Option<Step>) -> fmt::Result {
        w.write_fmt(format_args!("{}", value + 1))
    }
    fn separator(&self, w: &mut dyn fmt::Write, _value: Step, _upper: Option<Step>) -> fmt::Result {
        w.write_str(" of ")
    }

    fn dyn_hash(&self, state: &mut dyn Hasher) {
        self.name.dyn_hash(state)
    }

    fn display_unit(&self, w: &mut dyn fmt::Write, _value: Step) -> fmt::Result {
        w.write_str(self.name)
    }
}
