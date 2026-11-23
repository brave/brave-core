// Copyright (c) The camino Contributors
// SPDX-License-Identifier: MIT OR Apache-2.0

// Test that all required impls exist.

use crate::{Utf8Path, Utf8PathBuf};
use std::{
    borrow::Cow,
    path::{Path, PathBuf},
    rc::Rc,
    sync::Arc,
};

macro_rules! all_into {
    ($t:ty, $x:ident) => {
        test_into::<$t, Utf8PathBuf>($x.clone());
        test_into::<$t, Box<Utf8Path>>($x.clone());
        test_into::<$t, Arc<Utf8Path>>($x.clone());
        test_into::<$t, Rc<Utf8Path>>($x.clone());
        test_into::<$t, Cow<'_, Utf8Path>>($x.clone());
        test_into::<$t, PathBuf>($x.clone());
        test_into::<$t, Box<Path>>($x.clone());
        test_into::<$t, Arc<Path>>($x.clone());
        test_into::<$t, Rc<Path>>($x.clone());
        test_into::<$t, Cow<'_, Path>>($x.clone());
    };
}

#[test]
fn test_borrowed_into() {
    let utf8_path = Utf8Path::new("test/path");
    all_into!(&Utf8Path, utf8_path);
}

#[test]
fn test_owned_into() {
    let utf8_path_buf = Utf8PathBuf::from("test/path");
    all_into!(Utf8PathBuf, utf8_path_buf);
}

#[test]
fn test_boxed_into() {
    // Exercise Utf8PathBuf -> Box<Utf8Path> (mostly intended for Miri).
    let utf8_path_buf = Utf8PathBuf::from("test/path");
    // This exercises Clone.
    let boxed: Box<Utf8Path> = utf8_path_buf.clone().into();

    assert_eq!(
        &*boxed.clone().into_std_boxed_path(),
        Path::new("test/path")
    );
    let from_boxed: Box<Path> = boxed.clone().into();
    assert_eq!(&*from_boxed, Path::new("test/path"));

    // Box<Utf8Path> -> Utf8PathBuf.
    assert_eq!(boxed.clone().into_path_buf(), utf8_path_buf);
    let from_boxed: Utf8PathBuf = boxed.into();
    assert_eq!(from_boxed, utf8_path_buf);
}

#[test]
fn test_boxed_from_path() {
    // Exercise Box<Path> -> Box<Utf8Path> (mostly intended for Miri).
    let path: Box<Path> = Path::new("test/path").into();

    let utf8 = Utf8Path::from_boxed_path(path.clone()).expect("valid UTF-8 succeeds");
    assert_eq!(&*utf8, Utf8Path::new("test/path"));

    // This exercises the TryFrom impl.
    let utf8: Box<Utf8Path> = path.try_into().expect("valid UTF-8 succeeds");
    assert_eq!(&*utf8, Utf8Path::new("test/path"));
}

#[cfg(unix)]
#[test]
fn test_boxed_from_path_non_utf8() {
    use std::{ffi::OsStr, os::unix::ffi::OsStrExt};

    let non_unicode = OsStr::from_bytes(b"\xFF\xFF\xFF");
    let path: Box<Path> = Path::new(non_unicode).into();

    // from_boxed_path returns the original Box<Path> on failure.
    let err = Utf8Path::from_boxed_path(path.clone()).expect_err("non-UTF-8 fails");
    assert_eq!(&*err, &*path);

    // TryFrom surfaces a FromBoxedPathError that still owns the original path.
    let err = <Box<Utf8Path>>::try_from(path.clone()).expect_err("non-UTF-8 fails");
    assert_eq!(err.as_path(), &*path);
    assert_eq!(err.into_boxed_path(), path);
}

fn test_into<T, U>(orig: T)
where
    T: Into<U>,
{
    let _ = orig.into();
}

#[cfg(path_buf_deref_mut)]
#[test]
fn test_deref_mut() {
    // This test is mostly for miri.
    let mut path_buf = Utf8PathBuf::from("foobar");
    let _: &mut Utf8Path = &mut path_buf;
}
