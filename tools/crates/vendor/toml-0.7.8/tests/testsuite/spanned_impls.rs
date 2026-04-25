use std::cmp::{Ord, Ordering, PartialOrd};

use serde::Deserialize;
use toml::{from_str, Spanned};

#[test]
fn test_spans_impls() {
    #[derive(Deserialize)]
    struct Foo {
        bar: Spanned<bool>,
        baz: Spanned<String>,
    }
    let f: Foo = from_str(
        "
    bar = true
    baz = \"yes\"
    ",
    )
    .unwrap();
    let g: Foo = from_str(
        "
    baz = \"yes\"
    bar = true
    ",
    )
    .unwrap();
    assert!(f.bar.span() != g.bar.span());
    assert!(f.baz.span() != g.baz.span());

    // test that eq still holds
    assert_eq!(f.bar, g.bar);
    assert_eq!(f.baz, g.baz);

    // test that Ord returns equal order
    assert_eq!(f.bar.cmp(&g.bar), Ordering::Equal);
    assert_eq!(f.baz.cmp(&g.baz), Ordering::Equal);

    // test that PartialOrd returns equal order
    assert_eq!(f.bar.partial_cmp(&g.bar), Some(Ordering::Equal));
    assert_eq!(f.baz.partial_cmp(&g.baz), Some(Ordering::Equal));
}
