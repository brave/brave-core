use crate::model::{Content, Entry, Feed, FeedType, Image, Link, Person, Text};
use crate::parser;
use crate::util::test;

// Verify we can parse a more complete feed
#[test]
fn test_example_1() {
    // Parse the feed
    let test_data = test::fixture_as_string("jsonfeed/jsonfeed_example_1.json");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::JSON)
        .id(&actual.id)                 // not in test content
        .updated(actual.updated)        // not in test content
        .title(Text::new("Daring Fireball".into()))
        .link(Link::new("https://daringfireball.net/", None))
        .link(Link::new("https://daringfireball.net/feeds/json", None))
        .author(Person::new("John Gruber")
            .uri("https://twitter.com/gruber"))
        .logo(Image::new("https://daringfireball.net/graphics/apple-touch-icon.png".into()))
        .icon(Image::new("https://daringfireball.net/graphics/favicon-64.png".into()))
        .entry(Entry::default()
            .title(Text::new("How Jeff Bezos’s iPhone X Was Hacked".into()))
            .published("2020-01-24T23:46:57Z")
            .updated_parsed("2020-01-24T23:46:57Z")
            .id("https://daringfireball.net/linked/2020/01/24/bezos-iphone-x")
            .link(Link::new("https://daringfireball.net/linked/2020/01/24/bezos-iphone-x", None))
            .link(Link::new("https://www.nytimes.com/2020/01/22/technology/jeff-bezos-hack-iphone.html", None))
            .author(Person::new("John Gruber"))
            .content(Content::default()
                .body(r#"<p>Good summary from The New York Times. Until this week’s news, I don’t believe we knew what type of phone Bezos was using when he was hacked. Now we know: an iPhone X.</p>"#)
                .content_type("text/html")
                .length(177)))
        .entry(Entry::default()
            .title(Text::new("Instagram for Windows 95".into()))
            .published("2020-01-21T01:07:00Z")
            .updated_parsed("2020-01-21T20:58:36Z")
            .id("https://daringfireball.net/linked/2020/01/20/instagram-for-win95")
            .link(Link::new("https://daringfireball.net/linked/2020/01/20/instagram-for-win95", None))
            .link(Link::new("https://www.behance.net/gallery/41023081/Instagram-for-Win95?utm_source=morning_brew", None))
            .author(Person::new("John Gruber"))
            .content(Content::default()
                .body(r#"<p>Delightful work by Petrick Studio.</p>"#)
                .content_type("text/html")
                .length(41)));

    // Check
    assert_eq!(actual, expected);
}

// Verify we can parse the specification example
#[test]
fn test_spec_1() {
    // Parse the feed
    let test_data = test::fixture_as_string("jsonfeed/jsonfeed_spec_1.json");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::JSON)
        .id(&actual.id)                 // not in test content
        .updated(actual.updated)        // not in test content
        .title(Text::new("JSON Feed".into()))
        .description(Text::new("JSON Feed is a pragmatic syndication format for blogs, microblogs, and other time-based content.".into()))
        .link(Link::new("https://jsonfeed.org/", None))
        .link(Link::new("https://jsonfeed.org/feed.json", None))
        .author(Person::new("Brent Simmons and Manton Reece")
            .uri("https://jsonfeed.org/"))
        .entry(Entry::default()
            .updated(actual.entries[0].updated)             // not in test content
            .id("https://jsonfeed.org/2017/05/17/announcing_json_feed")
            .title(Text::new("Announcing JSON Feed".into()))
            .published("2017-05-17T08:02:12-07:00")
            .link(Link::new("https://jsonfeed.org/2017/05/17/announcing_json_feed", None))
            .author(Person::new("Brent Simmons and Manton Reece")
                .uri("https://jsonfeed.org/"))
            .content(Content::default()
                .body(r#"<p>We — Manton Reece and Brent Simmons — have noticed that JSON has become the developers’ choice for APIs, and that developers will often go out of their way to avoid XML. JSON is simpler to read and write, and it’s less prone to bugs.</p>
<p>So we developed JSON Feed, a format similar to <a href="http://cyber.harvard.edu/rss/rss.html">RSS</a> and <a href="https://tools.ietf.org/html/rfc4287">Atom</a> but in JSON. It reflects the lessons learned from our years of work reading and publishing feeds.</p>..."#)
                .content_type("text/html")
                .length(518)));

    // Check
    assert_eq!(actual, expected);
}

// Even though the spec is clear its mandatory, we still have feeds that do not supply the ID
#[test]
fn test_optional_entry_id() {
    // Parse the feed
    let test_data = test::fixture_as_string("jsonfeed/jsonfeed_elastic_1.1.json");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    assert!(!actual.id.is_empty());
    for entry in actual.entries {
        assert!(!entry.id.is_empty());
    }
}

// Verify we can parse the v1.1 specific payload
#[test]
fn test_elastic_v1_1() {
    // Parse the feed
    let test_data = test::fixture_as_string("jsonfeed/jsonfeed_elastic_1.1.json");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Check language was extracted correctly for the feed
    assert_eq!("en-US", actual.language.unwrap());

    // Check feed authors (should combine both deprecated and new fields)
    assert_eq!(actual.authors, vec!(Person::new("Fake Author 3"), Person::new("Fake Author 4")));

    // Check first item - combine author + authors
    let mut entries = actual.entries.iter();
    let entry = entries.next().unwrap();
    let chris = Person {
        name: "Chris Churilo".to_string(),
        uri: Some("https://www.influxdata.com/blog/author/chrisc/".to_string()),
        email: None,
    };
    assert_eq!(entry.authors, vec!(Person::new("Fake Author 2"), chris.clone(), Person::new("Fake Author 1")));

    // Second item migrates from old author to new authors field
    let entry = entries.next().unwrap();
    assert_eq!(entry.authors, vec!(chris.clone()));

    // Third item inherits feed authors (per the spec)
    let entry = entries.next().unwrap();
    assert_eq!(entry.authors, vec!(Person::new("Fake Author 3"), Person::new("Fake Author 4")));
}
