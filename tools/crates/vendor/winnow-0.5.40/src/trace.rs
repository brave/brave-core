//! Deprecated, replaced with [`winnow::combinator`][crate::combinator]

/// Deprecated, replaced with [`winnow::combinator::trace`][crate::combinator::trace]
#[deprecated(since = "0.5.35", note = "Replaced with `winnow::combinator::trace`")]
#[inline(always)]
pub fn trace<I: crate::stream::Stream, O, E>(
    name: impl crate::lib::std::fmt::Display,
    parser: impl crate::Parser<I, O, E>,
) -> impl crate::Parser<I, O, E> {
    crate::combinator::trace(name, parser)
}
