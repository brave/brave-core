//! This crate provides a parser and common data model over Atom and RSS feeds.
//!
//! It uses [quick-xml](https://crates.io/crates/quick-xml) a light-weight,
//! streaming XML parser to minimise memory usage and avoids copying (clone)
//! where possible.
//!
//! The model attempts to find a balance between:
//! * convenience of a single field where semantic equivalence exists e.g. "description" from RSS 2 and "subtitle" from Atom are semantically equivalent and mapped to the same field
//! * the real world where mandatory fields may not be specified and data may not be in the correct form
//!
//! Thus the parser errs on the side of leniency with the outcome that certain fields are represented as `Option<T>` in the model, even though they may be mandatory in one of the specifications.
//!
//! # Usage
//!
//! The parser consists of a single method (parser::parse) which accepts an stream representing an XML document and returns a Feed.

// TODO review the Rust doc guidelines and fix up links
// TODO improve tests with Coverage analysis e.g. https://github.com/mozilla/grcov

#![forbid(unsafe_code)]
// Standard names like MediaRSS and JSON are used throughout this crate
#![allow(clippy::upper_case_acronyms)]

#[macro_use]
extern crate serde;

mod util;
mod xml;

pub mod model;
pub mod parser;
