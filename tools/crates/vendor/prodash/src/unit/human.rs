use std::{fmt, fmt::Debug, hash::Hasher};

pub use human_format::{Formatter, Scales};

use crate::{progress::Step, unit::DisplayValue};

/// A helper for formatting numbers in a format easily read by humans in renderers, as in `2.54 million objects`
#[derive(Debug)]
pub struct Human {
    /// The name of the represented unit, like 'items' or 'objects'.
    pub name: &'static str,
    /// The formatter to format the actual numbers.
    pub formatter: Formatter,
}

impl Human {
    /// A convenience method to create a new new instance and its `formatter` and `name` fields.
    pub fn new(formatter: Formatter, name: &'static str) -> Self {
        Human { name, formatter }
    }
    fn format_bytes(&self, w: &mut dyn fmt::Write, value: Step) -> fmt::Result {
        let string = self.formatter.format(value as f64);
        for token in string.split(' ') {
            w.write_str(token)?;
        }
        Ok(())
    }
}

impl DisplayValue for Human {
    fn display_current_value(&self, w: &mut dyn fmt::Write, value: Step, _upper: Option<Step>) -> fmt::Result {
        self.format_bytes(w, value)
    }

    fn display_upper_bound(&self, w: &mut dyn fmt::Write, upper_bound: Step, _value: Step) -> fmt::Result {
        self.format_bytes(w, upper_bound)
    }

    fn dyn_hash(&self, state: &mut dyn Hasher) {
        state.write(self.name.as_bytes());
        state.write_u8(0);
    }

    fn display_unit(&self, w: &mut dyn fmt::Write, _value: Step) -> fmt::Result {
        w.write_str(self.name)
    }
}
