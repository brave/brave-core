use std::{fmt, hash::Hasher};

use crate::{progress::Step, unit::display};

/// A trait to encapsulate all capabilities needed to display a value with unit within a renderer.
pub trait DisplayValue {
    /// Display the absolute `value` representing the current progress of an operation and write it to `w`.
    ///
    /// The `upper` bound is possibly provided when known to add context, even though it is not to be output
    /// as part of this method call.
    fn display_current_value(&self, w: &mut dyn fmt::Write, value: Step, _upper: Option<Step>) -> fmt::Result {
        fmt::write(w, format_args!("{}", value))
    }
    /// Emit a token to separate two values.
    ///
    /// The `value` and its `upper` bound are provided to add context, even though it is not to be output
    /// as part of this method call.
    fn separator(&self, w: &mut dyn fmt::Write, _value: Step, _upper: Option<Step>) -> fmt::Result {
        w.write_str("/")
    }

    /// Emit the `upper_bound` to `w`.
    ///
    /// The `value` is provided to add context, even though it is not to be output as part of this method call.
    fn display_upper_bound(&self, w: &mut dyn fmt::Write, upper_bound: Step, _value: Step) -> fmt::Result {
        fmt::write(w, format_args!("{}", upper_bound))
    }

    /// A way to hash our state without using generics.
    ///
    /// This helps to determine quickly if something changed.
    fn dyn_hash(&self, state: &mut dyn std::hash::Hasher);

    /// Emit the unit of `value` to `w`.
    ///
    /// The `value` is provided to add context, even though it is not to be output as part of this method call.
    fn display_unit(&self, w: &mut dyn fmt::Write, value: Step) -> fmt::Result;

    /// Emit `percentage` to `w`.
    fn display_percentage(&self, w: &mut dyn fmt::Write, percentage: f64) -> fmt::Result {
        w.write_fmt(format_args!("[{}%]", percentage as usize))
    }

    /// Emit the `throughput` of an operation to `w`.
    fn display_throughput(&self, w: &mut dyn fmt::Write, throughput: &display::Throughput) -> fmt::Result {
        let (fraction, unit) = self.fraction_and_time_unit(throughput.timespan);
        w.write_char('|')?;
        self.display_current_value(w, throughput.value_change_in_timespan, None)?;
        w.write_char('/')?;
        match fraction {
            Some(fraction) => w.write_fmt(format_args!("{}", fraction)),
            None => Ok(()),
        }?;
        w.write_fmt(format_args!("{}|", unit))
    }

    /// Given a `timespan`, return a fraction of the timespan based on the given unit, i.e. `(possible fraction, unit`).
    fn fraction_and_time_unit(&self, timespan: std::time::Duration) -> (Option<f64>, &'static str) {
        fn skip_one(v: f64) -> Option<f64> {
            if (v - 1.0).abs() < f64::EPSILON {
                None
            } else {
                Some(v)
            }
        }
        const HOUR_IN_SECS: u64 = 60 * 60;
        let secs = timespan.as_secs();
        let h = secs / HOUR_IN_SECS;
        if h > 0 {
            return (skip_one(secs as f64 / HOUR_IN_SECS as f64), "h");
        }
        const MINUTES_IN_SECS: u64 = 60;
        let m = secs / MINUTES_IN_SECS;
        if m > 0 {
            return (skip_one(secs as f64 / MINUTES_IN_SECS as f64), "m");
        }
        if secs > 0 {
            return (skip_one(secs as f64), "s");
        }

        (skip_one(timespan.as_millis() as f64), "ms")
    }
}

impl DisplayValue for &'static str {
    fn dyn_hash(&self, state: &mut dyn Hasher) {
        state.write(self.as_bytes())
    }

    fn display_unit(&self, w: &mut dyn fmt::Write, _value: usize) -> fmt::Result {
        w.write_fmt(format_args!("{}", self))
    }
}
