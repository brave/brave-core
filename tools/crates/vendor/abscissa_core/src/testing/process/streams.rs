//! Streams (i.e. pipes) for communicating with a subprocess

use crate::testing::Regex;
use std::{
    io::{self, BufRead, BufReader},
    ops::{Deref, DerefMut},
    process::{ChildStderr, ChildStdout},
};

/// Buffered reader for standard output
#[derive(Debug)]
pub struct Stdout(BufReader<ChildStdout>);

/// Buffered reader for standard error
#[derive(Debug)]
pub struct Stderr(BufReader<ChildStderr>);

/// Methods common to output streams
pub trait OutputStream<R>: DerefMut<Target = BufReader<R>>
where
    R: io::Read,
{
    /// Read a line and ensure it matches the expected value.
    ///
    /// Panics if it is not the expected value.
    fn expect_line(&mut self, expected_line: &str) {
        let mut actual_line = String::new();
        self.read_line(&mut actual_line)
            .unwrap_or_else(|e| panic!("error reading line from {}: {}", stringify!($name), e));

        assert_eq!(expected_line, actual_line.trim_end_matches('\n'));
    }

    /// Read a line and test it against the given regex.
    ///
    /// Panics if the line does not match the regex.
    fn expect_regex<T>(&mut self, regex: T)
    where
        T: Into<Regex>,
    {
        let regex: Regex = regex.into();

        let mut line = String::new();
        self.read_line(&mut line)
            .unwrap_or_else(|e| panic!("error reading line from {}: {}", stringify!($name), e));

        assert!(
            regex.is_match(line.trim_end_matches('\n')),
            "regex {:?} did not match line: {:?}",
            regex,
            line
        );
    }
}

macro_rules! impl_output_stream {
    ($name:tt, $inner:ty) => {
        impl $name {
            /// Create standard output wrapper
            pub(super) fn new(stream: $inner) -> $name {
                $name(BufReader::new(stream))
            }
        }

        impl Deref for $name {
            type Target = BufReader<$inner>;

            fn deref(&self) -> &BufReader<$inner> {
                &self.0
            }
        }

        impl DerefMut for $name {
            fn deref_mut(&mut self) -> &mut BufReader<$inner> {
                &mut self.0
            }
        }

        impl OutputStream<$inner> for $name {}
    };
}

impl_output_stream!(Stdout, ChildStdout);
impl_output_stream!(Stderr, ChildStderr);
