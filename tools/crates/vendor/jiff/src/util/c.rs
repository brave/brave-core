/*!
A module for constants and various base utilities.

This module is a work-in-progress that may lead to helping us move off of
ranged integers. I'm not quite sure where this will go.
*/

use crate::util::t;

/// A representation of a numeric sign.
///
/// Its `Display` impl emits the ASCII minus sign, `-` when this
/// is negative. It emits the empty string in all other cases.
#[derive(
    Clone, Copy, Debug, Default, Eq, Hash, PartialEq, PartialOrd, Ord,
)]
#[repr(i8)]
pub(crate) enum Sign {
    #[default]
    Zero = 0,
    Positive = 1,
    Negative = -1,
}

impl Sign {
    /*
    pub(crate) fn is_zero(&self) -> bool {
        matches!(*self, Sign::Zero)
    }

    pub(crate) fn is_positive(&self) -> bool {
        matches!(*self, Sign::Positive)
    }
    */

    pub(crate) fn is_negative(&self) -> bool {
        matches!(*self, Sign::Negative)
    }

    pub(crate) fn as_ranged_integer(&self) -> t::Sign {
        match *self {
            Sign::Zero => t::Sign::N::<0>(),
            Sign::Positive => t::Sign::N::<1>(),
            Sign::Negative => t::Sign::N::<-1>(),
        }
    }
}

impl core::fmt::Display for Sign {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        if self.is_negative() {
            write!(f, "-")
        } else {
            Ok(())
        }
    }
}
