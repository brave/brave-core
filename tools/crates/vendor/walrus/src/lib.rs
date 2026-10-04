//! The `walrus` WebAssembly transformations library.

#![deny(missing_debug_implementations)]
#![deny(missing_docs)]

#[cfg(feature = "parallel")]
macro_rules! maybe_parallel {
    ($e:ident.($serial:ident | $parallel:ident)) => {
        $e.$parallel()
    };
}

#[cfg(not(feature = "parallel"))]
macro_rules! maybe_parallel {
    ($e:ident.($serial:ident | $parallel:ident)) => {
        $e.$serial()
    };
}

mod arena_set;
mod const_expr;
pub mod dot;
mod emit;
mod error;
mod function_builder;
pub mod ir;
mod map;
mod module;
mod parse;
pub mod passes;
mod tombstone_arena;
mod ty;

pub use crate::const_expr::ConstExpr;
pub use crate::emit::IdsToIndices;
pub use crate::error::{ErrorKind, Result};
pub use crate::function_builder::{FunctionBuilder, InstrSeqBuilder};
pub use crate::ir::{Local, LocalId};
pub use crate::module::*;
pub use crate::parse::IndicesToIds;
pub use crate::ty::{RefType, Type, TypeId, ValType};
