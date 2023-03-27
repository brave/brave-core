// Assumes proc-macro2's "span-locations" feature is off.

#![cfg(target_pointer_width = "64")]

use std::mem;
use syn::{Expr, Item, Lit, Pat, Type};

#[rustversion::attr(before(2022-11-24), ignore)]
#[test]
fn test_expr_size() {
    assert_eq!(mem::size_of::<Expr>(), 176);
}

#[rustversion::attr(before(2022-09-09), ignore)]
#[test]
fn test_item_size() {
    assert_eq!(mem::size_of::<Item>(), 360);
}

#[rustversion::attr(before(2022-11-24), ignore)]
#[test]
fn test_type_size() {
    assert_eq!(mem::size_of::<Type>(), 240);
}

#[rustversion::attr(before(2021-10-11), ignore)]
#[test]
fn test_pat_size() {
    assert_eq!(mem::size_of::<Pat>(), 192);
}

#[rustversion::attr(before(2022-09-09), ignore)]
#[test]
fn test_lit_size() {
    assert_eq!(mem::size_of::<Lit>(), 32);
}
