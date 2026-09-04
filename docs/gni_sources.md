### sources.gni

#### Background

[sources.gni](https://sourcegraph.com/r/github.com/brave/brave-core/-/blob/browser/sources.gni)
was originally added to work around circular dependencies. We often subclass
upstream code, for example `BraveContentBrowserClient` ->
`ChromeContentBrowserClient`, but we keep the subclass in `//brave/browser`
instead of `//chrome/browser`. That creates two dependency edges pointing in
opposite directions.

**Edge 1 - inheritance.** You cannot derive from an incomplete type, so the
subclass needs the full upstream header and the base class code, which means the
Brave target needs a dep on the upstream target. From
`//brave/browser/brave_content_browser_client.h`:

```cpp
#include "chrome/browser/chrome_content_browser_client.h"

class BraveContentBrowserClient : public ChromeContentBrowserClient {
  ...
};
```

**Edge 2 - instantiation.** A subclass that nobody constructs never runs, and
upstream is what constructs it. Written naively, an upstream file has to name
our type, so the upstream target needs a dep on `//brave/browser`:

```cpp
// somewhere in //chrome/browser
content_browser_client_ = std::make_unique<BraveContentBrowserClient>();
```

Together the two edges are a cycle: `//brave/browser` -> `//chrome/browser` ->
`//brave/browser`. GN rejects a `deps` cycle outright, and `gn check` and
`checkdeps` reject the includes that imply it. The edges exist for different
reasons - edge 1 is about inheriting from something, edge 2 is about being used
by something - so neither disappears on its own.

The original workaround broke neither edge. It removed the second _target_, by
adding `sources += brave_chrome_browser_sources` to `//chrome/browser` so that
both classes ended up in one target, and a target may always use its own files.

This is mainly an issue for `//chrome/browser` and `//chrome/browser/ui`, and
upstream is actively working to break these up, so we need to make sure we are
not adding to the problem on our side. Three reasons it matters more for these
two targets than elsewhere:

- Nearly everything depends on them and they depend on nearly everything, so
  almost any edge back into a Brave target closes a loop through something.
  Smaller targets let us depend only on the piece we actually subclass.
- `gn check` works per target, so a file compiled inside a monolith inherits
  that target's visibility of almost the whole tree. We do not satisfy the
  layering rules that way, we opt out of them.
- We inject sources by patching upstream's `BUILD.gn`. Those are among the most
  frequently changed files in the tree, so the patches conflict on rebases, and
  every injected file has to be relocated as upstream splits the target up.

Upstream now guards against this directly: `//chrome/browser` ends with
`sources = []` and a
[comment](https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/BUILD.gn;l=1972-1976)
telling you not to add sources there. Since GN evaluates sequentially, an
earlier `sources += [...]` in that target is silently discarded.

#### How this looks today

`brave_chrome_browser_sources` is no longer added to `//chrome/browser`. It is
the `sources` of the `//brave/browser:core` `source_set` in
[//brave/browser/BUILD.gn](https://sourcegraph.com/r/github.com/brave/brave-core/-/blob/browser/BUILD.gn?L75),
and upstream picks that target up through a one line patch in
[patches/chrome-browser-BUILD.gn.patch](https://sourcegraph.com/r/github.com/brave/brave-core/-/blob/patches/chrome-browser-BUILD.gn.patch?L9).

The detail that makes this work is that `//chrome/browser` and
`//chrome/browser:core` are two **different** targets that happen to live in the
same `BUILD.gn`. `//chrome/browser` is GN shorthand for
`//chrome/browser:browser`; it has `sources = []` and is a pure aggregator.
`//chrome/browser:core` is where `ChromeContentBrowserClient` and most of
upstream's browser code actually lives.

```
//chrome/browser  (== //chrome/browser:browser)
  sources = []  -- pure aggregator, no code of its own
  |
  |-- public_deps --> //chrome/browser:core
  |                     ChromeContentBrowserClient lives here
  |
  '-- public_deps --> //brave/browser:core        <-- added by our patch
                        BraveContentBrowserClient lives here
                        deps --> //chrome/browser:core
```

There is no cycle because no edge leaves `//chrome/browser:core` and arrives
back at `//brave/browser:core` or at the aggregator. Two rules keep it that way:

- `brave_chrome_browser_deps` must never contain `"//chrome/browser"`. Depend on
  `"//chrome/browser:core"`, or on a fine grained subtarget such as
  `"//chrome/browser/autofill"`, instead. Depending on the aggregator closes the
  loop immediately.
- `//brave/browser:core` declares `visibility = [ "//chrome/browser" ]`, which
  in GN names only the `:browser` target. Nothing else, `//chrome/browser:core`
  included, is allowed to depend on it, so the cycle cannot be reintroduced by
  adding a dep in the wrong place.

`BraveContentBrowserClient` is also no longer instantiated from
`//chrome/browser`, see
[Instantiate from a higher level target](#instantiate-from-a-higher-level-target).

Everything below still applies to anything you add to
`brave_chrome_browser_sources`.

#### Usage of sources.gni

Use of sources.gni to include sources in `//chrome/browser` and
`//chrome/browser/ui` should be avoided, see
[Circular dependencies](#circular-dependencies). Adding deps through sources.gni
is generally ok. Use of sources.gni to include sources in other targets can be
used if the there is no reasonable way to avoid it using the options below.

This does not mean that you cannot ever use sources.gni. For instance it may be
appropriate when adding a very small number of sources to an existing upstream
target, but please consider other approaches below first. Using sources.gni to
add dependencies and other non-source configuration to upstream targets is
generally ok.

Adding deps for a `chromium_src` override is a separate, always acceptable use.
A `chromium_src` override is compiled as part of the upstream target that owns
the file it replaces, so a new dependency has to be added to that upstream
target. The convention is a deps only `sources.gni` next to the override, for
example `//brave/chromium_src/chrome/browser/ui/autofill/sources.gni`, imported
by a patched line in the corresponding upstream `BUILD.gn`. Note that `gn check`
does not run for `chromium_src`, so nothing will flag a missing dep for you.

A small, legitimate example of adding sources is `BraveContentClient`. It
subclasses `ChromeContentClient`, so `//chrome/common/BUILD.gn` gets
`sources += brave_chrome_common_sources` from `//brave/common/sources.gni`,
which puts the subclass in the same target as its base. The type is then swapped
in `//brave/chromium_src/chrome/app/chrome_main_delegate.h` with
`#define ChromeContentClient BraveContentClient`.

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
`//brave/browser` to ensure that the implementation code is always linked into
the final output. See [tabs:tabs_public], [tabs:impl] and [//chrome/browser impl
dependency].

[tabs:tabs_public]:
  https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/BUILD.gn;l=12;drc=ad947f73e5449afe74659d107eb34e2521bee100
[tabs:impl]:
  https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/tabs/BUILD.gn;l=300;drc=ad947f73e5449afe74659d107eb34e2521bee100
[//chrome/browser impl dependency]:
  https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/BUILD.gn;l=4378;drc=265bc11af3dc764e0f59f93016aa350bbfa5f814

The chromium ios code is also a good model for separating out dependencies and
sometimes makes use of interface/implementation patterns.

##### Use a template to remove the inheritance edge

A template removes edge 1, because the base class becomes a type parameter and
the subclass header includes no upstream header at all.

brave_class.h

```cpp
template <typename ChromeClass>
class BraveClass : public ChromeClass {
  ...
};
```

The chrome target that we override will need a dependency on the brave target,
but with edge 1 gone there is only one direction left and therefore no circular
dependency. The template is bound at the point of use by a `chromium_src`
override.

some_chromium_source.cc

```cpp
  chrome_class_ = std::make_unique<ChromeClass>();
```

chromium_src/some_chromium_source.cc

```cpp
#define ChromeClass BraveClass<ChromeClass>
```

##### Instantiate from a higher level target

Edge 2 disappears if the code that constructs the object lives in a target above
both, because then that target depends on both and neither depends on the other.
This is how the content browser client works now:
`BraveMainDelegate::CreateContentBrowserClient()` in
`//brave/app/brave_main_delegate.cc` creates it, and that code is compiled into
the chrome executable rather than into `//chrome/browser`.

```cpp
content::ContentBrowserClient* BraveMainDelegate::CreateContentBrowserClient() {
  if (chrome_content_browser_client_ == nullptr) {
    chrome_content_browser_client_ =
        std::make_unique<BraveContentBrowserClient>();
  }
  return chrome_content_browser_client_.get();
}
```

`CreateContentBrowserClient()` is already virtual upstream, so the client itself
needs no preprocessor work, only an override. Constructing the delegate is not
virtual, so that one is handled with a macro, below.

##### Substitute the type in a chromium_src override

A `chromium_src` override replaces an upstream file inside the upstream target,
so a macro there can change which type upstream constructs without adding
anything to the GN graph. `//brave/chromium_src/chrome/app/chrome_main.cc`:

```cpp
#include "brave/app/brave_main_delegate.h"

#define ChromeMainDelegate BraveMainDelegate
#include <chrome/app/chrome_main.cc>
#undef ChromeMainDelegate
```

The angle bracket include is required. `brave/chromium_src` is added to the
quoted include path only, so `<...>` reaches the real upstream file instead of
recursing into the override. See
[patching_and_chromium_src.md](patching_and_chromium_src.md) for the full
mechanism. Also note, we now rarely have the need to modify/patch the upstream
files manually, we do it via Plaster. So, it's recommended to read these plaster
guides
[plaster_dos_and_donts.md](https://github.com/brave/brave-core/blob/master/docs/plaster_dos_and_donts.md)
and
[plaster.md](https://github.com/brave/brave-core/blob/master/docs/plaster.md)
before.

When the override also needs the subclass implementation in the same translation
unit it can include the `.cc` directly, as
`//brave/chromium_src/chrome/app/chrome_main_delegate.cc` does:

```cpp
#include "brave/app/brave_main_delegate.cc"  // IWYU pragma: export

#include <chrome/app/chrome_main_delegate.cc>  // IWYU pragma: export
```

That is the same "put them in one target" trick as `sources.gni`, done with the
preprocessor instead of GN, and it has the same drawback of growing the upstream
target. Prefer the options above.

#### Circular dependencies

Circular dependencies can sometimes (temporarily) use
`brave_chrome_browser_allow_circular_includes_from` and/or
`brave_chrome_browser_ui_allow_circular_includes_from` if necessary to split
sources up into smaller targets so they can be more easily resolved down the
road. This is the technique we should use for `//chrome/browser` and
`//chrome/browser/ui` if the circular dependencies cannot be removed through the
methods above. It may be appropriate in other cases, check in slack if you are
unsure. See [`//chrome/browser`] and [`//chrome/browser/ui`] for examples. Also
see [this brave-core PR] for an example of converting from sources.gni.

[`//chrome/browser`]:
  https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/BUILD.gn;l=3524;drc=80bd94ca218b30eb74a107ea54b469d79b25f16d
[`//chrome/browser/ui`]:
  https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/BUILD.gn;l=5752;drc=fe55ddc4724a631b7e1752ac29310cfb3de4a8c5
[this brave-core PR]: https://github.com/brave/brave-core/pull/25892/files

Do not use `check_includes = false` to suppress errors about circular includes.
