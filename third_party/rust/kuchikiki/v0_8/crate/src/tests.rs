use html5ever::tree_builder::QuirksMode;
use html5ever::QualName;
use std::path::Path;

use tempfile::TempDir;

use crate::parser::{parse_fragment, parse_html};
use crate::select::*;
use crate::traits::*;

#[test]
fn text_nodes() {
    let html = r"
<!doctype html>
<title>Test case</title>
<p>Content contains <b>Important</b> data</p>";
    let document = parse_html().one(html).document_node;
    let paragraph = document.select("p").unwrap().collect::<Vec<_>>();
    assert_eq!(paragraph.len(), 1);
    assert_eq!(
        paragraph[0].text_contents(),
        "Content contains Important data"
    );
    let texts = paragraph[0]
        .as_node()
        .descendants()
        .text_nodes()
        .collect::<Vec<_>>();
    assert_eq!(texts.len(), 3);
    assert_eq!(&*texts[0].borrow(), "Content contains ");
    assert_eq!(&*texts[1].borrow(), "Important");
    assert_eq!(&*texts[2].borrow(), " data");
    {
        let mut x = texts[0].borrow_mut();
        x.truncate(0);
        x.push_str("Content doesn't contain ");
    }
    assert_eq!(&*texts[0].borrow(), "Content doesn't contain ");
}

#[test]
fn parse_and_serialize() {
    let html = r"
<!doctype html>
<title>Test case</title>
<p>Content";
    let document = parse_html().one(html).document_node;
    assert_eq!(
        document.as_document().unwrap().quirks_mode(),
        QuirksMode::NoQuirks
    );
    assert_eq!(
        document.to_string(),
        r"<!DOCTYPE html><html><head><title>Test case</title>
</head><body><p>Content</p></body></html>"
    );
}

#[test]
fn parse_and_serialize_fragment() {
    let html = r"<tbody><tr><td>Test case";

    let ctx_name = QualName::new(None, ns!(html), local_name!("tbody"));
    let document = parse_fragment(ctx_name, vec![]).one(html).document_node;
    assert_eq!(
        document.as_document().unwrap().quirks_mode(),
        QuirksMode::NoQuirks
    );
    assert_eq!(
        document.to_string(),
        r"<html><tr><td>Test case</td></tr></html>"
    );
}

#[test]
fn parse_file() {
    let mut path = Path::new(env!("CARGO_MANIFEST_DIR")).to_path_buf();
    path.push("test_data");
    path.push("foo.html");

    let html = r"<!DOCTYPE html><html><head>
        <title>Test case</title>
    </head>
    <body>
        <p>Foo</p>
    

</body></html>";
    let document = parse_html()
        .from_utf8()
        .from_file(&path)
        .unwrap()
        .document_node;
    assert_eq!(document.to_string(), html);
}

#[test]
fn serialize_and_read_file() {
    let tempdir = TempDir::new().unwrap();
    let mut path = tempdir.path().to_path_buf();
    path.push("temp.html");

    let html = r"<!DOCTYPE html><html><head><title>Title</title></head><body>Body</body></html>";
    let document = parse_html().one(html).document_node;
    let _ = document.serialize_to_file(path.clone());

    let document2 = parse_html()
        .from_utf8()
        .from_file(&path)
        .unwrap()
        .document_node;
    assert_eq!(document.to_string(), document2.to_string());
}

#[test]
fn select() {
    let html = r"
<title>Test case</title>
<p class=foo>Foo
<p>Bar
<p class=foo>Foo
";

    let document = parse_html().one(html).document_node;
    let matching = document.select("p.foo").unwrap().collect::<Vec<_>>();
    assert_eq!(matching.len(), 2);
    let child = matching[0].as_node().first_child().unwrap();
    assert_eq!(&**child.as_text().unwrap().borrow(), "Foo\n");
    assert_eq!(matching[0].attributes.borrow().get("class"), Some("foo"));
    assert_eq!(
        matching[0].attributes.borrow().get(local_name!("class")),
        Some("foo")
    );

    let selectors = Selectors::compile("p.foo").unwrap();
    let matching2 = selectors
        .filter(document.descendants().elements())
        .collect::<Vec<_>>();
    assert_eq!(matching, matching2);
}

#[test]
fn select_first() {
    let html = r"
<title>Test case</title>
<p class=foo>Foo
<p>Bar
<p class=foo>Baz
";

    let document = parse_html().one(html).document_node;
    let matching = document.select_first("p.foo").unwrap();
    let child = matching.as_node().first_child().unwrap();
    assert_eq!(&**child.as_text().unwrap().borrow(), "Foo\n");
    assert_eq!(matching.attributes.borrow().get("class"), Some("foo"));
    assert_eq!(
        matching.attributes.borrow().get(local_name!("class")),
        Some("foo")
    );

    assert!(document.select_first("p.bar").is_err());
}

#[test]
fn to_string() {
    let html = r"<!DOCTYPE html>
<html>
    <head>
        <title>Test case</title>
    </head>
    <body>
        <p class=foo>Foo
    </body>
</html>";

    let document = parse_html().one(html).document_node;
    assert_eq!(
        document
            .inclusive_descendants()
            .nth(11)
            .unwrap()
            .to_string(),
        "<p class=\"foo\">Foo\n    \n</p>"
    );
}

#[test]
fn specificity() {
    let selectors = Selectors::compile(".example, :first-child, div").unwrap();
    let specificities = selectors
        .0
        .iter()
        .map(|s| s.specificity())
        .collect::<Vec<_>>();
    assert_eq!(specificities.len(), 3);
    assert!(specificities[0] == specificities[1]);
    assert!(specificities[0] > specificities[2]);
    assert!(specificities[1] > specificities[2]);
}
