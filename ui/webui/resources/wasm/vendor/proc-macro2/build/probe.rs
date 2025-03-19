// This code exercises the surface area that we expect of Span's unstable API.
// If the current toolchain is able to compile it, then proc-macro2 is able to
// offer these APIs too.

#![feature(proc_macro_span)]

extern crate proc_macro;

use core::ops::{Range, RangeBounds};
use proc_macro::{Literal, Span};

pub fn byte_range(this: &Span) -> Range<usize> {
    this.byte_range()
}

pub fn start(this: &Span) -> Span {
    this.start()
}

pub fn end(this: &Span) -> Span {
    this.end()
}

pub fn line(this: &Span) -> usize {
    this.line()
}

pub fn column(this: &Span) -> usize {
    this.column()
}

pub fn join(this: &Span, other: Span) -> Option<Span> {
    this.join(other)
}

pub fn subspan<R: RangeBounds<usize>>(this: &Literal, range: R) -> Option<Span> {
    this.subspan(range)
}

// Include in sccache cache key.
const _: Option<&str> = option_env!("RUSTC_BOOTSTRAP");
