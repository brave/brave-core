use bstr::ByteSlice;

/// The information usually found in remote progress messages as sent by a git server during
/// fetch, clone and push operations.
#[derive(PartialEq, Eq, Debug, Hash, Ord, PartialOrd, Clone, Copy)]
#[cfg_attr(feature = "serde", derive(serde::Serialize, serde::Deserialize))]
pub struct RemoteProgress<'a> {
    #[cfg_attr(feature = "serde", serde(borrow))]
    /// The name of the action, like "clone".
    pub action: &'a bstr::BStr,
    /// The percentage to indicate progress, between 0 and 100.
    pub percent: Option<u32>,
    /// The amount of items already processed.
    pub step: Option<usize>,
    /// The maximum expected amount of items. `step` / `max` * 100 = `percent`.
    pub max: Option<usize>,
}

impl RemoteProgress<'_> {
    /// Parse the progress from a typical git progress `line` as sent by the remote.
    pub fn from_bytes(mut line: &[u8]) -> Option<RemoteProgress<'_>> {
        parse_progress(&mut line)
            .ok()
            .filter(|&r| !(r.percent.is_none() && r.step.is_none() && r.max.is_none()))
    }

    /// Parse `text`, which is interpreted as error if `is_error` is true, as [`RemoteProgress`] and call the respective
    /// methods on the given `progress` instance.
    pub fn translate_to_progress(is_error: bool, text: &[u8], progress: &mut impl gix_utils::progress::Progress) {
        fn progress_name(current: Option<String>, action: &[u8]) -> String {
            match current {
                Some(current) => format!(
                    "{}: {}",
                    current.split_once(':').map_or(&*current, |x| x.0),
                    action.as_bstr()
                ),
                None => action.as_bstr().to_string(),
            }
        }
        if is_error {
            // ignore keep-alive packages sent with 'sideband-all'
            if !text.is_empty() {
                progress.fail(progress_name(None, text));
            }
        } else {
            match RemoteProgress::from_bytes(text) {
                Some(RemoteProgress {
                    action,
                    percent: _,
                    step,
                    max,
                }) => {
                    progress.set_name(progress_name(progress.name(), action));
                    progress.init(max, gix_utils::progress::count("objects"));
                    if let Some(step) = step {
                        progress.set(step);
                    }
                }
                None => progress.set_name(progress_name(progress.name(), text)),
            }
        }
    }
}

/// Parse a non-empty prefix of ASCII decimal digits as an unsigned number.
///
/// On success, `i` is advanced past the parsed digits and the parsed value is
/// returned. If there are no digits at the current position, `None` is
/// returned. If the digit prefix cannot be represented as `usize`, `i` is
/// advanced anyway to avoid retrying the same input and `None` is returned.
fn parse_number(i: &mut &[u8]) -> Option<usize> {
    let len = i.iter().take_while(|b| b.is_ascii_digit()).count();
    if len == 0 {
        return None;
    }
    let (number, rest) = i.split_at(len);
    *i = rest;
    gix_utils::btoi::to_signed(number).ok()
}

/// Advance `i` to the first ASCII digit in the remaining input.
///
/// If no digit is present, `i` is advanced to the end of the input.
/// If `i` already starts with a digit, it is left unchanged.
fn skip_until_digit_or_to_end(i: &mut &[u8]) {
    let pos = i.iter().position(u8::is_ascii_digit).unwrap_or(i.len());
    *i = &i[pos..];
}

/// Find and parse the next ASCII decimal number only if it is followed by `%`.
///
/// For example, `b" 42% (21/50)"` yields `Some(42)` and advances `i` to
/// `b" (21/50)"`, while `b" (21/50)"` yields `None` because the next number is
/// not a percentage. `b" done"` yields `None` with `i` fully consumed, as there
/// are no digits left to parse.
///
/// If the digit prefix cannot be represented as `u32`, it is treated as
/// absent and `None` is returned with `i` advanced past all consumed bytes.
fn next_optional_percentage(i: &mut &[u8]) -> Option<u32> {
    let before = *i;
    skip_until_digit_or_to_end(i);
    let number = parse_number(i)?;
    if let Some(rest) = i.strip_prefix(b"%") {
        *i = rest;
        u32::try_from(number).ok()
    } else {
        *i = before;
        None
    }
}

/// Find and parse the next ASCII decimal number, if one is present.
///
/// For example, `b" (21/50)"` yields `Some(21)` and advances `i` to `b"/50)"`.
/// Calling it again on that remainder yields `Some(50)` and advances `i` to
/// `b")"`. If no digit is present, it yields `None` and advances `i` to the
/// empty suffix.
///
/// If the next digit prefix cannot be represented as `usize`, it is treated as
/// absent and `None` is returned. In that case, `i` is advanced past the digit
/// prefix because [`parse_number`] consumes it before conversion.
fn next_optional_number(i: &mut &[u8]) -> Option<usize> {
    skip_until_digit_or_to_end(i);
    parse_number(i)
}

/// Parse a remote progress line with a non-empty action followed by `:`.
///
/// The remainder is scanned leniently for the common progress fields emitted by
/// git servers: an optional percentage, then up to two optional numbers for the
/// current step and maximum. For example, inputs like
/// `b"Receiving objects:  42% (21/50)"` and `b"Resolving deltas: 21/50"` can
/// produce an action plus `percent`, `step`, and `max` values.
///
/// `line` is advanced as the fields are found. If parsing succeeds, it points at
/// the unconsumed suffix after the parsed progress fields. Inputs without a
/// colon, or with an empty action before the colon, return an error.
fn parse_progress<'i>(line: &mut &'i [u8]) -> Result<RemoteProgress<'i>, ()> {
    let action_end = line.iter().position(|b| *b == b':').ok_or(())?;
    if action_end == 0 {
        return Err(());
    }
    let action = &line[..action_end];
    *line = &line[action_end..];
    let percent = next_optional_percentage(line);
    let step = next_optional_number(line);
    let max = next_optional_number(line);
    Ok(RemoteProgress {
        action: action.into(),
        percent,
        step,
        max,
    })
}
