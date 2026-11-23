use arbitrary::{Arbitrary, Result, Unstructured};
use std::ops::{
  Bound, Range, RangeBounds, RangeFrom, RangeInclusive, RangeTo,
  RangeToInclusive,
};

#[derive(Debug, Clone, Eq, PartialEq)]
pub enum ArbRange<T> {
  Range(Range<T>),
  RangeFrom(RangeFrom<T>),
  RangeInclusive(RangeInclusive<T>),
  RangeTo(RangeTo<T>),
  RangeToInclusive(RangeToInclusive<T>),
}

impl<T> RangeBounds<T> for ArbRange<T> {
  fn start_bound(&self) -> Bound<&T> {
    match self {
      ArbRange::Range(range) => range.start_bound(),
      ArbRange::RangeFrom(range) => range.start_bound(),
      ArbRange::RangeInclusive(range) => range.start_bound(),
      ArbRange::RangeTo(range) => range.start_bound(),
      ArbRange::RangeToInclusive(range) => range.start_bound(),
    }
  }

  fn end_bound(&self) -> Bound<&T> {
    match self {
      ArbRange::Range(range) => range.end_bound(),
      ArbRange::RangeFrom(range) => range.end_bound(),
      ArbRange::RangeInclusive(range) => range.end_bound(),
      ArbRange::RangeTo(range) => range.end_bound(),
      ArbRange::RangeToInclusive(range) => range.end_bound(),
    }
  }

  fn contains<U: ?Sized>(&self, item: &U) -> bool
  where
    T: PartialOrd<U>,
    U: PartialOrd<T>,
  {
    match self {
      ArbRange::Range(range) => range.contains(item),
      ArbRange::RangeFrom(range) => range.contains(item),
      ArbRange::RangeInclusive(range) => range.contains(item),
      ArbRange::RangeTo(range) => range.contains(item),
      ArbRange::RangeToInclusive(range) => range.contains(item),
    }
  }
}

impl<T: Arbitrary> Arbitrary for ArbRange<T> {
  fn arbitrary(u: &mut Unstructured) -> Result<Self> {
    let variant = u8::arbitrary(u)? % 5;
    Ok(match variant {
      0 => ArbRange::Range(T::arbitrary(u)?..T::arbitrary(u)?),
      1 => ArbRange::RangeFrom(T::arbitrary(u)?..),
      2 => ArbRange::RangeInclusive(T::arbitrary(u)?..=T::arbitrary(u)?),
      3 => ArbRange::RangeTo(..T::arbitrary(u)?),
      4 => ArbRange::RangeToInclusive(..=T::arbitrary(u)?),
      _ => unreachable!(),
    })
  }
}
