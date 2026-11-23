use core::iter;

use super::{Error, Location, Spanned, SpannedValue};
use crate::FormatDescriptionVersion;

pub(super) struct Lexed<I: Iterator> {
    iter: iter::Peekable<I>,
}

impl<I: Iterator> Iterator for Lexed<I> {
    type Item = I::Item;

    fn next(&mut self) -> Option<Self::Item> {
        self.iter.next()
    }
}

impl<'iter, 'token: 'iter, I: Iterator<Item = Result<Token<'token>, Error>> + 'iter> Lexed<I> {
    pub(super) fn peek(&mut self) -> Option<&I::Item> {
        self.iter.peek()
    }

    pub(super) fn next_if_whitespace(&mut self) -> Option<Spanned<&'token str>> {
        if let Some(&Ok(Token::ComponentPart {
            kind: ComponentKind::Whitespace,
            value,
        })) = self.peek()
        {
            self.next(); // consume
            Some(value)
        } else {
            None
        }
    }

    pub(super) fn next_if_not_whitespace(&mut self) -> Option<Spanned<&'token str>> {
        if let Some(&Ok(Token::ComponentPart {
            kind: ComponentKind::NotWhitespace,
            value,
        })) = self.peek()
        {
            self.next();
            Some(value)
        } else {
            None
        }
    }

    pub(super) fn next_if_opening_bracket(&mut self) -> Option<Location> {
        if let Some(&Ok(Token::Bracket {
            kind: BracketKind::Opening,
            location,
        })) = self.peek()
        {
            self.next();
            Some(location)
        } else {
            None
        }
    }

    pub(super) fn peek_closing_bracket(&'iter mut self) -> Option<&'iter Location> {
        if let Some(Ok(Token::Bracket {
            kind: BracketKind::Closing,
            location,
        })) = self.peek()
        {
            Some(location)
        } else {
            None
        }
    }

    pub(super) fn next_if_closing_bracket(&mut self) -> Option<Location> {
        if let Some(&Ok(Token::Bracket {
            kind: BracketKind::Closing,
            location,
        })) = self.peek()
        {
            self.next();
            Some(location)
        } else {
            None
        }
    }
}

pub(super) enum Token<'a> {
    Literal(Spanned<&'a [u8]>),
    Bracket {
        kind: BracketKind,
        location: Location,
    },
    ComponentPart {
        kind: ComponentKind,
        value: Spanned<&'a str>,
    },
}

impl std::fmt::Debug for Token<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Literal(arg0) => f
                .debug_tuple("Literal")
                .field(&String::from_utf8_lossy(arg0))
                .finish(),
            Self::Bracket { kind, location } => f
                .debug_struct("Bracket")
                .field("kind", kind)
                .field("location", location)
                .finish(),
            Self::ComponentPart { kind, value } => f
                .debug_struct("ComponentPart")
                .field("kind", kind)
                .field("value", value)
                .finish(),
        }
    }
}

#[derive(Debug)]
pub(super) enum BracketKind {
    Opening,
    Closing,
}

#[derive(Debug)]
pub(super) enum ComponentKind {
    Whitespace,
    NotWhitespace,
}

pub(super) fn lex(
    version: FormatDescriptionVersion,
    mut input: &[u8],
    proc_span: proc_macro::Span,
) -> Lexed<impl Iterator<Item = Result<Token<'_>, Error>>> {
    let mut depth: u32 = 0;
    let mut byte_pos: u32 = 0;
    let mut nested_component_name_seen = false;
    let mut second_bracket_location = None;

    let iter = iter::from_fn(move || {
        if version.is_v1()
            && let Some(location) = second_bracket_location.take()
        {
            return Some(Ok(Token::Bracket {
                kind: BracketKind::Opening,
                location,
            }));
        }

        let byte = *input.first()?;
        let location = Location {
            byte: byte_pos,
            proc_span,
        };

        Some(Ok(match byte {
            b'\\' if version.is_at_least_v2() => {
                let backslash_loc = location;
                match input.get(1) {
                    Some(b'\\' | b'[' | b']') => {
                        let char_loc = Location {
                            byte: byte_pos + 1,
                            proc_span,
                        };
                        // Safety: We know that the character is either a left bracket, a right
                        // bracket, or a backslash.
                        let char = unsafe { str::from_utf8_unchecked(&input[1..2]) };
                        input = &input[2..];
                        byte_pos += 2;
                        if depth == 0 {
                            Token::Literal(char.as_bytes().spanned(backslash_loc.to(char_loc)))
                        } else {
                            Token::ComponentPart {
                                kind: ComponentKind::NotWhitespace,
                                value: char.spanned(backslash_loc.to(char_loc)),
                            }
                        }
                    }
                    Some(_) => {
                        let loc = Location {
                            byte: byte_pos + 1,
                            proc_span,
                        };
                        return Some(Err(loc.error("invalid escape sequence")));
                    }
                    None => {
                        return Some(Err(backslash_loc.error("unexpected end of input")));
                    }
                }
            }
            b'[' if version.is_v1() && !nested_component_name_seen => {
                if input.get(1) == Some(&b'[') {
                    let second_location = Location {
                        byte: byte_pos + 1,
                        proc_span,
                    };
                    second_bracket_location = Some(second_location);
                    input = &input[2..];
                    byte_pos += 2;
                } else {
                    depth += 1;
                    input = &input[1..];
                    byte_pos += 1;
                }

                Token::Bracket {
                    kind: BracketKind::Opening,
                    location,
                }
            }
            b'[' => {
                depth += 1;
                input = &input[1..];
                byte_pos += 1;

                Token::Bracket {
                    kind: BracketKind::Opening,
                    location,
                }
            }
            b']' if depth > 0 => {
                depth -= 1;
                if version.is_v1() {
                    nested_component_name_seen = depth != 0;
                }
                input = &input[1..];
                byte_pos += 1;

                Token::Bracket {
                    kind: BracketKind::Closing,
                    location,
                }
            }
            _ if depth == 0 => {
                let mut bytes: u32 = 1;
                let mut end_location = location;

                while let Some(&next_byte) = input.get(bytes as usize) {
                    if (version.is_at_least_v2() && next_byte == b'\\') || next_byte == b'[' {
                        break;
                    }
                    end_location = Location {
                        byte: byte_pos + bytes,
                        proc_span,
                    };
                    bytes += 1;
                }

                let value = &input[..bytes as usize];
                input = &input[bytes as usize..];
                byte_pos += bytes;

                Token::Literal(value.spanned(location.to(end_location)))
            }
            byte => {
                let mut bytes: u32 = 1;
                let mut end_location = location;
                let is_whitespace = byte.is_ascii_whitespace();

                while let Some(&next_byte) = input.get(bytes as usize) {
                    if matches!(next_byte, b'\\' | b'[' | b']')
                        || is_whitespace != next_byte.is_ascii_whitespace()
                    {
                        break;
                    }
                    end_location = Location {
                        byte: byte_pos + bytes,
                        proc_span,
                    };
                    bytes += 1;
                }

                let Ok(value) = str::from_utf8(&input[..bytes as usize]) else {
                    return Some(Err(location.error("components must be valid UTF-8")));
                };
                input = &input[bytes as usize..];
                byte_pos += bytes;

                if version.is_v1() && !is_whitespace {
                    nested_component_name_seen = true;
                }

                Token::ComponentPart {
                    kind: if is_whitespace {
                        ComponentKind::Whitespace
                    } else {
                        ComponentKind::NotWhitespace
                    },
                    value: value.spanned(location.to(end_location)),
                }
            }
        }))
    });

    Lexed {
        iter: iter.peekable(),
    }
}
