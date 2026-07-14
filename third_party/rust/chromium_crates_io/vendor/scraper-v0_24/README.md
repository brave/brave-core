# scraper

[![crates.io](https://img.shields.io/crates/v/scraper?color=dark-green)][crate]
[![downloads](https://img.shields.io/crates/d/scraper)][crate]
[![test](https://github.com/causal-agent/scraper/actions/workflows/test.yml/badge.svg)][tests]

HTML parsing and querying with CSS selectors.

`scraper` is on [Crates.io][crate] and [GitHub][github].

[crate]: https://crates.io/crates/scraper
[github]: https://github.com/causal-agent/scraper
[tests]: https://github.com/causal-agent/scraper/actions/workflows/test.yml

Scraper provides an interface to Servo's `html5ever` and `selectors` crates, for browser-grade parsing and querying.

## Examples

### Parsing a document

```rust
use scraper::Html;

let html = r#"
    <!DOCTYPE html>
    <meta charset="utf-8">
    <title>Hello, world!</title>
    <h1 class="foo">Hello, <i>world!</i></h1>
"#;

let document = Html::parse_document(html);
```

### Parsing a fragment

```rust
use scraper::Html;
let fragment = Html::parse_fragment("<h1>Hello, <i>world!</i></h1>");
```

### Parsing a selector

```rust
use scraper::Selector;
let selector = Selector::parse("h1.foo").unwrap();
```

### Selecting elements

```rust
use scraper::{Html, Selector};

let html = r#"
    <ul>
        <li>Foo</li>
        <li>Bar</li>
        <li>Baz</li>
    </ul>
"#;

let fragment = Html::parse_fragment(html);
let selector = Selector::parse("li").unwrap();

for element in fragment.select(&selector) {
    assert_eq!("li", element.value().name());
}
```

### Selecting descendent elements

```rust
use scraper::{Html, Selector};

let html = r#"
    <ul>
        <li>Foo</li>
        <li>Bar</li>
        <li>Baz</li>
    </ul>
"#;

let fragment = Html::parse_fragment(html);
let ul_selector = Selector::parse("ul").unwrap();
let li_selector = Selector::parse("li").unwrap();

let ul = fragment.select(&ul_selector).next().unwrap();
for element in ul.select(&li_selector) {
    assert_eq!("li", element.value().name());
}
```

### Accessing element attributes

```rust
use scraper::{Html, Selector};

let fragment = Html::parse_fragment(r#"<input name="foo" value="bar">"#);
let selector = Selector::parse(r#"input[name="foo"]"#).unwrap();

let input = fragment.select(&selector).next().unwrap();
assert_eq!(Some("bar"), input.value().attr("value"));
```

### Serializing HTML and inner HTML

```rust
use scraper::{Html, Selector};

let fragment = Html::parse_fragment("<h1>Hello, <i>world!</i></h1>");
let selector = Selector::parse("h1").unwrap();

let h1 = fragment.select(&selector).next().unwrap();

assert_eq!("<h1>Hello, <i>world!</i></h1>", h1.html());
assert_eq!("Hello, <i>world!</i>", h1.inner_html());
```

### Accessing descendent text

```rust
use scraper::{Html, Selector};

let fragment = Html::parse_fragment("<h1>Hello, <i>world!</i></h1>");
let selector = Selector::parse("h1").unwrap();

let h1 = fragment.select(&selector).next().unwrap();
let text = h1.text().collect::<Vec<_>>();

assert_eq!(vec!["Hello, ", "world!"], text);
```

### Manipulating the DOM

```rust
use html5ever::tree_builder::TreeSink;
use scraper::{Html, Selector};

let html = "<html><body>hello<p class=\"hello\">REMOVE ME</p></body></html>";
let selector = Selector::parse(".hello").unwrap();
let mut document = Html::parse_document(html);
let node_ids: Vec<_> = document.select(&selector).map(|x| x.id()).collect();
for id in node_ids {
    document.remove_from_parent(&id);
}
assert_eq!(document.html(), "<html><head></head><body>hello</body></html>");
```

## Contributing

Please feel free to open pull requests. If you're planning on implementing
something big (i.e. not fixing a typo, a small bug fix, minor refactor, etc)
then please open an issue first.
