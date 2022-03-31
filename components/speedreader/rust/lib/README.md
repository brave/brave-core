# Brave SpeedReader

At a high level, SpeedReader:

* Distills text-focused document content from a suitable HTML
* Works on HTML documents before rendering them
* Generates HTML output with no external styling or scripting
* Content styled with Brave-designed themes

## Structure

SpeedReader decides whether a page is suitable for distillation,
or 'readable', based on a `classifier` that analyses HTML content
of a page and applies a pre-trained model to decide whether the page
is article-style, text-oriented, and so on.

Any page that is readable should then be passed to an instance of
`SpeedReader`. Internally it uses heuristics to determine whether
to apply streaming, rule-based rewriting (using [lol_html](https://github.com/cloudflare/lol-html)), or an approach derived from the [readability](https://github.com/kumabook/readability)
crate, itself loosely based on [Mozilla's Readability.js](https://github.com/mozilla/readability).

## Running the tests

You need to clone the speedreader test data to the
`components/speedreader/rust/lib` directory.

```
$ git clone https://github.com/brave-experiments/speedreader-test-data data
$ cargo test
```
