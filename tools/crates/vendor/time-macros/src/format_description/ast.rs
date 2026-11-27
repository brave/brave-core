use std::iter;

use super::{Error, Location, Spanned, SpannedValue, Unused, lexer, unused};
use crate::FormatDescriptionVersion;
use crate::format_description::Span;

pub(super) enum Item<'a> {
    Literal {
        version: FormatDescriptionVersion,
        value: Spanned<&'a [u8]>,
    },
    Component {
        version: FormatDescriptionVersion,
        opening_bracket: Location,
        _leading_whitespace: Unused<Option<Spanned<&'a str>>>,
        name: Spanned<&'a str>,
        modifiers: Vec<Modifier<'a>>,
        nested_format_descriptions: Vec<NestedFormatDescription<'a>>,
        _trailing_whitespace: Unused<Option<Spanned<&'a str>>>,
        closing_bracket: Location,
    },
}

pub(super) struct NestedFormatDescription<'a> {
    pub(super) leading_whitespace: Option<Spanned<&'a str>>,
    pub(super) opening_bracket: Location,
    pub(super) items: Vec<Item<'a>>,
    pub(super) closing_bracket: Location,
}

#[derive(Debug)]
pub(super) struct Modifier<'a> {
    pub(super) _leading_whitespace: Unused<Spanned<&'a str>>,
    pub(super) key: Spanned<&'a str>,
    pub(super) _colon: Unused<Location>,
    pub(super) value: Spanned<&'a str>,
}

impl<'a> Modifier<'a> {
    fn from_leading_whitespace_and_token(
        leading_whitespace: Spanned<&'a str>,
        token: Spanned<&'a str>,
    ) -> Result<Self, Error> {
        let Some(colon_index) = token.bytes().position(|b| b == b':') else {
            return Err(token.span.error("modifier must be of the form `key:value`"));
        };
        let key = &token[..colon_index];
        let value = &token[colon_index + 1..];

        if key.is_empty() {
            return Err(token.span.shrink_to_start().error("expected modifier key"));
        }
        if value.is_empty() {
            return Err(token.span.shrink_to_end().error("expected modifier value"));
        }

        Ok(Self {
            _leading_whitespace: unused(leading_whitespace),
            key: key.spanned(token.span),
            _colon: unused(token.span.start.offset(colon_index as u32)),
            value: value.spanned(token.span),
        })
    }

    pub(super) fn key_value_span(&self) -> Span {
        self.key.span.start.to(self.value.span.end)
    }
}

pub(super) fn parse<'item: 'iter, 'iter, I: Iterator<Item = Result<lexer::Token<'item>, Error>>>(
    version: FormatDescriptionVersion,
    tokens: &'iter mut lexer::Lexed<I>,
) -> impl Iterator<Item = Result<Item<'item>, Error>> + use<'item, 'iter, I> {
    parse_inner(version, false, tokens)
}

fn parse_inner<'item, I: Iterator<Item = Result<lexer::Token<'item>, Error>>>(
    version: FormatDescriptionVersion,
    nested: bool,
    tokens: &mut lexer::Lexed<I>,
) -> impl Iterator<Item = Result<Item<'item>, Error>> + use<'_, 'item, I> {
    iter::from_fn(move || {
        if nested && tokens.peek_closing_bracket().is_some() {
            return None;
        }

        let next = match tokens.next()? {
            Ok(token) => token,
            Err(err) => return Some(Err(err)),
        };

        Some(match next {
            lexer::Token::Literal(Spanned { value: _, span: _ }) if nested => {
                bug!("literal should not be present in nested description")
            }
            lexer::Token::Literal(value) => Ok(Item::Literal { version, value }),
            lexer::Token::Bracket {
                kind: lexer::BracketKind::Opening,
                location,
            } => {
                if version.is_v1()
                    && let Some(second_location) = tokens.next_if_opening_bracket()
                {
                    Ok(Item::Literal {
                        version,
                        value: b"[".as_slice().spanned(location.to(second_location)),
                    })
                } else {
                    parse_component(version, location, tokens)
                }
            }
            lexer::Token::Bracket {
                kind: lexer::BracketKind::Closing,
                location: _,
            } if nested => {
                bug!("closing bracket should be caught by the `if` statement")
            }
            lexer::Token::Bracket {
                kind: lexer::BracketKind::Closing,
                location: _,
            } => {
                bug!("closing bracket should have been consumed by `parse_component`")
            }
            lexer::Token::ComponentPart { kind: _, value } if nested => Ok(Item::Literal {
                version,
                value: value.map(str::as_bytes),
            }),
            lexer::Token::ComponentPart { kind: _, value: _ } => {
                bug!("component part should have been consumed by `parse_component`")
            }
        })
    })
}

struct Modifiers<'a> {
    modifiers: Vec<Modifier<'a>>,
    trailing_whitespace: Option<Spanned<&'a str>>,
}

impl<'a> Modifiers<'a> {
    /// Parse modifiers until there are none left. Returns any trailing whitespace after the last
    /// modifier.
    fn parse<I>(tokens: &mut lexer::Lexed<I>) -> Result<Self, Error>
    where
        I: Iterator<Item = Result<lexer::Token<'a>, Error>>,
    {
        let mut modifiers = Vec::new();
        loop {
            let Some(whitespace) = tokens.next_if_whitespace() else {
                return Ok(Self {
                    modifiers,
                    trailing_whitespace: None,
                });
            };

            let Some(token) = tokens.next_if_not_whitespace() else {
                return Ok(Self {
                    modifiers,
                    trailing_whitespace: Some(whitespace),
                });
            };

            let modifier = Modifier::from_leading_whitespace_and_token(whitespace, token)?;
            modifiers.push(modifier);
        }
    }

    fn span(&self) -> Span {
        match &*self.modifiers {
            [] => Span::dummy(),
            [modifier] => modifier.key.span.start.to(modifier.value.span.end),
            [first, .., last] => first.key.span.start.to(last.value.span.end),
        }
    }
}

fn parse_component<'a, I: Iterator<Item = Result<lexer::Token<'a>, Error>>>(
    version: FormatDescriptionVersion,
    opening_bracket: Location,
    tokens: &mut lexer::Lexed<I>,
) -> Result<Item<'a>, Error> {
    let leading_whitespace = tokens.next_if_whitespace();

    let Some(name) = tokens.next_if_not_whitespace() else {
        let span = match leading_whitespace {
            Some(Spanned { value: _, span }) => span,
            None => opening_bracket.to(opening_bracket),
        };
        return Err(span.error("expected component name"));
    };

    let modifiers = Modifiers::parse(tokens)?;

    let mut nested_format_descriptions = Vec::new();
    while let Ok(description) = parse_nested(version, modifiers.span().end, tokens) {
        nested_format_descriptions.push(description);
    }

    if modifiers.trailing_whitespace.is_some()
        && let Some(first_nested) = nested_format_descriptions.first_mut()
    {
        first_nested.leading_whitespace = modifiers.trailing_whitespace;
    }

    let nested_fds_trailing_whitespace =
        if modifiers.trailing_whitespace.is_some() && nested_format_descriptions.is_empty() {
            modifiers.trailing_whitespace
        } else {
            tokens.next_if_whitespace()
        };

    let Some(closing_bracket) = tokens.next_if_closing_bracket() else {
        return Err(opening_bracket.error("unclosed bracket"));
    };

    Ok(Item::Component {
        version,
        opening_bracket,
        _leading_whitespace: unused(leading_whitespace),
        name,
        modifiers: modifiers.modifiers,
        nested_format_descriptions,
        _trailing_whitespace: unused(nested_fds_trailing_whitespace),
        closing_bracket,
    })
}

fn parse_nested<'a, I: Iterator<Item = Result<lexer::Token<'a>, Error>>>(
    version: FormatDescriptionVersion,
    last_location: Location,
    tokens: &mut lexer::Lexed<I>,
) -> Result<NestedFormatDescription<'a>, Error> {
    let leading_whitespace = tokens.next_if_whitespace();
    let Some(opening_bracket) = tokens.next_if_opening_bracket() else {
        return Err(last_location.error("expected opening bracket"));
    };
    let items = parse_inner(version, true, tokens).collect::<Result<_, _>>()?;
    let Some(closing_bracket) = tokens.next_if_closing_bracket() else {
        return Err(opening_bracket.error("unclosed bracket"));
    };

    Ok(NestedFormatDescription {
        leading_whitespace,
        opening_bracket,
        items,
        closing_bracket,
    })
}
