use std::io::BufRead;

use crate::util::test;

use super::*;
use std::error::Error;

type TestResult = std::result::Result<(), Box<dyn Error>>;

fn handle_book<R: BufRead>(book: Element<R>) -> TestResult {
    // Iterate over the children of the book
    let mut count = 0;
    for child in book.children() {
        let child = child?;
        match child.name.as_str() {
            "author" => {
                count += 1;
                assert_eq!(child.child_as_text().unwrap(), "Gambardella, Matthew");
            }
            "title" => {
                count += 1;
                assert_eq!(child.child_as_text().unwrap(), "XML Developer's Guide");
            }
            "nest1" => {
                handle_nest1(child)?;
            }
            "empty1" | "empty2" => {
                assert!(child.child_as_text().is_none());
            }
            _ => panic!("Unexpected child node: {}", child.name),
        }
    }

    // Should have found two elements
    assert_eq!(count, 2);

    Ok(())
}

fn handle_catalog<R: BufRead>(catalog: Element<R>) -> TestResult {
    // Iterate over the children of the catalog
    let mut count = 0;
    for child in catalog.children() {
        let child = child?;
        // First child should be book
        assert_eq!(child.name, "book");

        // Should have an id attribute
        assert!(child.attributes.iter().any(|attr| &attr.name == "id" && &attr.value == "bk101"));

        // Should only have a single child at this level
        count += 1;

        // Handle the book
        handle_book(child)?;
    }
    assert_eq!(count, 1);

    Ok(())
}

fn handle_nest1<R: BufRead>(nest1: Element<R>) -> TestResult {
    // Should have a single child called "nest2"
    let mut count = 0;
    for child in nest1.children() {
        let child = child?;
        // First child should be nest2
        assert_eq!(child.name, "nest2");

        // It should have the expected text
        assert_eq!(child.child_as_text().unwrap(), "Nested");

        // Should only have a single child at this level
        count += 1;
    }
    assert_eq!(count, 1);

    Ok(())
}

#[test]
fn test_iterate_stream() -> TestResult {
    let test_data = test::fixture_as_string("xml/xml_sample_1.xml");

    // Root element should be "catalog"
    let source = ElementSource::new(test_data.as_bytes(), None)?;
    let catalog = source.root()?.unwrap();
    assert_eq!(catalog.name, "catalog");
    handle_catalog(catalog)?;

    Ok(())
}

// TODO expand test coverage (zero children, empty elements etc)
#[test]
fn test_children_as_string() -> TestResult {
    let test_data = test::fixture_as_string("xml/xml_sample_2.xml");

    // Root element should be "catalog"
    let source = ElementSource::new(test_data.as_bytes(), None)?;
    let catalog = source.root()?.unwrap();
    assert_eq!(catalog.name, "catalog");

    // Next element should be "book"
    let mut children = catalog.children();
    let book = children.next().unwrap()?;
    assert_eq!(book.name, "book");
    let expected = "\n        <author>Gambardella, Matthew</author>\n        <title>XML Developer's Guide</title>\n    ";
    assert_eq!(book.children_as_string()?.unwrap(), expected);

    // Next element should be "content:encoded"
    let encoded = children.next().unwrap()?;
    assert_eq!(NS::Content, encoded.namespace);
    assert_eq!(encoded.name, "encoded");
    let text = encoded.children_as_string()?.unwrap();
    assert_eq!(text, "<p>10 km, 21.9072&deg; East, 37.102&deg; North. </p>");

    Ok(())
}

// Verifies the XML decoder handles the various encodings detailed in the RSS2 best practices guide (https://www.rssboard.org/rss-profile#data-types-characterdata)
#[test]
fn test_rss_decoding() -> TestResult {
    let tests = vec![
        ("<title>AT&#x26;T</title>", "AT&T"),
        ("<title>Bill &#x26; Ted's Excellent Adventure</title>", "Bill & Ted's Excellent Adventure"),
        ("<title>The &#x26;amp; entity</title>", "The &amp; entity"),
        ("<title>I &#x3C;3 Phil Ringnalda</title>", "I <3 Phil Ringnalda"),
        ("<title>A &#x3C; B</title>", "A < B"),
        ("<title>A&#x3C;B</title>", "A<B"),
        ("<title>Nice &#x3C;gorilla&#x3E;, what's he weigh?</title>", "Nice <gorilla>, what's he weigh?"),
    ];
    for (xml, expected) in tests {
        let source = ElementSource::new(xml.as_bytes(), None)?;
        let title = source.root()?.unwrap();
        let parsed = title.children_as_string()?.unwrap();
        assert_eq!(expected, parsed);
    }

    Ok(())
}

fn assert_title_bases<R: BufRead>(feed: Element<R>, expected: Vec<&str>) -> TestResult {
    // Find the actual title bases
    let mut title_bases = Vec::new();
    for entry in feed.children() {
        let entry = entry?;
        for title in entry.children() {
            let title = title?;
            title_bases.push(title.xml_base.unwrap());
        }
    }

    // Verify the are as we expect
    let expected = expected.iter().map(|uri| Url::parse(uri).unwrap()).collect::<Vec<Url>>();
    assert_eq!(expected, title_bases);

    Ok(())
}

// Verifies the XML parser handles the xml:base schema
#[test]
fn test_xml_base() -> TestResult {
    let xml = r#"
        <feed version="0.3" xml:base="http://1.example.com/">
        <entry>
        <title type="application/xhtml+xml" xml:base="test/"><div xmlns="http://www.w3.org/1999/xhtml">Example <a href="test.html">test</a></div></title>
        </entry>
        <entry xml:base="http://2.example.com/">
        <title type="application/xhtml+xml" xml:base="test1/test2"><div xmlns="http://www.w3.org/1999/xhtml">Example <a href="test.html">test</a></div></title>
        </entry>
        <entry xml:base="http://3.example.com/">
        <title type="application/xhtml+xml" xml:base="/test3"><div xmlns="http://www.w3.org/1999/xhtml">Example <a href="test.html">test</a></div></title>
        </entry>
        </feed>
    "#;

    let source = ElementSource::new(xml.as_bytes(), None)?;
    let feed = source.root()?.unwrap();
    assert_eq!(&Url::parse("http://1.example.com/")?, feed.xml_base.as_ref().unwrap());

    assert_title_bases(
        feed,
        vec!["http://1.example.com/test/", "http://2.example.com/test1/test2", "http://3.example.com/test3"],
    )?;

    Ok(())
}

// Verifies the XML parser handles the xml:base schema
#[test]
fn test_xml_base_header() -> TestResult {
    let xml = r#"
        <feed version="0.3" xmlns="http://purl.org/atom/ns#" xml:base="feed/base/">
        <entry>
          <title type="application/xhtml+xml" xml:base="../"><div xmlns="http://www.w3.org/1999/xhtml">Example <a href="test.html">test</a></div></title>
        </entry>
        <entry xml:base="/feed2/entry/base/">
          <title type="application/xhtml+xml" xml:base="../"><div xmlns="http://www.w3.org/1999/xhtml">Example <a href="test.html">test</a></div></title>
        </entry>
        </feed>
    "#;

    let source = ElementSource::new(xml.as_bytes(), Some("http://example.com"))?;
    let feed = source.root()?.unwrap();
    assert_eq!(&Url::parse("http://example.com/feed/base/")?, feed.xml_base.as_ref().unwrap());

    assert_title_bases(feed, vec!["http://example.com/feed/", "http://example.com/feed2/entry/"])?;

    Ok(())
}

// Verifies the XML parser handles the xml:base schema
#[test]
fn test_xml_unescape_attrib() -> TestResult {
    let xml = r#"
        <feed xmlns="http://www.w3.org/2005/Atom">
            <link rel="self"
                  href="https://www.reddit.com/search.rss?q=site%3Akevincox.ca&amp;restrict_sr=&amp;sort=new&amp;t=all"
                  type="application/atom+xml"/>
        </feed>
    "#;

    let source = ElementSource::new(xml.as_bytes(), None)?;
    let root = source.root()?.unwrap();
    let link = root.children().next().unwrap()?;
    let href = link.attributes.iter().find(|a| a.name == "href").unwrap();
    assert_eq!(href.value, "https://www.reddit.com/search.rss?q=site%3Akevincox.ca&restrict_sr=&sort=new&t=all");

    Ok(())
}

// Verifies decoding of ISO 8859 content works correctly
#[test]
fn test_iso8859_decode() -> TestResult {
    let xml = test::fixture_as_raw("xml/xml_iso8859.xml");
    let source = ElementSource::new(xml.as_slice(), None)?;
    let root = source.root()?.unwrap();
    let item = root.children().next().unwrap()?;

    let mut elements = item.children();

    let title = elements.next().unwrap()?.child_as_text().unwrap();
    assert_eq!(title, "Digitalministerium: Neue Glasfaserförderung mit Schnellkasse");

    let expected = "Ab April soll es wieder Förderung für den Ausbau von Glasfaser geben.";

    let description = elements.next().unwrap()?.child_as_text().unwrap();
    assert_eq!(description, expected);

    let cdata = elements.next().unwrap()?.child_as_text().unwrap();
    assert_eq!(cdata, expected);

    // The nested XML (or HTML in feeds) breaks the decoding
    let nested = elements.next().unwrap()?.child_as_text().unwrap();
    assert_eq!(nested, format!("<p>{}</p>", expected));

    Ok(())
}
