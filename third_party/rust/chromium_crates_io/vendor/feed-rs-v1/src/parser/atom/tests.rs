use crate::model::{
    Category, Content, Entry, Feed, FeedType, Generator, Image, Link, MediaCommunity, MediaContent, MediaObject, MediaText, MediaThumbnail, Person, Text,
};
use crate::parser;
use crate::util::test;

// Verify we can parse a more complete example than the one provided in the standard
#[test]
fn test_example_1() {
    // Parse the feed
    let test_data = test::fixture_as_string("atom/atom_example_1.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::Atom)
        .title(Text::new("dive into mark".into()))
        .description(Text::new("A <em>lot</em> of effort\n        went into making this effortless".into()).content_type("text/html"))
        .updated_parsed("2005-07-31T12:29:29Z")
        .id("tag:example.org,2003:3")
        .link(Link::new("http://example.org/", None).rel("alternate").media_type("text/html").href_lang("en"))
        .link(Link::new("http://example.org/feed.atom", None).rel("self").media_type("application/atom+xml"))
        .rights(Text::new("Copyright (c) 2003, Mark Pilgrim".into()))
        .generator(Generator::new("Example Toolkit").uri("http://www.example.com/").version("1.0"))
        .entry(
            Entry::default()
                .id("tag:example.org,2003:3.2397")
                .title(Text::new("Atom draft-07 snapshot".into()))
                .updated_parsed("2005-07-31T12:29:29Z")
                .language("en")
                .base("http://diveintomark.org/")
                .author(Person::new("Mark Pilgrim").uri("http://example.org/").email("f8dy@example.com"))
                .link(Link::new("http://example.org/2005/04/02/atom", None).rel("alternate").media_type("text/html"))
                .link(
                    Link::new("http://example.org/audio/ph34r_my_podcast.mp3", None)
                        .rel("enclosure")
                        .media_type("audio/mpeg")
                        .length(1337),
                )
                .contributor(Person::new("Sam Ruby"))
                .contributor(Person::new("Joe Gregorio"))
                .content(Content::default().content_type("text/html").body(
                    "<div>\n                <p>\n                    <i>[Update: The Atom draft is finished.]</i>\n                </p>\n            </div>",
                ))
                .published("2003-12-13T08:29:29-04:00"),
        );

    // Check
    assert_eq!(actual, expected);
}

// Real-life example from the Register - https://www.theregister.co.uk/science/headlines.atom
#[test]
fn test_example_2() {
    // Parse the feed
    let test_data = test::fixture_as_string("atom/atom_example_2.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    let expected = Feed::new(FeedType::Atom)
        .id("tag:theregister.co.uk,2005:feed/theregister.co.uk/science/")
        .language("en")
        .title(Text::new("The Register - Science".into()))
        .link(Link::new("https://www.theregister.co.uk/science/headlines.atom", None)
            .rel("self")
            .media_type("application/atom+xml"))
        .link(Link::new("https://www.theregister.co.uk/science/", None)
            .rel("alternate")
            .media_type("text/html"))
        .rights(Text::new("Copyright © 2019, Situation Publishing".into()))
        .author(Person::new("Team Register")
            .email("webmaster@theregister.co.uk")
            .uri("https://www.theregister.co.uk/odds/about/contact/"))
        .icon(Image::new("https://www.theregister.co.uk/Design/graphics/icons/favicon.png".into()))
        .description(Text::new("Biting the hand that feeds IT — sci/tech news and views for the world".into()))
        .logo(Image::new("https://www.theregister.co.uk/Design/graphics/Reg_default/The_Register_r.png".into()))
        .updated_parsed("2019-07-31T11:54:28Z")
        .entry(Entry::default()
            .id("tag:theregister.co.uk,2005:story204156")
            .updated_parsed("2019-07-31T11:54:28Z")
            .author(Person::new("Richard Speed")
                .uri("https://search.theregister.co.uk/?author=Richard%20Speed"))
            .link(Link::new("http://go.theregister.com/feed/www.theregister.co.uk/2019/07/31/orbitbeyond_drops_nasa_moon_contract/", None)
                .rel("alternate")
                .media_type("text/html"))
            .title(Text::new("Will someone plz dump our shizz on the Moon, NASA begs as one of the space biz vendors drops out".into())
                .content_type("text/html"))
            .summary(Text::new("<h4>OrbitBeyond begone: Getting to the Moon is <i>hard</i></h4> <p>NASA made a slew of announcements yesterday aimed at bigging up the agency's efforts to get commercial companies involved with its deep space ambitions – despite one vendor dumping plans for a 2020 lunar landing.…</p>".into())
                .content_type("text/html")))
        .entry(Entry::default()
            .id("tag:theregister.co.uk,2005:story204131")
            .updated_parsed("2019-07-30T05:41:09Z")
            .author(Person::new("Kieren McCarthy")
                .uri("https://search.theregister.co.uk/?author=Kieren%20McCarthy"))
            .link(Link::new("http://go.theregister.com/feed/www.theregister.co.uk/2019/07/30/french_arming_satellites/", None)
                .rel("alternate")
                .media_type("text/html"))
            .title(Text::new("Satellites with lasers and machine guns coming! China's new plans? Trump's Space Force? Nope, the French".into())
                .content_type("text/html"))
            .summary(Text::new(r#"<h4>After all, what could possibly go wrong, apart from everything?</h4> <p>France is threatening to stick submachine guns on its next generation of satellites as part of an "active space defense" strategy that would enable it to shoot down other space hardware.…</p>"#.to_owned())
                .content_type("text/html")));

    // Check
    assert_eq!(actual, expected);
}

// Real-life example from Akamai (includes categories and elements from a different namespace, along with locally declared namespaces on atom 1.0 elements)
#[test]
fn test_example_3() {
    // Parse the feed
    let test_data = test::fixture_as_string("atom/atom_example_3.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    let expected = Feed::new(FeedType::Atom)
        .title(Text::new("The Akamai Blog".into()))
        .link(Link::new("https://blogs.akamai.com/", None)
            .rel("alternate")
            .media_type("text/html"))
        .id("tag:blogs.akamai.com,2019-07-30://2")
        .updated_parsed("2019-07-30T15:02:05Z")
        .generator(Generator::new("Movable Type Pro 5.2.13")
            .uri("http://www.sixapart.com/movabletype/"))
        .link(Link::new("http://feeds.feedburner.com/TheAkamaiBlog", None)
            .rel("self")
            .media_type("application/atom+xml"))
        .link(Link::new("http://pubsubhubbub.appspot.com/", None)
            .rel("hub"))
        .entry(Entry::default()
            .title(Text::new("Time to Transfer Risk: Why Security Complexity & VPNs Are No Longer Sustainable".into()))
            .language("en-us")
            .base("https://blogs.akamai.com/")
            .link(Link::new("http://feedproxy.google.com/~r/TheAkamaiBlog/~3/NnQEuqRSyug/time-to-transfer-risk-why-security-complexity-vpns-are-no-longer-sustainable.html", None)
                .rel("alternate")
                .media_type("text/html"))
            .id("tag:blogs.akamai.com,2019://2.3337")
            .published("2019-07-30T16:00:00Z")
            .updated_parsed("2019-07-30T15:02:05Z")
            .summary(Text::new("Now, there are many reasons to isolate your infrastructure from the Internet. Minimizing the number of exposed things not only reduces risk, it also reduces operational complexity. VPNs are counter to this. VPNs make it so you aren't exposing all of your applications publicly in a DMZ, which is good. But for the most part, they still provide access to the corporate network to get access to corporate apps. Definitely bad. At this point, I think we all agree that moats and castles belong in the past.".into()))
            .author(Person::new("Lorenz Jakober"))
            .category(Category::new("Zero Trust")
                .scheme("http://www.sixapart.com/ns/types#category"))
            .category(Category::new("ssl")
                .label("SSL")
                .scheme("http://www.sixapart.com/ns/types#tag"))
            .category(Category::new("zerotrust")
                .label("Zero Trust")
                .scheme("http://www.sixapart.com/ns/types#tag"))
            .content(Content::default()
                .body(r#"<p>We all heed the gospel of patching, but as recent incidents made clear, even cutting-edge disruptors struggle to patch everything, everywhere, and all the time.</p>
        <img src="http://feeds.feedburner.com/~r/TheAkamaiBlog/~4/NnQEuqRSyug" height="1" width="1" alt=""/>"#)
                .content_type("text/html")));

    // Check
    assert_eq!(actual, expected);
}

// Real-life example from ebmpapst (CDATA text elements, unusual category representation)
#[test]
fn test_example_4() {
    // Parse the feed
    let test_data = test::fixture_as_string("atom/atom_example_4.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    let expected = Feed::new(FeedType::Atom)
        .author(Person::new("ebm-papst"))
        .link(Link::new("http://www.ebmpapst.com/en/ebmpapst_productnews_atom_feed.xml", None)
            .rel("self")
            .media_type("application/atom+xml"))
        .title(Text::new("ebm-papst product news".into()))
        .id("tag:ebmpapst.com,2011-06-30:1309426729931")
        .updated_parsed("2019-07-29T09:41:09Z")
        .entry(Entry::default()
            .title(Text::new(" Connection with future".into()))
            .link(Link::new("https://idt.ebmpapst.com/de/en/idt/campaign/simatic-micro-drive.html", None)
                .rel("alternate"))
            .id("tag:ebmpapst.com,2019-07-17:0310161724098")
            .updated_parsed("2019-07-17T03:10:16Z")
            .summary(Text::new(r#" <a href="https://idt.ebmpapst.com/de/en/idt/campaign/simatic-micro-drive.html"><img src="http://www.ebmpapst.com//media/content/homepage/currenttopic/ads_cd2013/FF_ep_keyvisual_100px.jpg" border="0" align="right"></a> Working in perfect harmony: the ebm-papst drive solutions for SIMATIC MICRO-DRIVE drive regulators from Siemens. "#.to_owned())
                .content_type("text/html")));

    // Check
    assert_eq!(actual, expected);
}

// Real-life example from USGS (CDATA, different namespaces)
#[test]
fn test_example_5() {
    // Parse the feed
    let test_data = test::fixture_as_string("atom/atom_example_5.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    let expected = Feed::new(FeedType::Atom)
        .title(Text::new("USGS Magnitude 2.5+ Earthquakes, Past Hour".into()))
        .updated_parsed("2019-07-31T13:17:27Z")
        .author(Person::new("U.S. Geological Survey")
            .uri("https://earthquake.usgs.gov/"))
        .id("https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/2.5_hour.atom")
        .link(Link::new("https://earthquake.usgs.gov/earthquakes/feed/v1.0/summary/2.5_hour.atom", None)
            .rel("self"))
        .icon(Image::new("https://earthquake.usgs.gov/favicon.ico".into()))
        .entry(Entry::default()
            .id("urn:earthquake-usgs-gov:nc:73239366")
            .title(Text::new("M 3.6 - 15km W of Petrolia, CA".into()))
            .updated_parsed("2019-07-31T13:07:31.364Z")
            .link(Link::new("https://earthquake.usgs.gov/earthquakes/eventpage/nc73239366", None)
                .rel("alternate")
                .media_type("text/html"))
            .summary(Text::new(r#"
            <p class="quicksummary"><a href="https://earthquake.usgs.gov/earthquakes/eventpage/nc73239366#shakemap" title="ShakeMap maximum estimated intensity" class="mmi-II">ShakeMap - <strong class="roman">II</strong></a> <a href="https://earthquake.usgs.gov/earthquakes/eventpage/nc73239366#dyfi" class="mmi-IV" title="Did You Feel It? maximum reported intensity (4 reports)">DYFI? - <strong class="roman">IV</strong></a></p><dl><dt>Time</dt><dd>2019-07-31 12:26:15 UTC</dd><dd>2019-07-31 04:26:15 -08:00 at epicenter</dd><dt>Location</dt><dd>40.347&deg;N 124.460&deg;W</dd><dt>Depth</dt><dd>29.35 km (18.24 mi)</dd></dl>"#.to_owned())
                .content_type("text/html"))
            .category(Category::new("Past Hour")
                .label("Age"))
            .category(Category::new("Magnitude 3")
                .label("Magnitude"))
            .category(Category::new("nc")
                .label("Contributor"))
            .category(Category::new("nc")
                .label("Author")));

    // Check
    assert_eq!(actual, expected);
}

// GitHub Atom feed for feed-rs (https://github.com/feed-rs/feed-rs/issues/6)
#[test]
fn test_example_6() {
    // Parse the feed
    let test_data = test::fixture_as_string("atom/atom_example_6.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    let expected = Feed::new(FeedType::Atom)
        .id("tag:github.com,2008:https://github.com/feed-rs/feed-rs/releases")
        .language("en-US")
        .link(
            Link::new("https://github.com/feed-rs/feed-rs/releases", None)
                .rel("alternate")
                .media_type("text/html"),
        )
        .link(
            Link::new("https://github.com/feed-rs/feed-rs/releases.atom", None)
                .rel("self")
                .media_type("application/atom+xml"),
        )
        .title(Text::new("Release notes from feed-rs".into()))
        .updated_parsed("2020-01-19T16:01:56+11:00")
        .entry(
            Entry::default()
                .id("tag:github.com,2008:Repository/90976281/v0.2.0")
                .updated_parsed("2020-01-19T16:08:59+11:00")
                .link(
                    Link::new("https://github.com/feed-rs/feed-rs/releases/tag/v0.2.0", None)
                        .rel("alternate")
                        .media_type("text/html"),
                )
                .title(Text::new("0.2.0".into()))
                .content(
                    Content::default()
                        .body(
                            r#"<p>A range of maintenance work, including:</p>
            <ul>
            <li>migrate to Rust 2018 edition</li>
            <li>Align domain model around Atom spec as it is more modern+complete</li>
            <li>Switch to event-based parser (xml-rs) to reduce peak memory usage and use of clone()</li>
            <li>Expanded test coverage</li>
            <li>Documentation improvements</li>
            </ul>"#,
                        )
                        .content_type("text/html"),
                )
                .author(Person::new("markpritchard"))
                .media(
                    MediaObject::default().thumbnail(MediaThumbnail::new(
                        Image::new("https://avatars3.githubusercontent.com/u/8234070?s=60&v=4".to_owned())
                            .width(30)
                            .height(30),
                    )),
                ),
        )
        .entry(
            Entry::default()
                .id("tag:github.com,2008:Repository/90976281/0.1.3")
                .updated_parsed("2017-07-07T21:47:46+10:00")
                .link(
                    Link::new("https://github.com/feed-rs/feed-rs/releases/tag/0.1.3", None)
                        .rel("alternate")
                        .media_type("text/html"),
                )
                .title(Text::new("0.1.3".into()))
                .content(Content::default().body(r#"<p>Update version to 0.1.3</p>"#).content_type("text/html"))
                .author(Person::new("kumabook"))
                .media(
                    MediaObject::default().thumbnail(MediaThumbnail::new(
                        Image::new("https://avatars1.githubusercontent.com/u/753703?s=60&v=4".to_owned())
                            .width(30)
                            .height(30),
                    )),
                ),
        )
        .entry(
            Entry::default()
                .id("tag:github.com,2008:Repository/90976281/0.1.1")
                .updated_parsed("2017-06-16T18:49:36+10:00")
                .link(
                    Link::new("https://github.com/feed-rs/feed-rs/releases/tag/0.1.1", None)
                        .rel("alternate")
                        .media_type("text/html"),
                )
                .title(Text::new("0.1.1".into()))
                .content(
                    Content::default()
                        .body(r#"<p>Handle rel attribute of link element of entry of atom</p>"#)
                        .content_type("text/html"),
                )
                .author(Person::new("kumabook"))
                .media(
                    MediaObject::default().thumbnail(MediaThumbnail::new(
                        Image::new("https://avatars1.githubusercontent.com/u/753703?s=60&v=4".to_owned())
                            .width(30)
                            .height(30),
                    )),
                ),
        )
        .entry(
            Entry::default()
                .id("tag:github.com,2008:Repository/90976281/0.1.0")
                .updated_parsed("2017-06-15T16:44:26+10:00")
                .link(
                    Link::new("https://github.com/feed-rs/feed-rs/releases/tag/0.1.0", None)
                        .rel("alternate")
                        .media_type("text/html"),
                )
                .title(Text::new("0.1.0".into()))
                .content(Content::default().body(r#"<p>Update crate info to Cargo.toml</p>"#).content_type("text/html"))
                .author(Person::new("kumabook"))
                .media(
                    MediaObject::default().thumbnail(MediaThumbnail::new(
                        Image::new("https://avatars1.githubusercontent.com/u/753703?s=60&v=4".to_owned())
                            .width(30)
                            .height(30),
                    )),
                ),
        );

    // Check
    assert_eq!(actual, expected);
}

// Verify that we don't trim essential whitespace
#[test]
fn test_example_7() {
    let expected_body = r#"<div xmlns="http://www.w3.org/1999/xhtml"><p>This is a follow up from <a href="https://who-t.blogspot.com/2018/12/high-resolution-wheel-scrolling-on.html">the kernel support for high-resolution wheel scrolling</a> which you totally forgot about because it's already more then a year in the past and seriously, who has the attention span these days to remember this. Anyway, I finally found time and motivation to pick this up again and I started lining up the pieces like cans, for it only to be shot down by the commentary of strangers on the internet. The <a href="https://gitlab.freedesktop.org/wayland/wayland/-/merge_requests/72">Wayland merge request</a> lists the various pieces (libinput, wayland, weston, mutter, gtk and Xwayland) but for the impatient there's also an <a href="https://copr.fedorainfracloud.org/coprs/whot/high-resolution-wheel-scrolling/">Fedora 32 COPR</a>. For all you weirdos inexplicably not running the latest Fedora, well, you'll have to compile this yourself, just like I did. </p> <p>Let's recap: in v5.0 the kernel added new axes <b>REL_WHEEL_HI_RES</b> and <b>REL_HWHEEL_HI_RES</b> for all devices. On devices that actually support high-resolution wheel scrolling (Logitech and Microsoft mice, primarily) you'll get multiple hires events before the now-legacy <b>REL_WHEEL</b> events. On all other devices those two are in sync. </p> <p>Integrating this into the userspace stack was a bit of a mess at first, but I think the solution is good enough, even if it has a rather verbose explanation on how to handle it. The actual patches to integrate ended up being relatively simple. So let's see why it's a bit weird: </p> <p>When Wayland started, back in WhoahReallyThatLongAgo, scrolling was specified as the <b>wl_pointer.axis</b> event with a value in pixels. This works fine for touchpads, not so much for wheels. The early versions of Weston decreed that one wheel click was 10 pixels [1] and, perhaps surprisingly, the world kept on turning. When libinput was forked from Weston <a href="https://who-t.blogspot.com/2015/01/providing-physical-movement-of-wheel.html">an early change</a> was that wheel events would have two values - degrees of movement and click count ("discrete steps"). The wayland protocol was expanded to include the discrete steps as <b>wl_pointer.axis_discrete</b> as well. Then backwards compatibility reared its ugly head and Mutter, Weston, GTK all basically said: one discrete step equals 10 pixels so we multiply the discrete value by 10 and, perhaps surprisingly, the world kept on turning. </p> <p>This worked out well enough for a few years but with high resolution wheels we ran into a problem. Discrete steps are integers, so we can't send partial values. And the protocol is defined in a way that any tweaking of the behaviour would result in broken clients which, perhaps surprisingly, is a Bad Thing. This lead to the current proposal of separate events. <b>LIBINPUT_EVENT_POINTER_AXIS_WHEEL</b> and for Wayland the <b>wl_pointer.axis_v120</b> event, linked to above. These events are (like the kernel events) a parallel event stream to the previous events and effectively replace the <b>LIBINPUT_EVENT_POINTER_AXIS</b> and Wayland <b>wl_pointer.axis/axis_discrete</b> pair for wheel events (not so for touchpad or button scrolling though). </p> <p>The compositor side of things is relatively simple: take the events from libinput and pass the hires ones as v120 events and the lowres ones as v120 events with a value of zero. The client side takes the v120 events and uses them over <b>wl_pointer.axis/axis_discrete</b> unless one is zero in which case you can discard all axis events in that <b>wl_pointer.frame</b>. Since most client implementation already have the support for smooth scrolling (because, well, touchpads do exist) it's relatively simple to integrate - the new events just feed into the smooth scrolling code. And since you already have to do wheel emulation for that (because, well, old clients exist) wheel emulation is handled easily too. </p> <p>All that to provide buttery smooth [2] wheel scrolling. Or not, if your hardware doesn't support it. In which case, well, live with the warm fuzzy feeling that someone else has a better user experience now. Or soon, anyway. </p> <p><small>[1] with, I suspect, the scientific measurement of "yeah, that seems about alright"<br></br>[2] like butter out of a fridge, so still chunky but at least less so than before<br></br></small></p></div>"#;
    // Parse the feed
    let test_data = test::fixture_as_string("atom/atom_example_7.xml");
    let feed = parser::parse(test_data.as_bytes()).unwrap();
    let body = feed
        .entries
        .get(0)
        .map(|e| e.content.as_ref())
        .unwrap()
        .map(|c| c.body.as_ref())
        .unwrap()
        .unwrap();
    assert_eq!(body, expected_body);
}

// Verify that the Reddit ATOM feed parses correctly
#[test]
fn test_example_reddit() {
    let test_data = test::fixture_as_string("atom/atom_example_reddit.xml");
    let feed = parser::parse(test_data.as_bytes()).unwrap();
    for entry in feed.entries {
        assert!(entry.updated.is_some());
    }
}

// Verify we can parse the example contained in the Atom specification
// https://tools.ietf.org/html/rfc4287#section-1.1
#[test]
fn test_spec_1() {
    // Parse the feed
    let test_data = test::fixture_as_string("atom/atom_spec_1.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();

    // Expected feed
    let expected = Feed::new(FeedType::Atom)
        .id("urn:uuid:60a76c80-d399-11d9-b93C-0003939e0af6")
        .title(Text::new("Example Feed".into()))
        .link(Link::new("http://example.org/", None).rel("alternate"))
        .updated_parsed("2003-12-13T18:30:02Z")
        .author(Person::new("John Doe"))
        .entry(
            Entry::default()
                .id("urn:uuid:1225c695-cfb8-4ebb-aaaa-80da344efa6a")
                .title(Text::new("Atom-Powered Robots Run Amok".into()))
                .updated_parsed("2003-12-13T18:30:02Z")
                .summary(Text::new("Some text.".into()))
                .link(Link::new("http://example.org/2003/12/13/atom03", None).rel("alternate")),
        );

    // Check
    assert_eq!(actual, expected);
}

#[test]
fn test_relative_example() {
    let test_data = test::fixture_as_string("atom/atom_relative.xml");
    let feed = parser::parse_with_uri(test_data.as_bytes(), Some("https://example.com/blog/feed.xml")).unwrap();

    let feed_alternate_link = feed
        .links
        .iter()
        .find(|l| l.rel.as_deref() == Some("alternate"))
        .expect("feed has an alternate link");
    assert_eq!("https://example.com/blog/", feed_alternate_link.href);

    let icon = feed.icon.expect("feed has an icon");
    assert_eq!("https://example.com/favicon.ico", icon.uri);

    let logo = feed.logo.expect("feed has a logo");
    assert_eq!("https://example.com/blog/feed_logo.jpg", logo.uri);

    let entry = feed.entries.first().expect("feed has one entry");
    let alternate_link = entry
        .links
        .iter()
        .find(|l| l.rel.as_deref() == Some("alternate"))
        .expect("entry has an alternate link");
    assert_eq!("https://example.com/blog/2003/12/13/atom03", alternate_link.href);
}

// Verify we can parse Atom content elements without a type attribute
// https://tools.ietf.org/html/rfc5023#section-9.2.1
#[test]
fn test_pub_spec_1() {
    // Parse the feed
    let test_data = test::fixture_as_string("atom/atom_pub_spec_1.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap().id(""); // Clear randomly generated UUID

    // Expected feed
    let expected = Feed::new(FeedType::Atom).entry(
        Entry::default()
            .title(Text::new("Atom-Powered Robots Run Amok".into()))
            .id("urn:uuid:1225c695-cfb8-4ebb-aaaa-80da344efa6a")
            .updated_parsed("2003-12-13T18:30:02Z")
            .author(Person::new("John Doe"))
            .content(Content::default().content_type("text/plain").body("Some text.")),
    );

    // Check
    assert_eq!(actual, expected);
}

// Verify we can parse an Atom Entry (no feed)
#[test]
fn test_entry() {
    let test_data = test::fixture_as_string("atom/atom_entry_1.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap().id("");

    let expected = Feed::new(FeedType::Atom).entry(
        Entry::default()
            .title(Text::new("Specifications".into()))
            .id("urn:uuid:988EF5C55CDEA24EDE1251744888912")
            .updated_parsed("2009-08-31T18:55:12.569Z")
            .author(Person::new("S. A. Khuba"))
            .category(Category::new("45121504").scheme("http://www.unspsc.org/UNv1111201").label("Digital Camera"))
            .contributor(Person::new("Shri. S. A. Khuba"))
            .content(
                Content::default()
                    .body("1) Pixels 12.3 million Effective . 12) Weight is Approx. 840 g")
                    .content_type("text/plain"),
            )
            .summary(Text::new(
                "This Atom Entry XML Doc publishes tech specifications of Nikon D300S Digital Camera".into(),
            )),
    );

    // Check
    assert_eq!(actual, expected);
}

// Verify we can parse MediaRSS extensions from youtube
#[test]
fn test_mediarss_youtube() {
    let test_data = test::fixture_as_string("atom/atom_mediarss_youtube_1.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap().id("");

    let expected = MediaObject::default()
        .title("Navigating with Quantum Entanglement")
        .content(
            MediaContent::new()
                .url("https://www.youtube.com/v/0A1ouV7iD8o?version=3")
                .content_type("application/x-shockwave-flash")
                .width(640)
                .height(390),
        )
        .thumbnail(MediaThumbnail::new(
            Image::new("https://i1.ytimg.com/vi/0A1ouV7iD8o/hqdefault.jpg".to_string())
                .width(480)
                .height(360),
        ))
        .description("Check Out Weathered on PBS Terra https://www.youtube.com/watch?v=znSN7ZFIaOg&ab_channel=PBSTerra")
        .community(MediaCommunity::new().star_rating(15020, 4.95, 1, 5).statistics(304321, 42));

    // Check the media object
    let entry = &actual.entries[0];
    let media_obj = &entry.media[0];
    assert_eq!(media_obj, &expected);
}

// Verify we can parse MediaRSS extensions from newscred (don't use the media:group)
#[test]
fn test_mediarss_newscred() {
    let test_data = test::fixture_as_string("atom/atom_mediarss_newscred_1.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap().id("");

    let expected = MediaObject::default()
        .title("media title")
        .description("media description")
        .text(MediaText::new(Text::new("media text".to_string())))
        .credit("media credit")
        .content(
            MediaContent::new()
                .url("https://www.example.com/Zz1hNjRiMWFjMzdhYWIzNTEwNjk2YjIzYjc5NWQxNWFlMA==/.jpeg")
                .content_type("image/jpeg")
                .width(2048)
                .height(1365),
        )
        .thumbnail(MediaThumbnail::new(
            Image::new("https://www.example.com/Zz1hNjRiMWFjMzdhYWIzNTEwNjk2YjIzYjc5NWQxNWFlMA==?width=75&height=75".to_string())
                .width(2048)
                .height(1365),
        ));

    // Check the media object
    let entry = &actual.entries[0];
    let media_obj = &entry.media[0];
    assert_eq!(media_obj, &expected);
}

#[test]
fn test_reddit() {
    let test_data = test::fixture_as_string("atom/atom_mediarss_reddit_1.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap().id("");

    let expected = MediaObject::default().thumbnail(MediaThumbnail::new(Image::new(
        "https://b.thumbs.redditmedia.com/_MXt-0n8VXQc-EQ7Q0vFioALFWFITAgVWu4Wf8dThhU.jpg".to_string(),
    )));

    let entry = &actual.entries[actual.entries.len() - 2];
    let media_obj = &entry.media[0];
    assert_eq!(media_obj, &expected);
}

// Handle text/html specified as a mime type on content
#[test]
fn test_scattered() {
    let test_data = test::fixture_as_string("atom/atom_scattered.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap().id("");
    assert!(actual.entries[0]
        .content
        .as_ref()
        .unwrap()
        .body
        .as_ref()
        .unwrap()
        .contains("there are no strings on me"));
}

// Handle xml:base attribute on content
#[test]
fn test_atom_content_xml_base() {
    let test_data = test::fixture_as_string("atom/atom_xml_base.xml");
    let actual = parser::parse(test_data.as_bytes()).unwrap();
    assert!(actual.entries[0].base.as_ref().unwrap().eq("https://numi.st/post/2022/travel-uke/"));
}
