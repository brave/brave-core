### sources.gni

#### Background

The use of sources.gni was initially added to help deal with the issue of circular dependencies. We
often subclass upstream code like `BraveContentBrowserClient` -> `ChromeContentBrowserClient`, but in
`//brave/browser` instead of `//chrome/browser`. This created a circular dependency because:

- `BraveContentBrowserClient` depends on `//chrome/brave`.
- `//chrome/brave` depends on `//chrome/brave` because it now was to instantiate
  `BraveContentBrowserClient` instead of `ChromeContentBrowserClient`.

This became a problem when we started running `gn check` and `checkdeps` so worked around it by using
`sources.gni` to add `sources += brave_chrome_browser_sources` so both `BraveContentBrowserClient`
and `ChromeContentBrowserClient` were in the same target. This is mainly an issue for
`//chrome/browser` and `//chrome/browser/ui` and upstream is actively working to break these up so
we need to make sure we are not adding to the problem on our side.

#### Usage of sources.gni

Use of sources.gni to include sources in `//chrome/browser` and `//chrome/browser/ui` should be
avoided, see [Circular dependencies](#circular-dependencies). Adding deps through sources.gni is
generally ok. Use of sources.gni to include sources in other targets can be used if the there is no
reasonable way to avoid it using the options below.

This does not mean that you cannot ever use sources.gni. For instance it may be appropriate when
adding a very small number of sources to an existing upstream target, but please consider other
approaches below first. Using sources.gni to add dependencies and other non-source configuration to
upstream targets is generally ok.

#### Methods to avoid circular dependencies

Whenever possible try to break circular dependencies see [Recipes for Breaking Chrome Dependencies]
and [Dependency Inversion] for examples.

[Recipes for Breaking Chrome Dependencies]: https://www.chromium.org/developers/design-documents/cookbook/#recipes-for-breaking-chrome-dependencies
[Dependency Inversion]: https://www.chromium.org/developers/design-documents/cookbook/#dependency-inversion

An interface/impl pattern can also often be used where header files and possibly some cc files are
included in the direct dependency and the code that would cause the circular dependency is included
in a higher level target like `//brave/browser` to ensure that the implementation code is always
linked into the final output. See [tabs:tabs_public], [tabs:impl] and [//chrome/browser impl
dependency].

[tabs:tabs_public]: https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/BUILD.gn;l=12;drc=ad947f73e5449afe74659d107eb34e2521bee100
[tabs:impl]: https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/BUILD.gn;l=300;drc=ad947f73e5449afe74659d107eb34e2521bee100
[//chrome/browser impl dependency]: https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/BUILD.gn;l=4378;drc=265bc11af3dc764e0f59f93016aa350bbfa5f814

The chromium ios code is also a good model for separating out dependencies and sometimes makes use
of interface/implementation patterns.

Another technique to avoid circular dependencies is to use a template so the subclass does not need
a dependency on the base class.

brave_class.h
```cpp
template <typename ChromeClass>
class BraveClass : public ChromeClass {
  ...
}
```

The chrome target that we override will need a dependency on the brave target, but there is no
circular dependency some_chromium_source.cc
```cpp
  chrome_class_ = std::make_unique<ChromeClass>();
```

chromium_src/some_chromium_source.cc
```cpp
#define ChromeClass BraveClass<ChromeClass>()
```

#### Circular dependencies

Circular dependencies can sometimes (temporarily) use `brave_chrome_browser_allow_circular_includes_from`
and/or `brave_chrome_browser_ui_allow_circular_includes_from` if necessary to split sources up into
smaller targets so they can be more easily resolved down the road. This is the technique we should use
for `//chrome/browser` and `//chrome/browser/ui` if the circular dependencies cannot be removed
through the methods above. It may be appropriate in other cases, check in slack if you are unsure. See
[`//chrome/browser`] and [`//chrome/browser/ui`] for examples. Also see [this brave-core PR] for an
example of converting from sources.gni.

[`//chrome/browser`]: https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/BUILD.gn;l=3524;drc=80bd94ca218b30eb74a107ea54b469d79b25f16d
[`//chrome/browser/ui`]: https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/BUILD.gn;l=5752;drc=fe55ddc4724a631b7e1752ac29310cfb3de4a8c5
[this brave-core PR]: https://github.com/brave/brave-core/pull/25892/files

Do not use `check_includes = false` to suppress errors about circular includes.
