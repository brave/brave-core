// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is dual-licensed under either the MIT license found in the
// LICENSE-MIT file in the root directory of this source tree or the Apache
// License, Version 2.0 found in the LICENSE-APACHE file in the root directory
// of this source tree. You may select, at your option, one of the above-listed
// licenses.

//! Utility functions.

#[cfg(test)]
pub(crate) fn test_zeroize_on_drop<T: Sized>(value: &mut T) {
    drop_manually(value);

    test_zeroized(value);
}

#[cfg(test)]
pub(crate) fn test_zeroized<T: Sized>(value: &mut T) {
    use std::{mem, slice, vec};

    let test =
        unsafe { slice::from_raw_parts(value as *const _ as *const u8, mem::size_of::<T>()) };

    assert_eq!(test, vec![0; mem::size_of::<T>()]);
}

#[cfg(test)]
pub(crate) fn drop_manually<T: Sized>(value: &mut T) {
    use std::{mem, ptr, vec};

    assert!(mem::needs_drop::<T>());
    let mut test_holder = vec![value];
    let ptr = &mut *test_holder[0] as *mut T;

    unsafe {
        test_holder.set_len(0);
        ptr::drop_in_place(ptr);
    }

    assert_eq!(test_holder.capacity(), 1);
}
