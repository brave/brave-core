//! Parser for format descriptions.

use crate::FormatDescriptionVersion;

mod ast;
mod format_item;
mod lexer;
pub(crate) mod public;

pub(crate) fn parse_with_version(
    version: FormatDescriptionVersion,
    s: &[u8],
    proc_span: proc_macro::Span,
) -> Result<public::OwnedFormatItem, crate::Error> {
    let mut lexed = lexer::lex(version, s, proc_span);
    let ast = ast::parse(version, &mut lexed);
    let format_items = format_item::parse(ast)
        .map(|res| res.and_then(|item| (version, item).try_into()))
        .collect::<Result<_, _>>()?;

    let mut fd = public::OwnedFormatItem {
        version,
        inner: public::OwnedFormatItemInner::Compound(format_items),
    };
    fd.optimize();

    let top_level_item = if let public::OwnedFormatItemInner::Compound(items) = &mut fd.inner
        && items.len() == 1
        && let Some(item) = items.pop()
    {
        public::OwnedFormatItem {
            version,
            inner: item,
        }
    } else {
        fd
    };

    Ok(top_level_item)
}

#[derive(Debug, Clone, Copy)]
struct Location {
    byte: u32,
    proc_span: proc_macro::Span,
}

impl Location {
    fn to(self, end: Self) -> Span {
        Span { start: self, end }
    }

    #[must_use = "this does not modify the original value"]
    fn offset(&self, offset: u32) -> Self {
        Self {
            byte: self.byte + offset,
            proc_span: self.proc_span,
        }
    }

    fn error(self, message: &'static str) -> Error {
        Error {
            message,
            _span: unused(Span {
                start: self,
                end: self,
            }),
            proc_span: self.proc_span,
        }
    }
}

#[derive(Debug, Clone, Copy)]
struct Span {
    start: Location,
    end: Location,
}

impl Span {
    fn dummy() -> Self {
        Self {
            start: Location {
                byte: u32::MAX,
                proc_span: proc_macro::Span::call_site(),
            },
            end: Location {
                byte: u32::MAX,
                proc_span: proc_macro::Span::call_site(),
            },
        }
    }

    #[must_use = "this does not modify the original value"]
    const fn shrink_to_start(&self) -> Self {
        Self {
            start: self.start,
            end: self.start,
        }
    }

    #[must_use = "this does not modify the original value"]
    const fn shrink_to_end(&self) -> Self {
        Self {
            start: self.end,
            end: self.end,
        }
    }

    fn error(self, message: &'static str) -> Error {
        Error {
            message,
            _span: unused(self),
            proc_span: self.start.proc_span,
        }
    }
}

#[derive(Debug, Clone, Copy)]
struct Spanned<T> {
    value: T,
    span: Span,
}

impl<T> core::ops::Deref for Spanned<T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        &self.value
    }
}

impl<T> Spanned<T> {
    #[inline]
    fn map<F, U>(self, f: F) -> Spanned<U>
    where
        F: FnOnce(T) -> U,
    {
        Spanned {
            value: f(self.value),
            span: self.span,
        }
    }
}

trait OptionExt<T> {
    fn transpose(self) -> Spanned<Option<T>>;
}

impl<T> OptionExt<T> for Option<Spanned<T>> {
    #[inline]
    fn transpose(self) -> Spanned<Option<T>> {
        match self {
            Some(spanned) => Spanned {
                value: Some(spanned.value),
                span: spanned.span,
            },
            None => Spanned {
                value: None,
                span: Span::dummy(),
            },
        }
    }
}

trait SpannedValue: Sized {
    fn spanned(self, span: Span) -> Spanned<Self>;
}

impl<T> SpannedValue for T {
    fn spanned(self, span: Span) -> Spanned<Self> {
        Spanned { value: self, span }
    }
}

#[derive(Debug)]
struct Error {
    message: &'static str,
    _span: Unused<Span>,
    proc_span: proc_macro::Span,
}

impl From<Error> for crate::Error {
    fn from(error: Error) -> Self {
        Self::Custom {
            message: error.message.into(),
            span_start: Some(error.proc_span),
            span_end: Some(error.proc_span),
        }
    }
}

struct Unused<T>(core::marker::PhantomData<T>);

impl<T> core::fmt::Debug for Unused<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_tuple("Unused").finish()
    }
}

fn unused<T>(_: T) -> Unused<T> {
    Unused(core::marker::PhantomData)
}
