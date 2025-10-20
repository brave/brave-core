use chrono::{TimeZone, Utc};
use std::time::Duration;

use crate::model::*;
use crate::parser;
use crate::parser::util;
use crate::util::test;
use url::Url;

// Basic example from various sources (Wikipedia etc).
#[test]
fn test_example_1() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss2/rss_2.0_example_1.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::RSS2)
        .id(actual.id.as_ref()) // not present in the test data
        .title(Text::new("RSS Title".into()))
        .description(Text::new("This is an example of an RSS feed".into()))
        .link(Link::new("http://www.example.com/main.html", None))
        .updated_parsed("Mon, 06 Sep 2010 00:01:00 +0000")
        .published("Sun, 06 Sep 2009 16:20:00 +0000")
        .ttl(1800)
        .entry(
            Entry::default()
                .title(Text::new("Example entry".into()))
                .summary(Text::html("Here is some text containing an interesting description.".into()))
                .link(Link::new("http://www.example.com/blog/post/1", None))
                .id("7bd204c6-1655-4c27-aeee-53f933c5395f")
                .published("Sun, 06 Sep 2009 16:20:00 +0000")
                .updated_parsed("Mon, 06 Sep 2010 00:01:00 +0000"),
        ); // copy from feed

    // Check
    assert_eq!(actual, expected);
}

// More detailed feed from NASA
#[test]
fn test_example_2() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss2/rss_2.0_example_2.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected: Feed = Feed::new(FeedType::RSS2)
        .id(actual.id.as_ref())     // not present in the test data
        .updated(actual.updated)    // not present in the test data
        .title(Text::new("NASA Breaking News".into()))
        .description(Text::new("A RSS news feed containing the latest NASA news articles and press releases.".into()))
        .link(Link::new("http://www.nasa.gov/", None))
        .link(Link::new("http://www.nasa.gov/rss/dyn/breaking_news.rss", None)
            .rel("self"))
        .language("en-us")
        .contributor(Person::new("managingEditor")
            .email("jim.wilson@nasa.gov"))
        .contributor(Person::new("webMaster")
            .email("brian.dunbar@nasa.gov"))
        .entry(Entry::default()
            .title(Text::new("NASA Television to Broadcast Space Station Departure of Cygnus Cargo Ship".into()))
            .link(Link::new("http://www.nasa.gov/press-release/nasa-television-to-broadcast-space-station-departure-of-cygnus-cargo-ship", None))
            .summary(Text::html(r#"More than three months after delivering several tons of supplies and scientific experiments to
                the International Space Station, Northrop Grumman’s Cygnus cargo spacecraft, the SS Roger Chaffee, will
                depart the orbiting laboratory Tuesday, Aug. 6.
            "#.to_owned()))
            .id("http://www.nasa.gov/press-release/nasa-television-to-broadcast-space-station-departure-of-cygnus-cargo-ship")
            .published("Thu, 01 Aug 2019 16:15 EDT")
            .updated(actual.updated)
            .media(MediaObject::default()
                .content(MediaContent::new()
                    .url("http://www.nasa.gov/sites/default/files/styles/1x1_cardfeed/public/thumbnails/image/47616261882_4bb534d293_k.jpg?itok=Djjjs81t")
                    .content_type("image/jpeg")
                    .size(892854))));

    // Check
    assert_eq!(actual, expected);
}

// News feed from the New Yorker
#[test]
fn test_example_3() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss2/rss_2.0_example_3.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::RSS2)
        .id(actual.id.as_ref())     // not present in the test data
        .title(Text::new("News, Politics, Opinion, Commentary, and Analysis".into()))
        .description(Text::new("In-depth reporting, commentary on breaking news, political analysis, and opinion from The New\n            Yorker.".into()))
        .link(Link::new("https://www.newyorker.com/news", None))
        .link(Link::new("https://www.newyorker.com/feed/news/rss", None)
            .rel("self")
            .media_type("application/atom+xml"))
        .rights(Text::new("© Condé Nast 2019".into()))
        .language("en")
        .updated_parsed("Tue, 06 Aug 2019 10:46:05 +0000")
        .entry(Entry::default()
            .title(Text::new("How a Historian Uncovered Ronald Reagan’s Racist Remarks to Richard Nixon".into()))
            .link(Link::new("https://www.newyorker.com/news/q-and-a/how-a-historian-uncovered-ronald-reagans-racist-remarks-to-richard-nixon", None))
            .id("5d420f3abfe6c20008d5eaad")
            .author(Person::new("Isaac Chotiner"))
            .summary(Text::html("Isaac Chotiner talks with the historian Tim Naftali, who published the text and audio of a\n                taped call, from 1971, in which Reagan described the African delegates to the U.N. in luridly racist\n                terms.\n            ".into()))
            .category(Category::new("News / Q. & A."))
            .published("Fri, 02 Aug 2019 15:35:34 +0000")
            .updated(actual.updated)
            .media(MediaObject::default()
                .thumbnail(MediaThumbnail::new(Image::new("https://media.newyorker.com/photos/5d4211a4ba8a9c0009a57cfd/master/pass/Chotiner-ReaganRacismNaftali-3.jpg".into()).width(2560).height(1819)))
            )
        );

    // Check
    assert_eq!(actual, expected);
}

// Structured event data on earthquakes
#[test]
fn test_example_4() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss2/rss_2.0_example_4.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::RSS2)
        .id(actual.id.as_ref())     // not present in the test data
        .title(Text::new("Earthquakes today".into()))
        .link(Link::new("http://www.earthquakenewstoday.com/feed/", None)
            .rel("self")
            .media_type("application/rss+xml"))
        .link(Link::new("http://www.earthquakenewstoday.com", None))
        .description(Text::new("Current and latest world earthquakes breaking news, activity and articles today".into()))
        .updated_parsed("Tue, 06 Aug 2019 05:01:15 +0000")
        .language("en-us")
        .generator(Generator::new("https://wordpress.org/?v=5.1.1"))
        .entry(Entry::default()
            .title(Text::new("Minor earthquake, 3.5 mag was detected near Aris in Greece".into()))
            .author(Person::new("admin"))
            .link(Link::new("http://www.earthquakenewstoday.com/2019/08/06/minor-earthquake-3-5-mag-was-detected-near-aris-in-greece/", None))
            .published("Tue, 06 Aug 2019 05:01:15 +0000")
            .category(Category::new("Earthquake breaking news"))
            .category(Category::new("Minor World Earthquakes Magnitude -3.9"))
            .category(Category::new("Spárti"))
            .id("http://www.earthquakenewstoday.com/2019/08/06/minor-earthquake-3-5-mag-was-detected-near-aris-in-greece/")
            .summary(Text::html("A minor earthquake magnitude 3.5 (ml/mb) strikes near Kalamáta, Trípoli, Pýrgos, Spárti, Filiatrá, Messíni, Greece on Tuesday.".into()))
            .content(Content::default()
                .body("<p><img class='size-full alignleft' title='Earthquake location 37.102S, 21.9072W' alt='Earthquake location 37.102S, 21.9072W' src='http://www.earthquakenewstoday.com/wp-content/uploads/35_20.jpg' width='146' height='146' />A minor earthquake with magnitude 3.5 (ml/mb) was detected on Tuesday, 8 kilometers (5 miles) from Aris in Greece.Exact location of event, depth 10 km, 21.9072&deg; East, 37.102&deg; North. </p>")
                .content_type("text/html"))
            .updated(actual.updated));

    // Check
    assert_eq!(actual, expected);
}

// Tech news from Ars Technica
#[test]
fn test_example_5() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss2/rss_2.0_example_5.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::RSS2)
        .id(actual.id.as_ref()) // not present in the test data
        .title(Text::new("Ars Technica".into()))
        .link(Link::new("https://arstechnica.com", None))
        .link(
            Link::new("http://feeds.arstechnica.com/arstechnica/index", None)
                .rel("self")
                .media_type("application/rss+xml"),
        )
        .link(Link::new("http://pubsubhubbub.appspot.com/", None).rel("hub"))
        .description(Text::new(
            "Serving the Technologist for more than a decade. IT news, reviews, and analysis.".into(),
        ))
        .updated_parsed("Tue, 06 Aug 2019 00:03:56 +0000")
        .language("en-us")
        .generator(Generator::new("https://wordpress.org/?v=4.8.3"))
        .logo(
            Image::new("https://cdn.arstechnica.net/wp-content/uploads/2016/10/cropped-ars-logo-512_480-32x32.png".into())
                .title("Ars Technica")
                .link("https://arstechnica.com")
                .width(32)
                .height(32),
        )
        .entry(
            Entry::default()
                .title(Text::new(
                    "Apple isn’t the most cash-rich company in the world anymore, but it doesn’t matter".into(),
                ))
                .link(Link::new("https://arstechnica.com/?p=1546121", None))
                .published("Mon, 05 Aug 2019 23:11:09 +0000")
                .category(Category::new("Tech"))
                .category(Category::new("alphabet"))
                .category(Category::new("apple"))
                .category(Category::new("google"))
                .id("https://arstechnica.com/?p=1546121")
                .author(Person::new("Samuel Axon"))
                .summary(Text::html("Alphabet has $117 billion in cash on hand.".into()))
                .content(
                    Content::default()
                        .body("Google co-founder Larry Page is now CEO of Alphabet.")
                        .content_type("text/html"),
                )
                .updated(actual.updated),
        );

    // Check
    assert_eq!(actual, expected);
}

// Trailers from Apple (no UUID)
#[test]
fn test_example_6() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss2/rss_2.0_example_6.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::RSS2)
        .id("b2ef47d837e6c0d9d757e14852e5bde")     // hash of the link
        .title(Text::new("Latest Movie Trailers".into()))
        .link(Link::new("https://trailers.apple.com/", None))
        .description(Text::new("Recently added Movie Trailers.".into()))
        .language("en-us")
        .updated_parsed("2020-02-07T15:30:28Z")
        .generator(Generator::new("Custom"))
        .rights(Text::new("2020 Apple Inc.".into()))
        .entry(Entry::default()
            .title(Text::new("Vitalina Varela - Trailer".into()))
            .link(Link::new("https://trailers.apple.com/trailers/independent/vitalina-varela", None))
            .summary(Text::html("A film of deeply concentrated beauty, acclaimed filmmaker Pedro Costa’s VITALINA VARELA stars nonprofessional actor Vitalina Varela in an extraordinary performance based on her own life. Vitalina plays a Cape Verdean woman who has travelled to Lisbon to reunite with her husband, after two decades of separation, only to arrive mere days after his funeral. Alone in a strange forbidding land, she perseveres and begins to establish a new life. Winner of the Golden Leopard for Best Film and Best Actress at the Locarno Film Festival, as well as an official selection of the Sundance Film Festival, VITALINA VARELA is a film of shadow and whisper, a profoundly moving and visually ravishing masterpiece.".into()))
            .content(Content::default()
                .body(r#"<span style="font-size: 16px; font-weight: 900; text-decoration: underline;">Vitalina Varela - Trailer</span>"#)
                .content_type("text/html"))
            .published("2020-02-06T08:00:00Z")
            .id("73226f21f249d758bd97a1fac90897d2")        // hash of the link
            .updated(actual.updated));

    // Check
    assert_eq!(actual, expected);
}

#[test]
fn test_wirecutter() {
    let test_data = test::fixture_as_string("rss2/rss_2.0_wirecutter.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    let entry = actual.entries.get(0).expect("sample feed has one entry");
    let category = entry.categories.get(0).expect("entry has one category");
    assert_eq!(category.term, "Uncategorized");

    let author = entry.authors.get(0).expect("entry has one author");
    assert_eq!(author.name, "James Austin");
}

// Verify we can parse the example contained in the RSS 2.0 specification
#[test]
fn test_spec_1() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss2/rss_2.0_spec_1.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::RSS2)
        .id(actual.id.as_ref()) // not present in the test data
        .title(Text::new("Scripting News".into()))
        .link(Link::new("http://www.scripting.com/", None))
        .description(Text::new("A weblog about scripting and stuff like that.".into()))
        .language("en-us")
        .rights(Text::new("Copyright 1997-2002 Dave Winer".into()))
        .updated_parsed("Mon, 30 Sep 2002 11:00:00 GMT")
        .generator(Generator::new("Radio UserLand v8.0.5"))
        .category(Category::new("1765").scheme("Syndic8"))
        .contributor(Person::new("managingEditor").email("dave@userland.com"))
        .contributor(Person::new("webMaster").email("dave@userland.com"))
        .ttl(40)
        .entry(
            Entry::default()
                .summary(Text::html(
                    r#"Joshua Allen: <a href="http://www.netcrucible.com/blog/2002/09/29.html#a243">Who
                loves namespaces?</a>
            "#
                    .to_owned(),
                ))
                .published("Sun, 29 Sep 2002 19:59:01 GMT")
                .id("http://scriptingnews.userland.com/backissues/2002/09/29#When:12:59:01PM")
                .updated_parsed("Mon, 30 Sep 2002 11:00:00 GMT"),
        ) // copy from feed
        .entry(
            Entry::default()
                .summary(Text::html(
                    r#"<a href="http://www.docuverse.com/blog/donpark/2002/09/29.html#a68">Don Park</a>:
                "It is too easy for engineer to anticipate too much and XML Namespace is a frequent host of
                over-anticipation."
            "#
                    .to_owned(),
                ))
                .published("Mon, 30 Sep 2002 01:52:02 GMT")
                .id("http://scriptingnews.userland.com/backissues/2002/09/29#When:6:52:02PM")
                .updated_parsed("Mon, 30 Sep 2002 11:00:00 GMT"),
        ); // copy from feed

    // Check
    assert_eq!(actual, expected);
}

// Verifies that an invalid XML document (e.g. truncated) fails to parse
#[test]
fn test_invalid_1() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss2/rss_2.0_invalid_1.xml");
    let feed = parser::parse(test_data.as_bytes());
    assert!(feed.is_err());
}

// Verifies that we can handle non-UTF8 streams
#[test]
fn test_encoding_1() {
    let test_data = test::fixture_as_raw("rss2/rss_2.0_encoding_1.xml");
    let feed = parser::parse(test_data.as_slice()).unwrap();
    assert_eq!(feed.title.unwrap().content, "RSS Feed do Site Inovação Tecnológica");
}

// Verifies we extract the content:encoded element
#[test]
fn test_heated() {
    let test_data = test::fixture_as_raw("rss2/rss_2.0_heated.xml");
    let feed = parser::parse(test_data.as_slice()).unwrap();
    let content = &feed.entries[0].content.as_ref().unwrap();
    assert!(content.body.as_ref().unwrap().contains("I have some good news and some bad news"));
    assert_eq!(content.content_type, "text/html");
}

// Check reported issue that RockPaperShotgun does not extract summary
#[test]
fn test_rockpapershotgun() {
    let test_data = test::fixture_as_raw("rss2/rss_2.0_rps.xml");
    let feed = parser::parse(test_data.as_slice()).unwrap();

    // Feed should have a description
    assert!(!feed.description.unwrap().content.is_empty());

    // Entry should too
    let entry = &feed.entries[0];
    assert!(!entry.summary.as_ref().unwrap().content.is_empty());
}

// Verifies that we can handle mixed MediaRSS and itunes/enclosure
#[test]
fn test_spiegel() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss2/rss_2.0_spiegel.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::RSS2)
        .id(actual.id.as_ref()) // not present in the test data
        .language("de")
        .title(Text::new("SPIEGEL Update – Die Nachrichten".into()))
        .author(Person::new("DER SPIEGEL"))
        .link(Link::new("https://www.omnycontent.com/d/playlist/5ac1e950-45c7-4eb7-87c0-aa0f018441b8/bb17ca27-51f4-4349-bc1e-abc00102c975/4c18e072-24d2-4d60-9a42-abc00102c97e/podcast.rss", None)
            .rel("self")
            .media_type("application/rss+xml"))
        .link(Link::new("https://www.omnycontent.com/d/playlist/5ac1e950-45c7-4eb7-87c0-aa0f018441b8/bb17ca27-51f4-4349-bc1e-abc00102c975/4c18e072-24d2-4d60-9a42-abc00102c97e/podcast.rss?page=2", None)
            .rel("next")
            .media_type("application/rss+xml"))
        .link(Link::new("https://www.omnycontent.com/d/playlist/5ac1e950-45c7-4eb7-87c0-aa0f018441b8/bb17ca27-51f4-4349-bc1e-abc00102c975/4c18e072-24d2-4d60-9a42-abc00102c97e/podcast.rss?page=1", None)
            .rel("first")
            .media_type("application/rss+xml"))
        .link(Link::new("https://www.omnycontent.com/d/playlist/5ac1e950-45c7-4eb7-87c0-aa0f018441b8/bb17ca27-51f4-4349-bc1e-abc00102c975/4c18e072-24d2-4d60-9a42-abc00102c97e/podcast.rss?page=7", None)
            .rel("last")
            .media_type("application/rss+xml"))
        .link(Link::new("https://www.spiegel.de/thema/spiegel-update/", None))
        .category(Category::new("News"))
        .contributor(Person::new("SPIEGEL Update – Die Nachrichten").email("charlotte.meyer-hamme@spiegel.de"))
        .description(Text::new("<p>Die wichtigsten Nachrichten des Tages &ndash; erg&auml;nzt um Meinungen und Empfehlungen aus der SPIEGEL-Redaktion. Wochentags aktualisieren wir morgens, mittags und abends unsere Meldungen. Am Wochenende blicken wir zur&uuml;ck auf die vergangene Woche &ndash; und erkl&auml;ren, was in der n&auml;chsten Woche wichtig wird.</p>".into()))
        .rights(Text::new("2021 DER SPIEGEL GmbH & Co. KG".into()))
        .logo(Image::new("https://www.omnycontent.com/d/programs/5ac1e950-45c7-4eb7-87c0-aa0f018441b8/bb17ca27-51f4-4349-bc1e-abc00102c975/image.jpg?t=1589902935&size=Large".into())
            .title("SPIEGEL Update – Die Nachrichten")
            .link("https://www.spiegel.de/thema/spiegel-update/")
        )
        .entry(
            Entry::default()
                .title(Text::new("07.02. – die Wochenvorschau: Lockdown-Verlängerung, Kriegsverbrecher vor Gericht, Super Bowl, Karneval".into()))
                .content(Content::default().body(r#"Die wichtigsten Nachrichten aus der SPIEGEL-Redaktion. <br><br><p>See <a href="https://omnystudio.com/listener">omnystudio.com/listener</a> for privacy information.</p>"#).content_type("text/html"))
                .summary(Text::html("Die wichtigsten Nachrichten aus der SPIEGEL-Redaktion. \nSee omnystudio.com/listener for privacy information.".into()))
                .link(Link::new("https://omny.fm/shows/spiegel-update-die-nachrichten/07-02-die-wochenvorschau-lockdown-verl-ngerung-kri", None))
                .published("2021-02-06T23:01:00Z")
                .id("c7e3cca2-665e-4bc4-bcac-acc6011b9fa2")
                // <enclosure>, media: and itunes: tags
                .media(MediaObject::default()
                    .title("07.02. – die Wochenvorschau: Lockdown-Verlängerung, Kriegsverbrecher vor Gericht, Super Bowl, Karneval ")
                    .description("Die wichtigsten Nachrichten aus der SPIEGEL-Redaktion. \nSee omnystudio.com/listener for privacy information.")
                    .credit("DER SPIEGEL")
                    .thumbnail(MediaThumbnail::new(Image::new("https://www.omnycontent.com/d/programs/5ac1e950-45c7-4eb7-87c0-aa0f018441b8/bb17ca27-51f4-4349-bc1e-abc00102c975/image.jpg?t=1589902935&size=Large".into())))
                    .content(MediaContent::new()
                        .url("https://traffic.omny.fm/d/clips/5ac1e950-45c7-4eb7-87c0-aa0f018441b8/bb17ca27-51f4-4349-bc1e-abc00102c975/c7e3cca2-665e-4bc4-bcac-acc6011b9fa2/audio.mp3?utm_source=Podcast&in_playlist=4c18e072-24d2-4d60-9a42-abc00102c97e&t=1612652510")
                        .content_type("audio/mpeg")
                    )
                    .content(MediaContent::new()
                        .url("https://www.omnycontent.com/d/programs/5ac1e950-45c7-4eb7-87c0-aa0f018441b8/bb17ca27-51f4-4349-bc1e-abc00102c975/image.jpg?t=1589902935&size=Large")
                        .content_type("image/jpeg")
                    )
                    .content(MediaContent::new()
                        .url("https://traffic.omny.fm/d/clips/5ac1e950-45c7-4eb7-87c0-aa0f018441b8/bb17ca27-51f4-4349-bc1e-abc00102c975/c7e3cca2-665e-4bc4-bcac-acc6011b9fa2/audio.mp3?utm_source=Podcast&in_playlist=4c18e072-24d2-4d60-9a42-abc00102c97e&t=1612652510")
                        .size(2519606)
                        .content_type("audio/mpeg")
                    )
                    .duration(Duration::from_secs(312))
                )
        );

    // Check
    assert_eq!(actual, expected);
}

// Check that we ignore tags in unknown namespaces
// In the case of this feed, we ignore googleplay, and don't overwrite the RSS2 image
#[test]
fn test_spreaker_ignore_unknown_ns() {
    let test_data = test::fixture_as_raw("rss2/rss_2.0_spreaker.xml");
    let feed = parser::parse(test_data.as_slice()).unwrap();

    // Feed should have a logo
    assert!(feed.logo.is_some());
}

// Verifies that we can handle mixed MediaRSS and itunes/enclosure
#[test]
fn test_bbc() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss2/rss_2.0_bbc.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::RSS2)
        .id(actual.id.as_ref()) // not present in the test data
        .title(Text::new("In Our Time".into()))
        .link(Link::new("http://www.bbc.co.uk/programmes/b006qykl", None))
        .link(
            Link::new("http://www.bbc.co.uk/programmes/b006qykl/episodes/downloads.rss", None)
                .rel("self")
                .media_type("application/rss+xml"),
        )
        .category(Category::new("History"))
        .description(Text::new("Melvyn Bragg and guests discuss the history of ideas".into()))
        .author(Person::new("BBC Radio 4"))
        .contributor(Person::new("BBC").email("RadioMusic.Support@bbc.co.uk"))
        .language("en")
        .logo(
            Image::new("http://ichef.bbci.co.uk/images/ic/3000x3000/p087hyhs.jpg".into())
                .title("In Our Time")
                .link("http://www.bbc.co.uk/programmes/b006qykl"),
        )
        .rights(Text::new("(C) BBC 2021".into()))
        .published("Thu, 25 Feb 2021 10:15:00 +0000")
        .entry(
            Entry::default()
                .title(Text::new("Marcus Aurelius".into()))
                .summary(Text::html("Melvyn Bragg and guests discuss...".into()))
                .published("Thu, 25 Feb 2021 10:15:00 +0000")
                .id("urn:bbc:podcast:m000sjxt")
                .link(Link::new("http://www.bbc.co.uk/programmes/m000sjxt", None))
                // <enclosure>,  media: and itunes: tags
                .media(
                    MediaObject::default()
                        .description("Melvyn Bragg and guests discuss the man who, according to Machiavelli...")
                        .duration(Duration::from_secs(3156))
                        .content(
                            MediaContent::new()
                                .url("http://open.live.bbc.co.uk/mediaselector/6/redir/version/2.0/mediaset/audio-nondrm-download/proto/http/vpid/p097wt5b.mp3")
                                .size(50496000)
                                .content_type("audio/mpeg"),
                        )
                        .content(
                            MediaContent::new()
                                .url("http://open.live.bbc.co.uk/mediaselector/6/redir/version/2.0/mediaset/audio-nondrm-download/proto/http/vpid/p097wt5b.mp3")
                                .size(50496000)
                                .content_type("audio/mpeg")
                                .duration(Duration::from_secs(3156)),
                        )
                        .credit("BBC Radio 4"),
                ),
        );

    // Check
    assert_eq!(actual, expected);
}

// Verifies that we can handle mixed MediaRSS and itunes/enclosure
#[test]
fn test_ch9() {
    // Parse the feed
    let test_data = test::fixture_as_string("rss2/rss_2.0_ch9.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::RSS2)
        .id(actual.id.as_ref()) // not present in the test data
        .title(Text::new("Azure Friday (HD) - Channel 9".into()))
        .logo(
            Image::new("https://f.ch9.ms/thumbnail/4761e196-da48-4b41-abfe-e56e0509f04d.png".into())
                .title("Azure Friday (HD) - Channel 9")
                .link("https://s.ch9.ms/Shows/Azure-Friday"),
        )
        .description(Text::new(
            "Join Scott Hanselman, Donovan Brown, or Lara Rubbelke as they host the engineers who build Azure, demo it, answer questions, and share insights."
                .into(),
        ))
        .link(
            Link::new("https://s.ch9.ms/Shows/Azure-Friday/feed/mp4high", None)
                .rel("self")
                .media_type("application/rss+xml"),
        )
        .link(Link::new("https://s.ch9.ms/Shows/Azure-Friday", None))
        .category(Category::new("Technology"))
        .language("en")
        .published("Sat, 27 Feb 2021 06:55:01 GMT")
        .updated_parsed("Sat, 27 Feb 2021 06:55:01 GMT")
        .author(Person::new("Microsoft"))
        .generator(Generator::new("Rev9"))
        .entry(
            Entry::default()
                .title(Text::new("Troubleshoot AKS cluster issues with AKS Diagnostics and AKS Periscope".into()))
                .summary(Text::html("<p>Yun Jung Choi shows Scott Hanselman...".into()))
                .link(Link::new(
                    "https://channel9.msdn.com/Shows/Azure-Friday/Troubleshoot-AKS-cluster-issues-with-AKS-Diagnostics-and-AKS-Periscope",
                    None,
                ))
                .published("Fri, 26 Feb 2021 20:00:00 GMT")
                .updated_parsed("Sat, 27 Feb 2021 06:55:01 GMT")
                .id("https://channel9.msdn.com/Shows/Azure-Friday/Troubleshoot-AKS-cluster-issues-with-AKS-Diagnostics-and-AKS-Periscope")
                .author(Person::new("Scott Hanselman, Rob Caron"))
                .category(Category::new("Azure"))
                .category(Category::new("Kubernetes"))
                .category(Category::new("aft"))
                // <media:group>
                .media(
                    MediaObject::default()
                        .content(
                            MediaContent::new()
                                .url("https://rev9.blob.core.windows.net/mfupload/04b236b5-e824-4091-85d8-acd90155d4b0_20210124205102.mp4")
                                .duration(Duration::from_secs(867))
                                .size(1)
                                .content_type("video/mp4"),
                        )
                        .content(
                            MediaContent::new()
                                .url("https://sec.ch9.ms/ch9/075d/6e61e6c6-3890-4172-a617-fa0c4b38075d/azfr663.mp3")
                                .duration(Duration::from_secs(867))
                                .size(13878646)
                                .content_type("audio/mp3"),
                        )
                        .content(
                            MediaContent::new()
                                .url("https://sec.ch9.ms/ch9/075d/6e61e6c6-3890-4172-a617-fa0c4b38075d/azfr663.mp4")
                                .duration(Duration::from_secs(867))
                                .size(20450133)
                                .content_type("video/mp4"),
                        )
                        .content(
                            MediaContent::new()
                                .url("https://sec.ch9.ms/ch9/075d/6e61e6c6-3890-4172-a617-fa0c4b38075d/azfr663_high.mp4")
                                .duration(Duration::from_secs(867))
                                .size(126659374)
                                .content_type("video/mp4"),
                        )
                        .content(
                            MediaContent::new()
                                .url("https://sec.ch9.ms/ch9/075d/6e61e6c6-3890-4172-a617-fa0c4b38075d/azfr663_mid.mp4")
                                .duration(Duration::from_secs(867))
                                .size(49241848)
                                .content_type("video/mp4"),
                        )
                        .content(
                            MediaContent::new()
                                .url("https://www.youtube-nocookie.com/embed/E-XqYb88hUY?enablejsapi=1")
                                .duration(Duration::from_secs(867))
                                .size(1),
                        ),
                )
                // <enclosure> and <media:*>
                .media(
                    MediaObject::default()
                        .description("Yun Jung Choi shows Scott Hanselman how to use AKS Diagnostics...")
                        .duration(Duration::from_secs(867))
                        .thumbnail(MediaThumbnail::new(
                            Image::new("https://sec.ch9.ms/ch9/3724/8609074c-2b7b-41ae-9345-f49973543724/azfr663_100.jpg".into())
                                .height(56)
                                .width(100),
                        ))
                        .thumbnail(MediaThumbnail::new(
                            Image::new("https://sec.ch9.ms/ch9/3724/8609074c-2b7b-41ae-9345-f49973543724/azfr663_220.jpg".into())
                                .height(123)
                                .width(220),
                        ))
                        .thumbnail(MediaThumbnail::new(
                            Image::new("https://sec.ch9.ms/ch9/3724/8609074c-2b7b-41ae-9345-f49973543724/azfr663_512.jpg".into())
                                .height(288)
                                .width(512),
                        ))
                        .thumbnail(MediaThumbnail::new(
                            Image::new("https://sec.ch9.ms/ch9/3724/8609074c-2b7b-41ae-9345-f49973543724/azfr663_960.jpg".into())
                                .height(540)
                                .width(960),
                        ))
                        .content(
                            MediaContent::new()
                                .url("https://sec.ch9.ms/ch9/075d/6e61e6c6-3890-4172-a617-fa0c4b38075d/azfr663_high.mp4")
                                .size(126659374)
                                .content_type("video/mp4"),
                        )
                        .credit("Scott Hanselman, Rob Caron"),
                ),
        );

    // Check
    assert_eq!(actual, expected);
}

// Verifies that we handle relative URLs for links on the <content:encoded> element
#[test]
fn test_relurl_1() {
    // This example feed uses the xml:base standard so we don't need to pass the source URI
    let test_data = test::fixture_as_string("rss2/rss_2.0_relurl_1.xml");
    let actual = parser::parse_with_uri(test_data.as_bytes(), None).unwrap();

    // Check the links in the feed
    let content = actual.entries[0].content.as_ref().unwrap();
    assert_eq!(
        content.src,
        Some(Link::new("https://insanity.industries/post/pareto-optimal-compression/", None))
    );
    let content = actual.entries[1].content.as_ref().unwrap();
    assert_eq!(
        content.src,
        Some(Link::new("https://insanity.industries/post/pacman-tracking-leftover-packages/", None))
    );
}

// Verifies that we handle relative URLs for links on the enclosure element
#[test]
fn test_relurl_2() {
    // This example feed does not use the xml:base standard so we test using a provided feed URI
    let test_data = test::fixture_as_string("rss2/rss_2.0_relurl_2.xml");
    let actual = parser::parse_with_uri(test_data.as_bytes(), Some("http://example.com")).unwrap();

    // The link for the enclosure should be absolute
    let content = &actual.entries[0].media[0].content[0];
    assert_eq!(content.url, Url::parse("http://example.com/images/me/hackergotchi-simpler.png").ok());
}

// Verify that attributes containing escaped characters are decoded correctly
#[test]
fn test_escaped_attributes() {
    let test_data = test::fixture_as_raw("rss2/rss_2.0_reddit.xml");
    let feed = parser::parse(test_data.as_slice()).unwrap();

    assert_eq!(
        feed.links[0].href,
        "https://www.reddit.com/search.rss?q=site%3Akevincox.ca&restrict_sr=&sort=new&t=all"
    );
}

// Verify that valid XML with no whitespace separation is parsed correctly
#[test]
fn test_ghost_no_ws() {
    let test_data = test::fixture_as_raw("rss2/rss_2.0_ghost_1.xml");
    let feed = parser::parse(test_data.as_slice()).unwrap();

    // Entry should have content
    for entry in feed.entries {
        assert!(entry.content.is_some());
    }
}
// Verifies that we extract the 'content:encoded' element correctly from a variety of problematic feeds
#[test]
fn test_ghost_feeds() {
    let files = vec!["rss_2.0_ghost_2.xml", "rss_2.0_cloudflare.xml", "rss_2.0_element_io.xml"];
    for file in files {
        let test_data = test::fixture_as_string(&format!("rss2/{}", file));
        let actual = parser::parse(test_data.as_bytes()).unwrap();
        for entry in actual.entries {
            assert!(entry.content.is_some());
        }
    }
}

// Verifies that content is not set (as there is no enclosure in this case)
#[test]
fn test_matrix() {
    let test_data = test::fixture_as_string("rss2/rss_2.0_matrix.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();
    let entry = actual.entries.get(0).expect("feed has 1 entry");

    // The content should not be present
    assert!(entry.content.is_none());
}

// Verifies we can handle an RFC1123 date in an RSS 2.0 feed
#[test]
fn test_rfc1123_ilgiornale() {
    let test_data = test::fixture_as_string("rss2/rss_2.0_ilgiornale.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();
    let entry = actual.entries.get(0).expect("feed has 1 entry");

    // Should have the expected date
    assert_eq!(entry.published.unwrap(), Utc.with_ymd_and_hms(2022, 11, 15, 20, 15, 4).unwrap());
}

// Verifies we can handle an RFC1123 date in an RSS 2.0 feed where the week day name is in a different language
#[test]
fn test_rfc1123_ilmessaggero() {
    let test_data = test::fixture_as_string("rss2/rss_2.0_ilmessaggero.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();
    let entry = actual.entries.get(0).expect("feed has 1 entry");

    // Should have the expected date
    assert_eq!(entry.published.unwrap(), Utc.with_ymd_and_hms(2022, 11, 15, 23, 38, 15).unwrap());
}

// Verifies we trim leading and trailing whitespace in text fields
// e.g. those with newlines before the CDATA
#[test]
fn test_trim_whitespace_text_nodes() {
    let test_data = test::fixture_as_string("rss2/rss_2.0_nightvale.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    assert!(actual.description.unwrap().content.starts_with("<p>Twice-monthly community updates"));

    let entry = actual.entries.get(0).expect("feed has 1 entry");
    assert!(entry.summary.as_ref().unwrap().content.starts_with("<p>The University of What It Is"));

    let media = entry.media.get(0).expect("entry has 1 media item");
    assert!(media.description.as_ref().unwrap().content.starts_with("The University of What It Is"));
}

// Verifies we use DublinCore date as entry published date if present
#[test]
fn test_published_from_dc_date() {
    let test_data = test::fixture_as_string("rss2/rss_2.0_dbengines.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();
    let entry = actual.entries.get(0).expect("feed has 1 entry");
    assert_eq!(entry.published.unwrap(), Utc.with_ymd_and_hms(2023, 1, 3, 15, 0, 0).unwrap());
}

// Verifies that an custom parser is correctly called and can return a useful date
#[test]
fn test_custom_timestamp_parser() {
    let test_data = test::fixture_as_string("rss2/rss_2.0_nbcny.xml");

    let actual = parser::Builder::new()
        .timestamp_parser(|text| {
            if text == "Sat, Dec 16 2023 02:02:33 PM" {
                util::parse_timestamp_lenient("Sat, 16 Dec 2023 19:02:33 UTC")
            } else {
                None
            }
        })
        .build()
        .parse(test_data.as_bytes())
        .unwrap();
    let entry = actual.entries.get(0).expect("feed has 1 entry");
    assert_eq!(entry.published.unwrap(), Utc.with_ymd_and_hms(2023, 12, 16, 19, 2, 33).unwrap());
}

// Verifies we correctly extract subcategories from the iTunes NS
#[test]
fn test_subcategories() {
    let test_data = test::fixture_as_string("rss2/rss_2.0_anchorfm.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    let category = &actual.categories[0];
    assert_eq!("Kids & Family", category.term.as_str());

    let subcat = &category.subcategories[0];
    assert_eq!("Parenting", subcat.term.as_str());
}
