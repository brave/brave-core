//! # Custom Errors
//!
//! Between [`ContextError`], [`Parser::context`], and [`cut_err`],
//! most error needs will likely be met
//! (see [tutorial][chapter_6]).
//! When that isn't the case, you can implement your own error type.
//!
//! The most basic error trait is [`ParserError`].
//!
//! Optional traits include:
//! - [`AddContext`]
//! - [`FromExternalError`]
//!
//! # Example
//!
//!```rust
#![doc = include_str!("../../examples/custom_error.rs")]
//!```

#![allow(unused_imports)]
use crate::combinator::cut_err;
use crate::error::ContextError;
use crate::Parser;
use crate::_tutorial::chapter_6;
use crate::error::AddContext;
use crate::error::FromExternalError;
use crate::error::ParserError;
