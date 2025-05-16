use crate::{Html, Selector};

#[test]
fn tag_with_newline() {
    let selector = Selector::parse("a").unwrap();

    let document = Html::parse_fragment(
        r#"
        <a
                            href="https://github.com/causal-agent/scraper">

                            </a>
        "#,
    );

    let mut iter = document.select(&selector);
    let a = iter.next().unwrap();
    assert_eq!(
        a.value().attr("href"),
        Some("https://github.com/causal-agent/scraper")
    );
}

#[test]
fn has_selector() {
    let document = Html::parse_fragment(
        r#"
        <div>
            <div id="foo">
                Hi There!
            </div>
        </div>
        <ul>
            <li>first</li>
            <li>second</li>
            <li>third</li>
        </ul>
        "#,
    );

    let selector = Selector::parse("div:has(div#foo) + ul > li:nth-child(2)").unwrap();

    let mut iter = document.select(&selector);
    let li = iter.next().unwrap();
    assert_eq!(li.inner_html(), "second");
}
