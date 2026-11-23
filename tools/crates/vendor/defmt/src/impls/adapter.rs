use core::fmt;

use crate as defmt;
use crate::{export, Format, Formatter, Str};

/// An "adapter" type to feed `Debug` values into defmt macros, which expect `defmt::Format` values.
///
/// This adapter disables compression and uses the `core::fmt` code on-device! You should prefer
/// `defmt::Format` over `Debug` whenever possible.
///
/// # Examples
///
/// ```rust
/// # #[derive(Debug)]
/// # struct ExpensiveThing();
/// # let expensive_thing = ExpensiveThing();
/// #
/// defmt::info!("{:?}", defmt::Debug2Format(&expensive_thing));
/// //                                        ˆˆˆˆˆˆˆˆˆˆˆˆˆˆˆ
/// //                                        must `#[derive(Debug)]`
/// ```
///
/// Note that any provided defmt display hints will be ignored
/// because this always uses `{:?}` to format the contained value.
pub struct Debug2Format<'a, T: fmt::Debug + ?Sized>(pub &'a T);

impl<T: fmt::Debug + ?Sized> fmt::Debug for Debug2Format<'_, T> {
    fn fmt(&self, fmt: &mut core::fmt::Formatter<'_>) -> Result<(), core::fmt::Error> {
        self.0.fmt(fmt)
    }
}

impl<T: fmt::Debug + ?Sized> Format for Debug2Format<'_, T> {
    default_format!();

    fn _format_tag() -> Str {
        defmt_macros::internp!("{=__internal_Debug}")
    }

    fn _format_data(&self) {
        export::debug(&self.0);
    }
}

/// An "adapter" type to feed `Display` values into defmt macros, which expect `defmt::Format` values.
///
/// This adapter disables compression and uses the `core::fmt` code on-device! You should prefer
/// `defmt::Format` over `Display` whenever possible.
///
/// # Examples
///
/// ```rust
/// # struct ExpensiveThing();
/// #
/// # impl core::fmt::Display for ExpensiveThing {
/// #     fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
/// #         write!(f, "{}", "expensive")
/// #     }
/// #  }
/// # let expensive_thing = ExpensiveThing();
/// #
/// defmt::info!("{}", defmt::Display2Format(&expensive_thing));
/// //                                        ˆˆˆˆˆˆˆˆˆˆˆˆˆˆˆ
/// //                                        must implement `fmt::Display`
/// ```
///
/// Note that any provided defmt display hints will be ignored
/// because this always uses `{}` to format the contained value.
pub struct Display2Format<'a, T: fmt::Display + ?Sized>(pub &'a T);

impl<T: fmt::Display + ?Sized> fmt::Display for Display2Format<'_, T> {
    fn fmt(&self, fmt: &mut core::fmt::Formatter<'_>) -> Result<(), core::fmt::Error> {
        self.0.fmt(fmt)
    }
}

impl<T: fmt::Display + ?Sized> Format for Display2Format<'_, T> {
    default_format!();

    fn _format_tag() -> Str {
        defmt_macros::internp!("{=__internal_Display}")
    }

    fn _format_data(&self) {
        export::display(&self.0);
    }
}
