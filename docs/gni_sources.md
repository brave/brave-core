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

#### Usage of sources.gni

Do not add sources to `brave_chrome_browser_sources`; put new code in its own
target (see [MOD-002](best-practices/modularization.md#MOD-002)). Adding deps
through `sources.gni` is generally ok.

Using `sources.gni` to add a very small number of sources to another upstream
target may be appropriate, but consider the approaches below first. Using it to
add dependencies and other non-source configuration to upstream targets is
generally ok.

#### Methods to avoid circular dependencies

Whenever possible try to break circular dependencies see [Recipes for Breaking
Chrome Dependencies] and [Dependency Inversion] for examples.

[Recipes for Breaking Chrome Dependencies]:
  https://www.chromium.org/developers/design-documents/cookbook/#recipes-for-breaking-chrome-dependencies
[Dependency Inversion]:
  https://www.chromium.org/developers/design-documents/cookbook/#dependency-inversion

An interface/impl pattern can also often be used where header files and possibly
some cc files are included in the direct dependency and the code that would
cause the circular dependency is included in a higher level target like
`//brave/browser:core` to ensure that the implementation code is always linked
into the final output. See [tabs:tabs_public], [tabs:impl] and [//chrome/browser
impl dependency].

[tabs:tabs_public]:
  https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/BUILD.gn;l=12;drc=ad947f73e5449afe74659d107eb34e2521bee100
[tabs:impl]:
  https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/BUILD.gn;l=300;drc=ad947f73e5449afe74659d107eb34e2521bee100
[//chrome/browser impl dependency]:
  https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/BUILD.gn;l=4378;drc=265bc11af3dc764e0f59f93016aa350bbfa5f814

The chromium ios code is also a good model for separating out dependencies and
sometimes makes use of interface/implementation patterns.

Another technique to avoid circular dependencies is to use a template so the
subclass does not need a dependency on the base class.

brave_class.h

```cpp
template <typename ChromeClass>
class BraveClass : public ChromeClass {
  ...
}
```

The chrome target that we override will need a dependency on the brave target,
but there is no circular dependency some_chromium_source.cc

```cpp
  chrome_class_ = std::make_unique<ChromeClass>();
```

chromium_src/some_chromium_source.cc

```cpp
#define ChromeClass BraveClass<ChromeClass>()
```

#### Circular dependencies

`brave_chrome_browser_allow_circular_includes_from` (on `//brave/browser:core`),
`brave_ui_allow_circular_includes_from` and
`brave_chrome_browser_ui_allow_circular_includes_from` (on
`//brave/browser/ui:ui` and `//chrome/browser/ui:ui`) list the targets allowed
to include headers from those monoliths. They exist only while the monoliths are
split up. Do not add entries
([MOD-001](best-practices/modularization.md#MOD-001)); use the
`/remove-circular-includes` skill to remove them. Progress is tracked in
[brave-browser#59199](https://github.com/brave/brave-browser/issues/59199). See
[this brave-core PR] for an example of converting from `sources.gni`.

[this brave-core PR]: https://github.com/brave/brave-core/pull/25892/files

Do not use `check_includes = false` to suppress errors about circular includes.
