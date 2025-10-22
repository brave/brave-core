use crate::model::{Entry, Feed, FeedType, Image, Link, Person, Text};
use crate::parser;
use crate::util::test;

// Example from the web
#[test]
fn test_example_1() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss1/rss_1.0_example_1.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let entry0 = actual.entries.get(0).unwrap();
    let entry1 = actual.entries.get(1).unwrap();
    let expected = Feed::new(FeedType::RSS1)
        .id(actual.id.as_ref()) // not present in the test data
        .title(Text::new("Feed title".into()))
        .link(Link::new("http://www.example.com/main.html", None))
        .description(Text::new("Site description".into()))
        .updated(actual.updated) // not present in the test data
        .published("2017-06-13T09:00:00Z")
        .language("ja")
        .entry(
            Entry::default()
                .id("7d61c42a2d8ecf2289e789e1fb2035d1") // hash of the link
                .updated(entry0.updated) // not present in the test data
                .title(Text::new("記事1のタイトル".into()))
                .link(Link::new("記事1のURL", None))
                .summary(Text::new("記事1の内容".into()))
                .published("2017-06-13T09:00:00Z")
                .author(Person::new("記事1の作者名")),
        )
        .entry(
            Entry::default()
                .id("e342c1b080da9ffbfd10c0a6ba49395f") // hash of the link
                .updated(entry1.updated) // not present in the test data
                .title(Text::new("記事2のタイトル".into()))
                .link(Link::new("記事2のURL", None))
                .summary(Text::new("記事2の内容".into()))
                .author(Person::new("記事2の作者名")),
        );

    // Check
    assert_eq!(actual, expected);
}

// Example with content:encoded from https://planet.freedesktop.org/rss10.xml
#[test]
fn test_example_2() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss1/rss_1.0_example_2.xml");
    let feed = parser::parse(test_data.as_bytes()).unwrap();

    // content:encoded should be mapped to the content field
    assert!(feed.entries[0]
        .content
        .as_ref()
        .unwrap()
        .body
        .as_ref()
        .unwrap()
        .starts_with("This morning I saw two things that were Microsoft "));
    // content media type should be text/html.
    assert_eq!(feed.entries[0].content.as_ref().unwrap().content_type, "text/html");
}

// Example 1 from the spec at https://validator.w3.org/feed/docs/rss1.html
#[test]
fn test_spec_1() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss1/rss_1.0_spec_1.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let entry0 = actual.entries.get(0).unwrap();
    let entry1 = actual.entries.get(1).unwrap();
    let expected = Feed::new(FeedType::RSS1)
        .id(actual.id.as_ref())     // not present in the test data
        .title(Text::new("XML.com".into()))
        .link(Link::new("http://xml.com/pub", None))
        .description(Text::new("XML.com features a rich mix of information and services\n            for the XML community.".into()))
        .logo(Image::new("http://xml.com/universal/images/xml_tiny.gif".into())
            .link("http://www.xml.com")
            .title("XML.com"))
        .updated(actual.updated)    // not present in the test data
        .entry(Entry::default()
            .id("958983927af7075ad55f6d2c9b0c24b2")     // hash of the link
            .updated(entry0.updated)            // not present in the test data
            .title(Text::new("Processing Inclusions with XSLT".into()))
            .link(Link::new("http://xml.com/pub/2000/08/09/xslt/xslt.html", None))
            .summary(Text::new("Processing document inclusions with general XML tools can be\n            problematic. This article proposes a way of preserving inclusion\n            information through SAX-based processing.".into())))
        .entry(Entry::default()
            .id("54f62e9fe8901546d7e25c0827e1b8e8")     // hash of the link
            .updated(entry1.updated)            // not present in the test data
            .title(Text::new("Putting RDF to Work".into()))
            .link(Link::new("http://xml.com/pub/2000/08/09/rdfdb/index.html", None))
            .summary(Text::new("Tool and API support for the Resource Description Framework\n            is slowly coming of age. Edd Dumbill takes a look at RDFDB,\n            one of the most exciting new RDF toolkits.".into())));

    // Check
    assert_eq!(actual, expected);
}

// Example 2 from the spec at https://validator.w3.org/feed/docs/rss1.html
#[test]
fn test_spec_2() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss1/rss_1.0_spec_2.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let entry0 = actual.entries.get(0).unwrap();
    let expected = Feed::new(FeedType::RSS1)
        .id(actual.id.as_ref()) // not present in the test data
        .title(Text::new("Meerkat".into()))
        .link(Link::new("http://meerkat.oreillynet.com", None))
        .description(Text::new("Meerkat: An Open Wire Service".into()))
        .logo(
            Image::new("http://meerkat.oreillynet.com/icons/meerkat-powered.jpg".into())
                .link("http://meerkat.oreillynet.com")
                .title("Meerkat Powered!"),
        )
        .updated(actual.updated) // not present in the test data
        .author(Person::new("Rael Dornfest (mailto:rael@oreilly.com)"))
        .rights(Text::new("Copyright © 2000 O'Reilly & Associates, Inc.".into()))
        .entry(
            Entry::default()
                .id("acf7c86547d5d594af6d8f3327e84b06") // hash of the link
                .updated(entry0.updated) // not present in the test data
                .title(Text::new("XML: A Disruptive Technology".into()))
                .link(Link::new("http://c.moreover.com/click/here.pl?r123", None))
                .summary(Text::new(
                    "XML is placing increasingly heavy loads on the existing technical\n            infrastructure of the Internet.".into(),
                ))
                .author(Person::new("Simon St.Laurent (mailto:simonstl@simonstl.com)"))
                .rights(Text::new("Copyright © 2000 O'Reilly & Associates, Inc.".into())),
        );

    // Check
    assert_eq!(actual, expected);
}

// Verifies that publish date is set
#[test]
fn test_debian() {
    let test_data = test::fixture_as_string("rss1/rss_1.0_debian.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();
    let entry = actual.entries.get(0).expect("feed has 1 entry");

    assert!(entry.published.is_some());
}

// Verifies ISO8859 decoding works correctly
#[test]
fn test_iso8859() {
    let test_data = test::fixture_as_raw("rss1/rss_1.0_iso8859.xml");
    let actual = parser::parse(test_data.as_slice()).unwrap();
    let entry = actual.entries.get(0).expect("feed has 1 entry");

    let expected = "Ab April soll es wieder Förderung für den Ausbau von Glasfaser geben.";
    let summary = &entry.summary.as_ref().unwrap().content;
    assert!(summary.starts_with(expected));
    let content = entry.content.as_ref().unwrap().body.as_ref().unwrap();
    assert!(content.contains(expected));
}

// Verifies bioRxiv titles are extracted correctly
#[test]
fn test_bio_rxiv() {
    let test_data = test::fixture_as_raw("rss1/rss_1.0_biorxiv.xml");
    let actual = parser::parse(test_data.as_slice()).unwrap();
    for entry in actual.entries {
        assert!(!entry.title.unwrap().content.is_empty())
    }
}
