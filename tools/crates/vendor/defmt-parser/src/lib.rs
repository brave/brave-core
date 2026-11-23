//! Parsing library for [`defmt`] format strings.
//!
//! This is an implementation detail of [`defmt`] and not meant to be consumed by other tools at the
//! moment so all the API is unstable.
//!
//! [`defmt`]: https://github.com/knurling-rs/defmt

#![cfg_attr(docsrs, feature(doc_cfg))]
#![doc(html_logo_url = "https://knurling.ferrous-systems.com/knurling_logo_light_text.svg")]

mod display_hint;
#[cfg(test)]
mod tests;
mod types;

use std::{borrow::Cow, ops::Range};

pub use crate::{
    display_hint::{DisplayHint, TimePrecision},
    types::Type,
};

/// The kinds of error this library can return
#[derive(thiserror::Error, Debug, PartialEq, Eq, Clone)]
pub enum Error {
    #[error("invalid type specifier `{0:?}`")]
    InvalidTypeSpecifier(String),
    #[error("unable to parse given integer")]
    InvalidInteger(#[from] std::num::ParseIntError),
    #[error("invalid array specifier (missing length)")]
    InvalidArraySpecifierMissingLength,
    #[error("invalid array specifier (missing `]`")]
    InvalidArraySpecifierMissingBracket,
    #[error("trailing data after bitfield range")]
    TrailingDataAfterBitfieldRange,
    #[error("malformed format string (missing display hint after ':')")]
    MalformedFormatString,
    #[error("unknown display hint: {0:?}")]
    UnknownDisplayHint(String),
    #[error("unexpected content `{0:?}` in format string")]
    UnexpectedContentInFormatString(String),
    #[error("unmatched `{{` in format string")]
    UnmatchedOpenBracket,
    #[error("unmatched `}}` in format string")]
    UnmatchedCloseBracket,
    #[error("conflicting types for argument {0}: used as {1:?} and {2:?}")]
    ConflictingTypes(usize, Type, Type),
    #[error("argument {0} is not used in this format string")]
    UnusedArgument(usize),
}

/// A format string that a future release will reject.
///
/// This is *not* a part of the public API.
#[doc(hidden)]
#[derive(thiserror::Error, Debug, PartialEq, Eq, Clone)]
#[non_exhaustive]
pub enum Warning {
    #[error("unmatched `{{` in format string, escape it as `{{{{`: this will be an error in a future release")]
    UnmatchedOpenBracket,
    #[error("unmatched `}}` in format string, escape it as `}}}}`: this will be an error in a future release")]
    UnmatchedCloseBracket,
}

/// A parameter of the form `{{0=Type:hint}}` in a format string.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Parameter {
    /// The argument index to display at this position.
    pub index: usize,
    /// The type of the argument to display, e.g. '=u8', '=bool'.
    pub ty: Type,
    /// The display hint, e.g. ':x', ':b', ':a'.
    pub hint: Option<DisplayHint>,
}

/// A part of a format string.
#[derive(Clone, Debug, Eq, PartialEq)]
pub enum Fragment<'f> {
    /// A literal string (eg. `"literal "` in `"literal {:?}"`).
    Literal(Cow<'f, str>),

    /// A format parameter.
    Parameter(Parameter),
}

/// A parsed formatting parameter (contents of `{` `}` block).
///
/// # Syntax
///
/// ```notrust
/// param := '{' [ argument ] [ '=' argtype ] [ ':' format_spec ] '}'
/// argument := integer
///
/// argtype := bitfield | '?' | format-array | '[?]' | byte-array | '[u8]' | 'istr' | 'str' |
///     'bool' | 'char' | 'u8' | 'u16' | 'u32' | 'u64' | 'u128' | 'usize' | 'i8' | 'i16' | 'i32' |
///     'i64' | 'i128 | 'isize' | 'f32' | 'f64'
/// bitfield := integer '..' integer
/// format-array := '[?;' spaces integer ']'
/// byte-array := '[u8;' spaces integer ']'
/// spaces := ' '*
///
/// format_spec := [ zero_pad ] type
/// zero_pad := '0' integer
/// type := 'a' | 'b' | 'o' | 'x' | 'X' | '?' | 'us'
/// ```
#[derive(Debug, PartialEq)]
struct Param {
    index: Option<usize>,
    ty: Type,
    hint: Option<DisplayHint>,
}

/// The log level
#[derive(Clone, Copy, Debug, Eq, PartialEq, PartialOrd)]
pub enum Level {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
}

impl Level {
    pub fn as_str(self) -> &'static str {
        match self {
            Level::Trace => "trace",
            Level::Debug => "debug",
            Level::Info => "info",
            Level::Warn => "warn",
            Level::Error => "error",
        }
    }
}

fn parse_range(mut s: &str) -> Option<(Range<u8>, usize /* consumed */)> {
    // consume first number
    let start_digits = s
        .as_bytes()
        .iter()
        .take_while(|b| (**b as char).is_ascii_digit())
        .count();
    let start = s[..start_digits].parse().ok()?;

    // next two `char`s should be `..`
    if s.get(start_digits..start_digits + 2) != Some("..") {
        return None;
    }
    s = &s[start_digits + 2..];

    // consume second number
    let end_digits = s
        .as_bytes()
        .iter()
        .take_while(|b| (**b as char).is_ascii_digit())
        .count();
    let end = s[..end_digits].parse().ok()?;

    // check for faulty state
    if end <= start || start >= 128 || end > 128 {
        return None;
    }

    Some((start..end, start_digits + end_digits + 2))
}

/// Parse and consume an array at the beginning of `s`.
///
/// Return the length of the array.
fn parse_array(mut s: &str) -> Result<usize, Error> {
    // skip spaces
    let len_pos = s
        .find(|c: char| c != ' ')
        .ok_or(Error::InvalidArraySpecifierMissingLength)?;
    s = &s[len_pos..];

    // consume length
    let after_len = s
        .find(|c: char| !c.is_ascii_digit())
        .ok_or(Error::InvalidArraySpecifierMissingBracket)?;
    let len = s[..after_len].parse::<usize>()?;
    s = &s[after_len..];

    // consume final `]`
    if s != "]" {
        return Err(Error::InvalidArraySpecifierMissingBracket);
    }

    Ok(len)
}

/// Parser mode
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ParserMode {
    /// Rejects unknown display hints
    Strict,
    /// Accepts unknown display hints
    ForwardsCompatible,
}

/// Parse `Param` from `&str`
///
/// * example `input`: `0=Type:hint` (note: no curly braces)
fn parse_param(mut input: &str, mode: ParserMode) -> Result<Param, Error> {
    const TYPE_PREFIX: &str = "=";
    const HINT_PREFIX: &str = ":";

    // First, optional argument index.
    let mut index = None;
    let index_end = input
        .find(|c: char| !c.is_ascii_digit())
        .unwrap_or(input.len());

    if index_end != 0 {
        index = Some(input[..index_end].parse::<usize>()?);
    }

    // Then, optional type
    let mut ty = Type::default(); // when no explicit type; use the default one
    input = &input[index_end..];

    if input.starts_with(TYPE_PREFIX) {
        // skip the prefix
        input = &input[TYPE_PREFIX.len()..];

        // type is delimited by `HINT_PREFIX` or end-of-string
        let type_end = input.find(HINT_PREFIX).unwrap_or(input.len());
        let type_fragment = &input[..type_end];

        const FORMAT_ARRAY_START: &str = "[?;";
        const U8_ARRAY_START: &str = "[u8;";

        // what comes next is the type
        ty = if let Ok(ty) = type_fragment.parse() {
            Ok(ty)
        } else if let Some(s) = type_fragment.strip_prefix(U8_ARRAY_START) {
            Ok(Type::U8Array(parse_array(s)?))
        } else if let Some(s) = type_fragment.strip_prefix(FORMAT_ARRAY_START) {
            Ok(Type::FormatArray(parse_array(s)?))
        } else if let Some((range, used)) = parse_range(type_fragment) {
            // Check for bitfield syntax.
            match used != type_fragment.len() {
                true => Err(Error::TrailingDataAfterBitfieldRange),
                false => Ok(Type::BitField(range)),
            }
        } else {
            Err(Error::InvalidTypeSpecifier(input.to_owned()))
        }?;

        input = &input[type_end..];
    }

    // Then, optional hint
    let mut hint = None;

    if input.starts_with(HINT_PREFIX) {
        // skip the prefix
        input = &input[HINT_PREFIX.len()..];
        if input.is_empty() {
            return Err(Error::MalformedFormatString);
        }

        hint = match (DisplayHint::parse(input), mode) {
            (Some(a), _) => Some(a),
            (None, ParserMode::Strict) => return Err(Error::UnknownDisplayHint(input.to_owned())),
            (None, ParserMode::ForwardsCompatible) => Some(DisplayHint::Unknown(input.to_owned())),
        };
    } else if !input.is_empty() {
        return Err(Error::UnexpectedContentInFormatString(input.to_owned()));
    }

    Ok(Param { index, ty, hint })
}

/// Rejects a brace that is not doubled, matching what `format!` accepts.
fn check_braces_by_pairing(unescaped_literal: &str) -> Result<(), Warning> {
    let mut chars = unescaped_literal.chars().peekable();
    while let Some(c) = chars.next() {
        if (c == '{' || c == '}') && chars.next_if_eq(&c).is_none() {
            return Err(if c == '{' {
                Warning::UnmatchedOpenBracket
            } else {
                Warning::UnmatchedCloseBracket
            });
        }
    }

    Ok(())
}

/// Rejects a brace kind whose count is odd at a plain character or at the end.
fn check_braces_by_parity(unescaped_literal: &str) -> Result<(), Error> {
    let mut last_open = false;
    let mut last_close = false;
    for c in unescaped_literal.chars() {
        match c {
            '{' => last_open = !last_open,
            '}' => last_close = !last_close,
            _ if last_open => return Err(Error::UnmatchedOpenBracket),
            _ if last_close => return Err(Error::UnmatchedCloseBracket),
            _ => {}
        }
    }

    // Handle trailing unescaped `{` or `}`.
    if last_open {
        return Err(Error::UnmatchedOpenBracket);
    } else if last_close {
        return Err(Error::UnmatchedCloseBracket);
    }

    Ok(())
}

fn push_literal<'f>(
    frag: &mut Vec<Fragment<'f>>,
    unescaped_literal: &'f str,
    warnings: &mut Vec<Warning>,
) -> Result<(), Error> {
    // Replace `{{` with `{` and `}}` with `}`. Single braces are errors.

    // Parity is what this parser has always shipped and accepts strings pairing rejects, such
    // as `}{{}`. Rejecting those outright would break code that compiles today, so parity keeps
    // deciding and pairing only warns until a future release swaps them.
    check_braces_by_parity(unescaped_literal)?;
    if let Err(warning) = check_braces_by_pairing(unescaped_literal) {
        if !warnings.contains(&warning) {
            warnings.push(warning);
        }
    }

    // FIXME: This always allocates a `String`, so the `Cow` is useless.
    let literal = unescaped_literal.replace("{{", "{").replace("}}", "}");
    frag.push(Fragment::Literal(literal.into()));
    Ok(())
}

/// Returns `Some(smallest_bit_index, largest_bit_index)` contained in `params` if
/// `params` contains any bitfields. Otherwise `None`.
pub fn get_max_bitfield_range<'a, I>(params: I) -> Option<(u8, u8)>
where
    I: Iterator<Item = &'a Parameter> + Clone,
{
    let largest_bit_index = params
        .clone()
        .map(|param| match &param.ty {
            Type::BitField(range) => range.end,
            _ => unreachable!(),
        })
        .max();

    let smallest_bit_index = params
        .map(|param| match &param.ty {
            Type::BitField(range) => range.start,
            _ => unreachable!(),
        })
        .min();

    match (smallest_bit_index, largest_bit_index) {
        (Some(smallest), Some(largest)) => Some((smallest, largest)),
        (None, None) => None,
        _ => unreachable!(),
    }
}

pub fn parse(format_string: &str, mode: ParserMode) -> Result<Vec<Fragment<'_>>, Error> {
    parse_with_warnings(format_string, mode).map(|(fragments, _warnings)| fragments)
}

/// Same as [`parse`], additionally reporting each warning once.
///
/// This is *not* a part of the public API.
#[doc(hidden)]
pub fn parse_with_warnings(
    format_string: &str,
    mode: ParserMode,
) -> Result<(Vec<Fragment<'_>>, Vec<Warning>), Error> {
    let mut fragments = Vec::new();
    let mut warnings = Vec::new();

    // Index after the `}` of the last format specifier.
    let mut end_pos = 0;

    // Next argument index assigned to a parameter without an explicit one.
    let mut next_arg_index = 0;

    let mut chars = format_string.char_indices();
    while let Some((brace_pos, ch)) = chars.next() {
        if ch != '{' {
            // Part of a literal fragment.
            continue;
        }

        // Peek at the next char.
        if chars.as_str().starts_with('{') {
            // Escaped `{{`, also part of a literal fragment.
            chars.next(); // Move after both `{`s.
            continue;
        }

        if brace_pos > end_pos {
            // There's a literal fragment with at least 1 character before this parameter fragment.
            let unescaped_literal = &format_string[end_pos..brace_pos];
            push_literal(&mut fragments, unescaped_literal, &mut warnings)?;
        }

        // Else, this is a format specifier. It ends at the next `}`.
        let len = chars
            .as_str()
            .find('}')
            .ok_or(Error::UnmatchedOpenBracket)?;
        end_pos = brace_pos + 1 + len + 1;

        // Parse the contents inside the braces.
        let param_str = &format_string[brace_pos + 1..][..len];
        let param = parse_param(param_str, mode)?;
        fragments.push(Fragment::Parameter(Parameter {
            index: param.index.unwrap_or_else(|| {
                // If there is no explicit index, assign the next one.
                let idx = next_arg_index;
                next_arg_index += 1;
                idx
            }),
            ty: param.ty,
            hint: param.hint,
        }));
    }

    // Trailing literal.
    if end_pos != format_string.len() {
        push_literal(&mut fragments, &format_string[end_pos..], &mut warnings)?;
    }

    // Check for argument type conflicts.
    let mut args = Vec::new();
    for frag in &fragments {
        if let Fragment::Parameter(Parameter { index, ty, .. }) = frag {
            if args.len() <= *index {
                args.resize(*index + 1, None);
            }

            match &args[*index] {
                None => args[*index] = Some(ty.clone()),
                Some(other_ty) => match (other_ty, ty) {
                    (Type::BitField(_), Type::BitField(_)) => {} // FIXME: Bitfield range shouldn't be part of the type.
                    (a, b) if a != b => {
                        return Err(Error::ConflictingTypes(*index, a.clone(), b.clone()))
                    }
                    _ => {}
                },
            }
        }
    }

    // Check that argument indices are dense (all arguments must be used).
    for (index, arg) in args.iter().enumerate() {
        if arg.is_none() {
            return Err(Error::UnusedArgument(index));
        }
    }

    Ok((fragments, warnings))
}
