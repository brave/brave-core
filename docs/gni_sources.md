### sources.gni

#### Background

`sources.gni` was added to work around circular dependencies. We often subclass
upstream code, like `BraveContentBrowserClient` -> `ChromeContentBrowserClient`,
in `//brave/browser`, and a `chromium_src` override makes upstream instantiate
the Brave class. The Brave class depends on upstream, and upstream depends on
the Brave class.

To keep `gn check` passing, these Brave sources are collected in
`brave_chrome_browser_sources` and compiled into `//brave/browser:core`.
`//chrome/browser:browser` is a sourceless aggregator that depends on both
`//chrome/browser:core` (upstream's sources) and `//brave/browser:core`, the
latter added by `rewrite/chrome/browser/BUILD.gn.yaml`. `//brave/browser:core`
is therefore Brave's monolith, the counterpart of upstream's
`//chrome/browser:core` and `//chrome/browser/ui:ui`. Upstream is breaking its
monoliths up, and we must not add to ours.

#### Rules

What may go in `sources.gni`, and how to break the circular dependencies it
works around, are covered in
[Modularization and Circular Dependencies](best-practices/modularization.md).
