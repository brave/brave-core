# feed-rs

[![Build Status](https://travis-ci.org/feed-rs/feed-rs.svg?branch=master)](https://travis-ci.org/feed-rs/feed-rs.svg?branch=master)
[![Crates.io Status](https://img.shields.io/crates/v/feed-rs.svg)](https://crates.io/crates/feed-rs)

Library for parsing various forms of feeds such as Atom, RSS and JSON Feed.
It also supports extensions such as iTunes, Dublin Core and Media RSS.

[Documentation](https://docs.rs/feed-rs/)

## Usage

Add the dependency to your `Cargo.toml`.

```toml
[dependencies]
feed-rs = "1.5.3"
```

## Reading

A feed can be parsed from any object that implements the `Read` trait.

```rust
use feed_rs::parser;
let xml = r#"
<feed>
   <title type="text">sample feed</title>
   <updated>2005-07-31T12:29:29Z</updated>
   <id>feed1</id>
   <entry>
       <title>sample entry</title>
       <id>entry1</id>
   </entry>
</feed>
"#;
let feed = parser::parse(xml.as_bytes()).unwrap();
```

The parser will automatically detect XML vs. JSON so parsing JSON Feed content works the same way.

```rust
use feed_rs::parser;
let json = r#"
{
  "version": "https://jsonfeed.org/version/1",
  "title": "JSON Feed",
  "description": "JSON Feed is a pragmatic syndication format for blogs, microblogs, and other time-based content.",
  "home_page_url": "https://jsonfeed.org/",
  "feed_url": "https://jsonfeed.org/feed.json",
  "author": {
    "name": "Brent Simmons and Manton Reece",
    "url": "https://jsonfeed.org/"
  },
  "items": [
    {
      "title": "Announcing JSON Feed",
      "date_published": "2017-05-17T08:02:12-07:00",
      "id": "https://jsonfeed.org/2017/05/17/announcing_json_feed",
      "url": "https://jsonfeed.org/2017/05/17/announcing_json_feed",
      "content_html": "<p>We — Manton Reece and Brent Simmons — have noticed that JSON...</p>"
    }
  ]
}"#;
let feed = parser::parse(json.as_bytes()).unwrap();
```

## License

MIT license ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

## Contribution

Any contribution intentionally submitted for inclusion in the work by you, 
shall be licensed as above, without any additional terms or conditions.
