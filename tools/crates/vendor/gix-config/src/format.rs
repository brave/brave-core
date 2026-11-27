//! Reformat a git-config file with normalized, sanitized whitespace.
//!
//! This operates purely on the syntactic [event stream](crate::parse::Events) of a single
//! file. `include`/`includeIf` directives are *never* resolved here - those are only acted upon
//! when constructing a [`File`](crate::File) - so the formatter is "flat" by construction.
//!
//! Values, comments and section headers are reproduced verbatim; only insignificant whitespace,
//! newlines and the `=` separator are rewritten according to [`Options`][crate::format::Options].

use bstr::BString;

use crate::parse::{self, EventRef};

/// How lines beneath a section header are indented.
#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum Indent {
    /// The given number of horizontal tabs per line.
    Tabs(usize),
    /// The given number of spaces per line.
    Spaces(usize),
}

/// Which newline sequence to write between lines.
#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub enum Newline {
    /// Use the first newline sequence found in the input, falling back to `\n` if none is present.
    Detect,
    /// Always use a Unix newline (`\n`).
    Lf,
    /// Always use a Windows newline (`\r\n`).
    CrLf,
}

/// Options controlling [`normalize()`].
///
/// The defaults are intentionally conservative: they tidy the common sources of noise (stray
/// indentation, spacing around `=`, trailing whitespace, missing final newline) while leaving
/// blank lines and the substance of the file untouched, following Git defaults as far as these exist.
///
/// Note that trailing whitespace at the end of a line is always removed - it is never significant
/// in git-config syntax - so there is no option to retain it.
#[derive(Debug, Clone, Copy, Eq, PartialEq)]
pub struct Options {
    /// How to indent root-level comments.
    ///
    /// If `None`, their existing indentation is preserved. Defaults to `Some(Indent::Spaces(0))`,
    /// removing all indentation.
    pub root_comment_indent: Option<Indent>,
    /// How to indent key/value and standalone comment lines beneath a section header.
    ///
    /// Inline comments continue to be separated from the preceding content by one space.
    /// Defaults to one tab, like Git's writer.
    pub key_value_indent: Indent,
    /// If `true`, place a single space on each side of the `=` separator (`a = b`);
    /// if `false`, emit a bare `=` (`a=b`).
    ///
    /// Defaults to `true`.
    pub spaces_around_separator: bool,
    /// Which newline sequence to emit between lines. Defaults to [`Newline::Detect`], for best
    /// compatibility. Git's writer uses [Newline::Lf].
    pub newline: Newline,
    /// If `true`, ensure a non-empty file ends with exactly one newline. Defaults to `true`.
    pub ensure_trailing_newline: bool,
    /// If `Some(n)`, cap the run of blank lines before the first content at `n`.
    ///
    /// Defaults to `None`, leaving leading blank lines exactly as they are.
    pub max_leading_blank_lines: Option<usize>,
    /// If `Some(n)`, cap runs of consecutive blank lines after the first content at `n`.
    ///
    /// Defaults to `None`, leaving blank lines exactly as they are.
    pub max_consecutive_blank_lines: Option<usize>,
}

impl Default for Options {
    fn default() -> Self {
        Options {
            key_value_indent: Indent::Tabs(1),
            root_comment_indent: Some(Indent::Spaces(0)),
            spaces_around_separator: true,
            newline: Newline::Detect,
            ensure_trailing_newline: true,
            max_leading_blank_lines: None,
            max_consecutive_blank_lines: None,
        }
    }
}

/// Parse `input` as a single git-config file and return it with whitespace normalized per `options`.
///
/// Includes are never resolved. Values, comments and section headers are preserved byte-for-byte;
/// only insignificant whitespace, newlines and the `=` separator are rewritten.
///
/// # Errors
///
/// Returns a [`parse::Error`] if `input` is not a syntactically valid git-config file.
pub fn normalize(input: &[u8], options: Options) -> Result<BString, parse::Error> {
    let parsed = parse::Events::from_bytes(input, None)?;
    let events: Vec<_> = parsed.iter().collect();
    Ok(normalize_events(&events, options))
}

/// Return the style of the first newline in `events`, falling back to LF if there is none.
fn detect_newline(events: &[EventRef<'_>]) -> &'static [u8] {
    for event in events {
        if let EventRef::Newline(n) = event {
            return if n.contains(&b'\r') { b"\r\n" } else { b"\n" };
        }
    }
    b"\n"
}

fn indentation(indent: Indent) -> Vec<u8> {
    match indent {
        Indent::Tabs(n) => vec![b'\t'; n],
        Indent::Spaces(n) => vec![b' '; n],
    }
}

fn normalize_events(
    events: &[EventRef<'_>],
    Options {
        root_comment_indent,
        key_value_indent,
        spaces_around_separator,
        newline,
        ensure_trailing_newline,
        max_leading_blank_lines,
        max_consecutive_blank_lines,
    }: Options,
) -> BString {
    let newline: &[u8] = match newline {
        Newline::Detect => detect_newline(events),
        Newline::Lf => b"\n",
        Newline::CrLf => b"\r\n",
    };
    let key_value_indent = indentation(key_value_indent);
    let root_comment_indent = root_comment_indent.map(indentation);

    let mut out: Vec<u8> = Vec::with_capacity(events.len() * 8);
    let mut in_section = false;
    let mut line_has_content = false;
    let mut has_seen_content = false;
    let mut consecutive_blank_lines = 0usize;
    let mut events = events.iter().copied().peekable();

    while let Some(event) = events.next() {
        match event {
            // Standalone, insignificant whitespace is dropped; we synthesize whitespace
            // deterministically around the structural events below.
            EventRef::Whitespace(_) => {
                let precedes_root_comment = !in_section
                    && !line_has_content
                    && events
                        .peek()
                        .is_some_and(|event| matches!(event, EventRef::Comment { .. }));
                if precedes_root_comment && root_comment_indent.is_none() {
                    event.write_to(&mut out).expect("write to Vec is infallible");
                }
            }
            EventRef::SectionHeader { .. } => {
                event.write_to(&mut out).expect("write to Vec is infallible");
                in_section = true;
                line_has_content = true;
                consecutive_blank_lines = 0;
            }
            EventRef::SectionValueName(_) => {
                if in_section && !line_has_content {
                    out.extend_from_slice(&key_value_indent);
                }
                event.write_to(&mut out).expect("write to Vec is infallible");
                line_has_content = true;
                consecutive_blank_lines = 0;
            }
            EventRef::KeyValueSeparator => {
                if spaces_around_separator {
                    out.extend_from_slice(b" = ");
                } else {
                    out.push(b'=');
                }
                line_has_content = true;
                consecutive_blank_lines = 0;
            }
            EventRef::Value(_) | EventRef::ValueNotDone(_) | EventRef::ValueDone(_) => {
                event.write_to(&mut out).expect("write to Vec is infallible");
                line_has_content = true;
                consecutive_blank_lines = 0;
            }
            EventRef::Comment { tag, text } => {
                if line_has_content {
                    // Inline comment trailing a value/header: one space before the marker.
                    out.push(b' ');
                } else if in_section {
                    out.extend_from_slice(&key_value_indent);
                } else if let Some(indent) = &root_comment_indent {
                    out.extend_from_slice(indent);
                }
                out.push(tag);
                let text: &[u8] = text.as_ref();
                let newline_follows = events.peek().is_some_and(|event| matches!(event, EventRef::Newline(_)));
                out.extend_from_slice(if newline_follows {
                    text.strip_suffix(b"\r").unwrap_or(text)
                } else {
                    text
                });
                line_has_content = true;
                consecutive_blank_lines = 0;
            }
            EventRef::Newline(n) => {
                for _ in 0..n.iter().filter(|&&b| b == b'\n').count() {
                    let is_blank_line = !line_has_content;
                    let max_blank_lines = if has_seen_content {
                        max_consecutive_blank_lines
                    } else {
                        max_leading_blank_lines
                    };
                    let should_emit =
                        max_blank_lines.is_none_or(|max_blank| !is_blank_line || consecutive_blank_lines < max_blank);
                    if should_emit {
                        out.extend_from_slice(newline);
                    }
                    if is_blank_line {
                        consecutive_blank_lines = consecutive_blank_lines.saturating_add(1);
                    } else {
                        has_seen_content = true;
                        consecutive_blank_lines = 0;
                    }
                    line_has_content = false;
                }
            }
        }
    }

    if ensure_trailing_newline && !out.is_empty() {
        while out.last() == Some(&b'\n') || out.last() == Some(&b'\r') {
            out.pop();
        }
        out.extend_from_slice(newline);
    }

    out.into()
}
