use winnow::error::AddContext;
use winnow::error::ErrMode;
use winnow::error::ErrorKind;
use winnow::error::FromExternalError;
use winnow::error::ParserError;
use winnow::prelude::*;
use winnow::stream::Stream;

#[derive(Debug)]
pub enum CustomError<I> {
    MyError,
    Winnow(I, ErrorKind),
    External {
        cause: Box<dyn std::error::Error + Send + Sync + 'static>,
        input: I,
        kind: ErrorKind,
    },
}

impl<I: Stream + Clone> ParserError<I> for CustomError<I> {
    fn from_error_kind(input: &I, kind: ErrorKind) -> Self {
        CustomError::Winnow(input.clone(), kind)
    }

    fn append(self, _: &I, _: &<I as Stream>::Checkpoint, _: ErrorKind) -> Self {
        self
    }
}

impl<C, I: Stream> AddContext<I, C> for CustomError<I> {
    #[inline]
    fn add_context(
        self,
        _input: &I,
        _token_start: &<I as Stream>::Checkpoint,
        _context: C,
    ) -> Self {
        self
    }
}

impl<I: Stream + Clone, E: std::error::Error + Send + Sync + 'static> FromExternalError<I, E>
    for CustomError<I>
{
    #[inline]
    fn from_external_error(input: &I, kind: ErrorKind, e: E) -> Self {
        CustomError::External {
            cause: Box::new(e),
            input: input.clone(),
            kind,
        }
    }
}

pub fn parse<'s>(_input: &mut &'s str) -> PResult<&'s str, CustomError<&'s str>> {
    Err(ErrMode::Backtrack(CustomError::MyError))
}

fn main() {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let err = parse.parse_next(&mut "").unwrap_err();
        assert!(matches!(err, ErrMode::Backtrack(CustomError::MyError)));
    }
}
