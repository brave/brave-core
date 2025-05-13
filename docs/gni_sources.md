### sources.gni

#### Background
The use of sources.gni was initially added to help deal with the issue of circular dependencies. We often subclass upstream code like `BraveContentBrowserClient` -> `ChromeContentBrowserClient`, but in a separate target there would be a circular dependency because `BraveContentBrowserClient` depends on `//chrome/brave` and `//chrome/brave` would have to depend on the target that contains `BraveContentBrowserClient`. This actually originated from gyp which had very large targets that upstream has been slowly working to split up. This is mainly an issue for `//chrome/browser` and `//chrome/browser/ui` and as upstream is working to break these targets up, we need to make sure we are not adding to the problem on our side.

#### Usage of sources.gni

Use of sources.gni to include sources in `//chrome/browser` and `//chrome/browser/ui` should be avoided. Adding deps through sources.gni is generally ok. Use of sources.gni to include sources in other targets can be used if the there is no reasonable way to avoid it using the options below.

This does not mean that you cannot ever use sources.gni. For instance it may be appropriate when adding a very small number of sources to an existing upstream target, but please consider other approaches below first. Using sources.gni to add dependencies and other non-source configuration to upstream targets is generally ok.

#### Methods to avoid circular dependencies

Whenever possible try to break circular dependencies see [Recipes for Breaking Chrome Dependencies](https://www.chromium.org/developers/design-documents/cookbook/#recipes-for-breaking-chrome-dependencies) and [https://www.chromium.org/developers/design-documents/cookbook/#dependency-inversion](https://www.chromium.org/developers/design-documents/cookbook/#dependency-inversion) for examples.

An interface/impl pattern can also often be used where header files and possibly some cc files are included in the direct dependency and the code that would cause the circular dependency is included in a higher level target like `//brave/browser` to ensure that the implementation code is always linked into the final output. See [tabs:tabs_public](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/BUILD.gn;l=12;drc=ad947f73e5449afe74659d107eb34e2521bee100) and [tabs:impl](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/BUILD.gn;l=300;drc=ad947f73e5449afe74659d107eb34e2521bee100) and [//chrome/browser impl dependency](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/BUILD.gn;l=4378;drc=265bc11af3dc764e0f59f93016aa350bbfa5f814). The chromium ios code is also a good model for separating out dependencies and sometimes makes use of interface/implementation patterns.

Another technique to avoid circular dependencies is to use a template so the subclass does not need a dependency on the base class.
```cpp
template <typename ChromeClass>
class BraveClass : public ChromeClass {
  ...
}
```

The chrome target that we override will need a dependency on the brave target, but there is no circular dependency
```cpp
#define ChromeClass BraveClass<ChromeClass>()
```

Circular dependencies can sometimes (temporarily) use `allow_circular_includes_from` to split sources up into smaller targets so they can be more easily resolved down the road. This is the technique we should use for `//chrome/browser` and `//chrome/browserui` if the circular dependencies cannot be removed through the methods above. It may be appropriate in other cases, check in slack if you are unsure. See [`//chrome/browser/ui`](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/BUILD.gn;l=3524;drc=80bd94ca218b30eb74a107ea54b469d79b25f16d) and [`//chrome/browser/ui`](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/BUILD.gn;l=5752;drc=fe55ddc4724a631b7e1752ac29310cfb3de4a8c5) for examples.

Do not use `check_includes = false` to suppress errors about circular includes.
