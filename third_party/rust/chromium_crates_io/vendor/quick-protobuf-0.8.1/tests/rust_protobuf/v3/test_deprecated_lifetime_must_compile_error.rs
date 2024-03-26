// DO NOT link to module tree
// This file is run remotely from test_deprecated_lifetime.rs and should fail to compile

#[path = "./test_deprecated_lifetime_pb.rs"]
pub mod proto;
use crate::proto::{NorShouldThis, ThisShouldNotHaveALifetimeParameter, ThisShouldntEither};

// The structs that we set here as our fields should not have lifetimes
// generated for their definitions; therefore, assigning lifetimes to these
// fields should throw an error saying they don't have lifetimes, which we catch
// using the `trybuild` crate in `test_deprecated_lifetime.rs`.
struct Thing<'a> {
    throw_error_field_1: ThisShouldNotHaveALifetimeParameter<'a>,
    throw_error_field_2: ThisShouldntEither<'a>,
    throw_error_field_3: NorShouldThis<'a>,
}

fn main() {}
