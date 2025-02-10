use std::collections::BTreeSet;
use std::ffi::OsStr;
use xattr::FileExt;

use tempfile::{tempfile_in, NamedTempFile};

#[test]
#[cfg(any(target_os = "linux", target_os = "freebsd", target_os = "macos"))]
fn test_fd() {
    use std::os::unix::ffi::OsStrExt;
    // Only works on "real" filesystems.
    let tmp = tempfile_in("/var/tmp").unwrap();
    assert!(tmp.get_xattr("user.test").unwrap().is_none());
    assert_eq!(
        tmp.list_xattr()
            .unwrap()
            .filter(|x| x.as_bytes().starts_with(b"user."))
            .count(),
        0
    );

    tmp.set_xattr("user.test", b"my test").unwrap();
    assert_eq!(tmp.get_xattr("user.test").unwrap().unwrap(), b"my test");
    assert_eq!(
        tmp.list_xattr()
            .unwrap()
            .filter(|x| x.as_bytes().starts_with(b"user."))
            .collect::<Vec<_>>(),
        vec![OsStr::new("user.test")]
    );

    tmp.remove_xattr("user.test").unwrap();
    assert!(tmp.get_xattr("user.test").unwrap().is_none());
    assert_eq!(
        tmp.list_xattr()
            .unwrap()
            .filter(|x| x.as_bytes().starts_with(b"user."))
            .count(),
        0
    );
}

#[test]
#[cfg(any(target_os = "linux", target_os = "freebsd", target_os = "macos"))]
fn test_path() {
    use std::os::unix::ffi::OsStrExt;
    // Only works on "real" filesystems.
    let tmp = NamedTempFile::new_in("/var/tmp").unwrap();
    assert!(xattr::get(tmp.path(), "user.test").unwrap().is_none());
    assert_eq!(
        xattr::list(tmp.path())
            .unwrap()
            .filter(|x| x.as_bytes().starts_with(b"user."))
            .count(),
        0
    );

    xattr::set(tmp.path(), "user.test", b"my test").unwrap();
    assert_eq!(
        xattr::get(tmp.path(), "user.test").unwrap().unwrap(),
        b"my test"
    );
    assert_eq!(
        xattr::list(tmp.path())
            .unwrap()
            .filter(|x| x.as_bytes().starts_with(b"user."))
            .collect::<Vec<_>>(),
        vec![OsStr::new("user.test")]
    );

    xattr::remove(tmp.path(), "user.test").unwrap();
    assert!(xattr::get(tmp.path(), "user.test").unwrap().is_none());
    assert_eq!(
        xattr::list(tmp.path())
            .unwrap()
            .filter(|x| x.as_bytes().starts_with(b"user."))
            .count(),
        0
    );
}

#[test]
#[cfg(any(target_os = "linux", target_os = "freebsd", target_os = "macos"))]
fn test_missing() {
    assert!(xattr::get("/var/empty/does-not-exist", "user.test").is_err());
}

#[test]
#[cfg(any(target_os = "linux", target_os = "freebsd", target_os = "macos"))]
fn test_debug() {
    // Only works on "real" filesystems.
    let tmp = tempfile_in("/var/tmp").unwrap();

    tmp.set_xattr("user.myattr", b"value").unwrap();
    let mut attrs = tmp.list_xattr().unwrap();

    // Debug is idempotent
    assert_eq!(format!("{:?}", attrs), format!("{:?}", attrs));

    // It produces the right value.
    assert_eq!(r#"XAttrs(["user.myattr"])"#, format!("{:?}", attrs));

    // It doesn't affect the underlying iterator.
    assert_eq!("user.myattr", attrs.next().unwrap());

    // An empty iterator produces the right value.
    assert_eq!(r#"XAttrs([])"#, format!("{:?}", attrs));
}

#[test]
#[cfg(any(target_os = "linux", target_os = "freebsd", target_os = "macos"))]
fn test_multi() {
    use std::os::unix::ffi::OsStrExt;
    // Only works on "real" filesystems.
    let tmp = tempfile_in("/var/tmp").unwrap();
    let mut items: BTreeSet<_> = [
        OsStr::new("user.test1"),
        OsStr::new("user.test2"),
        OsStr::new("user.test3"),
    ]
    .iter()
    .cloned()
    .collect();

    for it in &items {
        tmp.set_xattr(it, b"value").unwrap();
    }
    for it in tmp
        .list_xattr()
        .unwrap()
        .filter(|x| x.as_bytes().starts_with(b"user."))
    {
        assert!(items.remove(&*it));
    }
    assert!(items.is_empty());
}

// Tests the deref API variants - regression test for
// https://github.com/Stebalien/xattr/issues/57
#[test]
#[cfg(any(target_os = "linux", target_os = "freebsd", target_os = "macos"))]
fn test_path_deref() {
    use std::os::unix::ffi::OsStrExt;
    // Only works on "real" filesystems.
    let tmp = NamedTempFile::new_in("/var/tmp").unwrap();
    assert!(xattr::get_deref(tmp.path(), "user.test").unwrap().is_none());
    assert_eq!(
        xattr::list_deref(tmp.path())
            .unwrap()
            .filter(|x| x.as_bytes().starts_with(b"user."))
            .count(),
        0
    );

    xattr::set_deref(tmp.path(), "user.test", b"my test").unwrap();
    assert_eq!(
        xattr::get_deref(tmp.path(), "user.test").unwrap().unwrap(),
        b"my test"
    );
    assert_eq!(
        xattr::list_deref(tmp.path())
            .unwrap()
            .filter(|x| x.as_bytes().starts_with(b"user."))
            .collect::<Vec<_>>(),
        vec![OsStr::new("user.test")]
    );

    xattr::remove_deref(tmp.path(), "user.test").unwrap();
    assert!(xattr::get_deref(tmp.path(), "user.test").unwrap().is_none());
    assert_eq!(
        xattr::list_deref(tmp.path())
            .unwrap()
            .filter(|x| x.as_bytes().starts_with(b"user."))
            .count(),
        0
    );
}
