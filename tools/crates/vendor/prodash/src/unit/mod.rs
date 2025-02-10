use std::{fmt, ops::Deref, sync::Arc};

use crate::progress::Step;

#[cfg(feature = "unit-bytes")]
mod bytes;
#[cfg(feature = "unit-bytes")]
pub use bytes::Bytes;

#[cfg(feature = "unit-duration")]
mod duration;
#[cfg(feature = "unit-duration")]
pub use duration::Duration;

#[cfg(feature = "unit-human")]
///
pub mod human;
#[cfg(feature = "unit-human")]
#[doc(inline)]
pub use human::Human;

mod range;
pub use range::Range;

mod traits;
pub use traits::DisplayValue;

/// Various utilities to display values and units.
pub mod display;

/// A configurable and flexible unit for use in [Progress::init()][crate::Progress::init()].
#[derive(Debug, Clone, Hash)]
pub struct Unit {
    kind: Kind,
    mode: Option<display::Mode>,
}

/// Either a static label or a dynamic one implementing [`DisplayValue`].
#[derive(Clone)]
pub enum Kind {
    /// Display only the given statically known label.
    Label(&'static str),
    /// Display a label created dynamically.
    Dynamic(Arc<dyn DisplayValue + Send + Sync>),
}

impl std::hash::Hash for Kind {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        match self {
            Kind::Label(s) => {
                0.hash(state);
                s.dyn_hash(state)
            }
            Kind::Dynamic(label) => {
                1.hash(state);
                label.dyn_hash(state);
            }
        }
    }
}

impl fmt::Debug for Kind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Kind::Label(name) => f.write_fmt(format_args!("Unit::Label({:?})", name)),
            Kind::Dynamic(_) => f.write_fmt(format_args!("Unit::Dynamic(..)")),
        }
    }
}

impl From<&'static str> for Unit {
    fn from(v: &'static str) -> Self {
        label(v)
    }
}

/// Returns a unit that is a static `label`.
pub fn label(label: &'static str) -> Unit {
    Unit {
        kind: Kind::Label(label),
        mode: None,
    }
}

/// Returns a unit that is a static `label` along with information on where to display a fraction and throughput.
pub fn label_and_mode(label: &'static str, mode: display::Mode) -> Unit {
    Unit {
        kind: Kind::Label(label),
        mode: Some(mode),
    }
}

/// Returns a unit that is a dynamic `label`.
pub fn dynamic(label: impl DisplayValue + Send + Sync + 'static) -> Unit {
    Unit {
        kind: Kind::Dynamic(Arc::new(label)),
        mode: None,
    }
}

/// Returns a unit that is a dynamic `label` along with information on where to display a fraction and throughput.
pub fn dynamic_and_mode(label: impl DisplayValue + Send + Sync + 'static, mode: display::Mode) -> Unit {
    Unit {
        kind: Kind::Dynamic(Arc::new(label)),
        mode: Some(mode),
    }
}

/// Display and utilities
impl Unit {
    /// Create a representation of `self` implementing [`Display`][std::fmt::Display] in configurable fashion.
    ///
    /// * `current_value` is the progress value to display.
    /// * `upper_bound` is the possibly available upper bound of `current_value`.
    /// * `throughput` configures how throughput should be displayed if already available.
    ///
    /// Note that `throughput` is usually not available the first time a value is displayed.
    pub fn display(
        &self,
        current_value: Step,
        upper_bound: Option<Step>,
        throughput: impl Into<Option<display::Throughput>>,
    ) -> display::UnitDisplay {
        display::UnitDisplay {
            current_value,
            upper_bound,
            throughput: throughput.into(),
            parent: self,
            display: display::What::ValuesAndUnit,
        }
    }

    /// Return `self` as trait object implementing `DisplayValue`.
    pub fn as_display_value(&self) -> &dyn DisplayValue {
        match self.kind {
            Kind::Label(ref unit) => unit,
            Kind::Dynamic(ref unit) => unit.deref(),
        }
    }
}
