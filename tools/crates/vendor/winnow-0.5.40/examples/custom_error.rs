use winnow::error::ErrMode;
use winnow::error::ErrorKind;
use winnow::error::ParserError;
use winnow::prelude::*;

#[derive(Debug, PartialEq, Eq)]
pub enum CustomError<I> {
    MyError,
    Nom(I, ErrorKind),
}

impl<I: Clone> ParserError<I> for CustomError<I> {
    fn from_error_kind(input: &I, kind: ErrorKind) -> Self {
        CustomError::Nom(input.clone(), kind)
    }

    fn append(self, _: &I, _: ErrorKind) -> Self {
        self
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
        match err {
            ErrMode::Backtrack(e) => assert_eq!(e, CustomError::MyError),
            _ => panic!("Unexpected error: {:?}", err),
        }
    }
}
