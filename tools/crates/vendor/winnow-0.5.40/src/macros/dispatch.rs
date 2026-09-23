/// `match` for parsers
///
/// When parsers have unique prefixes to test for, this offers better performance over
/// [`alt`][crate::combinator::alt] though it might be at the cost of duplicating parts of your grammar
/// if you needed to [`peek`][crate::combinator::peek].
///
/// For tight control over the error in a catch-all case, use [`fail`][crate::combinator::fail].
///
/// # Example
///
/// ```rust
/// use winnow::prelude::*;
/// use winnow::combinator::dispatch;
/// # use winnow::token::any;
/// # use winnow::combinator::peek;
/// # use winnow::combinator::preceded;
/// # use winnow::combinator::empty;
/// # use winnow::combinator::fail;
///
/// fn escaped(input: &mut &str) -> PResult<char> {
///     preceded('\\', escape_seq_char).parse_next(input)
/// }
///
/// fn escape_seq_char(input: &mut &str) -> PResult<char> {
///     dispatch! {any;
///         'b' => empty.value('\u{8}'),
///         'f' => empty.value('\u{c}'),
///         'n' => empty.value('\n'),
///         'r' => empty.value('\r'),
///         't' => empty.value('\t'),
///         '\\' => empty.value('\\'),
///         '"' => empty.value('"'),
///         _ => fail::<_, char, _>,
///     }
///     .parse_next(input)
/// }
///
/// assert_eq!(escaped.parse_peek("\\nHello"), Ok(("Hello", '\n')));
/// ```
#[macro_export]
#[doc(hidden)] // forced to be visible in intended location
macro_rules! dispatch {
    ($match_parser: expr; $( $pat:pat $(if $pred:expr)? => $expr: expr ),+ $(,)? ) => {
        $crate::combinator::trace("dispatch", move |i: &mut _|
        {
            use $crate::Parser;
            let initial = $match_parser.parse_next(i)?;
            match initial {
                $(
                    $pat $(if $pred)? => $expr.parse_next(i),
                )*
            }
        })
    }
}
