extern crate mime;

use mime::{Mime, MimeIter};
use std::collections::hash_map::DefaultHasher;
use std::collections::{BTreeSet, HashSet};
use std::hash::{Hash, Hasher};

fn hash(mime: &Mime) -> u64 {
    let mut hasher = DefaultHasher::new();
    mime.hash(&mut hasher);
    hasher.finish()
}

#[test]
fn different_parameters_are_not_equal() {
    for &(a, b) in &[
        (
            "text/plain; charset=iso-8859-1",
            "text/plain; charset=utf-8",
        ),
        (
            "multipart/form-data; boundary=abc",
            "multipart/form-data; boundary=xyz",
        ),
        (
            "multipart/form-data; boundary=A",
            "multipart/form-data; boundary=a",
        ),
        ("a/b;p=x", "a/b;p=y"),
        ("a/b;p=x", "a/b;p=x;q=y"),
        ("a/b; charset=utf-8", "a/b; charset=utf-9"),
        ("a/b;p=x;q=y", "a/b;p=x;q=z"),
        ("a/b;p=x;q=y", "a/b;q=y;p=x"),
    ] {
        let left: Mime = a.parse().unwrap();
        let right: Mime = b.parse().unwrap();
        assert_ne!(left, right);
        assert_ne!(right, left);
        assert_ne!(left, b);
        assert_ne!(b, left);
        assert_ne!(right, a);
        assert_ne!(a, right);
    }
    let incoming: Mime = "text/plain; charset=iso-8859-1".parse().unwrap();
    assert_ne!(incoming, mime::TEXT_PLAIN_UTF_8);
    assert_ne!(mime::TEXT_PLAIN_UTF_8, incoming);
}

#[test]
fn equivalent_parameters_have_consistent_equality_hash_and_order() {
    for &(a, b) in &[
        ("a/b;p=x", "a/b; p=x"),
        ("a/b;p=x;q=y", "A/B; P=\"x\"; Q=\"y\""),
        ("text/plain; charset=utf-8", "TEXT/PLAIN; CHARSET=\"UTF-8\""),
        ("a/b; charset=utf-9", "a/b; CHARSET=\"UTF-9\""),
        ("a/b;p=\"x;y\";q=z", "a/b; p=\"x;y\"; q=\"z\""),
    ] {
        let left: Mime = a.parse().unwrap();
        let right: Mime = b.parse().unwrap();
        assert_eq!(left, right);
        assert_eq!(right, left);
        assert_eq!(left, b);
        assert_eq!(b, left);
        assert_eq!(right, a);
        assert_eq!(a, right);
        assert_eq!(hash(&left), hash(&right));
        assert_eq!(left.cmp(&right), ::std::cmp::Ordering::Equal);
        let mut hashed = HashSet::new();
        hashed.insert(left.clone());
        assert!(hashed.contains(&right));
        let mut ordered = BTreeSet::new();
        ordered.insert(left);
        assert!(ordered.contains(&right));
    }
    let quoted: Mime = "text/plain; charset=\"UTF-8\"".parse().unwrap();
    assert_eq!(quoted, mime::TEXT_PLAIN_UTF_8);
    assert_eq!(hash(&quoted), hash(&mime::TEXT_PLAIN_UTF_8));
}

#[test]
fn quoted_charset_before_another_parameter() {
    let mime: Mime = "text/html; charset=\"utf-8\"; q=1".parse().unwrap();
    assert_eq!(mime.get_param("charset").unwrap(), "utf-8");
    assert_eq!(mime.get_param("q").unwrap(), "1");
}

#[test]
fn invalid_strings_compare_without_panicking() {
    let mime: Mime = "a/b;p=x".parse().unwrap();
    for s in &[
        "😀",
        "éé",
        "a/b;p=",
        "a/b;p=\"",
        "a/b;p=\"x\"junk",
        "a/bXp=x",
    ] {
        assert_ne!(mime, *s);
        assert_ne!(*s, mime);
    }
}

#[test]
fn iterator_skips_whole_unicode_characters() {
    assert_eq!(MimeIter::new("é😀").next(), None);
    let mut iter = MimeIter::new("é😀text/plain😀application/json");
    assert_eq!(iter.next().unwrap().unwrap(), mime::TEXT_PLAIN);
    assert_eq!(iter.next().unwrap().unwrap(), mime::APPLICATION_JSON);
    assert_eq!(iter.next(), None);
    let mut iter = MimeIter::new("invalidétext/plain");
    assert_eq!(iter.next().unwrap().unwrap_err(), "invalid");
    assert_eq!(iter.next().unwrap().unwrap(), mime::TEXT_PLAIN);
    assert_eq!(iter.next(), None);
}

#[test]
fn iterator_handles_long_invalid_prefix_without_recursion() {
    let mut input = String::new();
    for _ in 0..100_000 {
        input.push_str("é");
    }
    input.push_str("text/plain");
    let mut iter = MimeIter::new(&input);
    assert_eq!(iter.next().unwrap().unwrap(), mime::TEXT_PLAIN);
    assert_eq!(iter.next(), None);
}
