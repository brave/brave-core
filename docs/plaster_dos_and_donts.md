# 🩹 Plaster Dos and Don'ts

This document collects guidance for authoring plasters. The recommendations that
follow are not hard rules. These are observations gathered from practical use,
and offered as context for how 🩹 Plaster is best used.

## Write the match to convey the right intent

At the moment, 🩹 Plaster supports only regular expressions, and so the regex
itself is the vehicle through which the intent of a change is expressed. A
robust match describes _what_ is being changed and, where it can, offers some
indication of _why_, rather than concerning itself merely with the mechanics of
_how_ the substitution happens to take effect today.

- **Robustness follows from intent.** A regex that does not reflect the intent
  of a change will break on upstream edits unrelated to it. The fewer extraneous
  particulars a match depends on, the less likely it is to fail for reasons
  unrelated to the change itself. A match that breaks owing to unrelated reason
  to its original intent is, by definition, a fragile match.

- **Context conveys intent, to the appropriate degree.** "The last entry of the
  switch on `type` within `Foo()`" is a useful specification: function name,
  switch variable, position. Any further context dilutes the intent and makes
  the match fragile to unrelated changes. Inverselly, any less makes it too
  broad, and equally fragile. By the same token, `pattern = 'testonly = true'`
  in a `BUILD.gn` may be the right amount of context, even though it may appear
  minimal, as anything more would only weaken the match.

- **Intentional breakage is a safety feature.** A patch failing because its
  premise no longer holds (for example `count = 1` finding zero or several
  matches) can be a safety attribute of a particular match, since it prompts
  review. This is the opposite of fragility, which is failure unrelated to
  intent. Guarding against unforeseeable future changes is not straightforward,
  but it is one of the concerns when patching upstream code.

- **Intent informs recovery.** When a plaster breaks, the intent encoded in the
  regex tells the next reader what to look for: for example, the function name,
  the switch variable, the key being added, etc. By contrast, meager
  communication of intent may result in poor context when dealing with breakage,
  for example, "Match the last `return false;` in this file".

### Why a plaster is always better than an unmanaged patch

A `.patch` relies on its surrounding lines to apply its changes. When those
lines move, the patch requires `--3way` conflict resolution. Even the simplest
resolution involves a measure of judgement, and intervention. When enough lines
diverge between the diff and the tree, the patch may be considered broken
altogether, and the changes must then be replicated one-by-one.

Consider a one-line change as the following:

```diff
-  testonly = true
+  testonly = false
```

Now take the plaster equivalent:

```yaml
substitutions:
  - pattern: 'testonly = true'
    replace: 'testonly = false'
```

The former will break from time to time as adjacent lines shift. The latter
survives any reformatting around it, and `count = 1` (the default) flags any
further occurrence of the same pattern, which is enough safety for the case in
question.

### Why a plaster is always better than a `#define`

A `#define` rewrites every use of the identifier it targets across all sources
included after it, in a manner invisible to the reader and frequently
unintended. Consider, for instance, the following:

```cpp
#define GetFoo GetFoo_ChromiumImpl
```

The macro may appear benign, yet it rewrites any occurrence of `GetFoo`. Should
someone later introduce a new call to `GetFoo`, it too will be quietly rewritten
— with no warning at compile time, in code review, or during rebase. A plaster,
by contrast, addresses only the particular occurrence the match describes:

```yaml
substitutions:
  - re_pattern: '(int Foo::)GetFoo\b'
    replace: '\1GetFoo_ChromiumImpl'
```

Only the definition line of `Foo::GetFoo` is rewritten. Other uses of `GetFoo`
in the same file are left untouched, and in case a new overload is introduced,
`count = 1` (the default) raises the failure to a reviewer rather than silently
changing the meaning of the substitution.

A `#define` rewrites whatever it happens to match, and this extends to symbols
pulled in by `#include` uses. A plaster substitution can more adequately narrow
a specific match, and it only applies within a particular source.

It would be an understatement to call `#define` transformations difficult to
understand. A considerable share of rebase effort goes into unpicking failures
caused by them, and cases where `#define` does something other than what the
reader assumes are common causes for bugs. Wrong assumptions about what some
`#define` statements do are not uncommon, due to how hard it is to reason about
code relying on them.

## Don't use plaster for mere additions

A `chromium_src` shadow file can introduce any file-scope code (headers,
anonymous-namespace helpers, types, etc) before and after re-including the
upstream source. They serve us well as hosts for code that does not need to be
placed in a plaster to achieve the same effect.

Reserve plasters for those additions that cannot be made at file scope (e.g.
adding methods to a class).

Do:

```cpp
// chromium_src/foo/foo.cc
#include "brave/bar/bar.h"

namespace {
constexpr int kFooMax = 42;
}  // namespace

#include <foo/foo.cc>
```

Don't:

```yaml
# rewrite/foo/foo.cc.yaml
substitutions:
  - description: 'Add bar header'
    re_pattern: '(#include "foo/foo.h"\n)'
    replace: '\1#include "brave/bar/bar.h"\n'

  - description: 'Add kFooMax constant'
    re_pattern: '(constexpr int kBaz = 1;\n)'
    replace: '\1constexpr int kFooMax = 42;\n'
```

## Use `chromium_src` to host substantive C++

Where the C++ accompanying a plaster amounts to more than a trivial
substitution, place it in a `chromium_src` shadow file and keep the plaster
itself as small as possible. This improves the legibility of what is being done
to upstream code, and keeps the C++ where it can benefit from the tooling we
have in place:

- `clang-format`
- presubmit checks
- DEPS checks
- semgrep rules

Do:

```cpp
// chromium_src/foo/foo.cc
namespace {

void MaybeApplyBar(Foo* foo) {
  if (base::FeatureList::IsEnabled(bar::features::kBar)) {
    foo->ApplyBar();
  }
}

}  // namespace

#include <foo/foo.cc>
```

```yaml
# rewrite/foo/foo.cc.yaml
substitutions:
  - description: 'Call MaybeApplyBar in Foo::DoSomething'
    re_pattern: '(void Foo::DoSomething\(\) \{\n)'
    replace: '\1  MaybeApplyBar(this);\n'
```

Don't:

```yaml
# rewrite/foo/foo.cc.yaml
substitutions:
  - description: 'Apply bar in Foo::DoSomething'
    re_pattern: '(void Foo::DoSomething\(\) \{\n)'
    replace: |
      \1  if (base::FeatureList::IsEnabled(bar::features::kBar)) {
          ApplyBar();
        }
```

### Be pragmatic when deciding to use `chromium_src` or not

Where the substitution is trivial enough to be expressed plainly in the plaster
itself, making content in a shadow file an aspect only add indirection, and more
context. It is better to not make such convoluted uses of `chromium_src` that
pose no gain. When the plaster alone can express the change clearly, let it.

Do — let the plaster carry the change directly:

```yaml
# rewrite/chrome/browser/ssl/ask_before_http_dialog_controller.cc.yaml
substitutions:
  - description: 'Point the Learn More link at the Brave support article.'
    re_pattern: '(kLearnMoreLink[^=]*=\s*)(?:"[^"]*"\s*)+;'
    replace: '\1"https://support.brave.app/hc/en-us/articles/15513090104717";'
    count: 1
```

Don't — split a one-line substitution across two files:

```cpp
// chromium_src/chrome/browser/ssl/ask_before_http_dialog_controller.cc
namespace {

constexpr char kBraveLearnMoreLink[] =
    "https://support.brave.app/hc/en-us/articles/15513090104717";

}  // namespace

#include <chrome/browser/ssl/ask_before_http_dialog_controller.cc>
```

```yaml
# rewrite/chrome/browser/ssl/ask_before_http_dialog_controller.cc.yaml
substitutions:
  - description: 'Alias kLearnMoreLink to kBraveLearnMoreLink.'
    re_pattern: '^[^\n]*kLearnMoreLink\[\] =[^;]*;'
    replace: 'inline constexpr auto& kLearnMoreLink = kBraveLearnMoreLink;'
    re_flags: [MULTILINE]
```

## Whitespace in matches

`re_pattern` should, as a rule, avoid depending on particular whitespace. Prefer
the generic forms (`\s\*`, `\s+`, etc) so that a match is not made fragile by
irrelevant changes in spacing.

Literal whitespace is nonetheless acceptable, but only where _all_ of the
following hold:

- readability of the regular expression is materially improved
- the whitespace is a single space
- the file is covered by a formatting tool that enforces consistent spacing
- the chance of the whitespace ever becoming a newline is close to none

Pair flexible whitespace matching with `re_flags = ['DOTALL']` so that `.`
matches across newlines as well. This often makes the regex considerably easier
to read.

Do:

```yaml
# good - a reasonable mix of literal spaces and generic whitespace matching.
re_pattern: '(enum class RequestType \{.+?,)(\s+)kMaxValue = \w+'
re_flags: [DOTALL]
```

Don't:

```yaml
# bad - matching arbitrary newlines and whitespace throughout.
re_pattern: '(enum class RequestType\n\{\s*\n\s+.+?,\s*\n)\s+kMaxValue *=    \w+'
re_flags: [DOTALL]

# bad - avoiding literal single spaces at all costs, at the expense of
#       readability, and for no real gain.
re_pattern: '(enum\s*class\s*RequestType\s*\{.+?,)(\s+)kMaxValue\s*=\s*\w+'
re_flags: [DOTALL]
```

## Extending enums

When extending an enum with your own keys, anchor the insertion either at the
top or at the end, and avoid anchoring to a particular existing key wherever
possible. Any given key may be dropped or renamed at some future point. When
extending at the end, taking due care of `kMaxValue`, and trailing commas.

> [!WARNING]
>
> When an enum type has its underlying value serialised and converted to and
> from a persistent storage, the keys must remain stable. Relative entry
> positioning cannot be relied upon, and extensions must instead be handled case
> by case.

Do:

```yaml
substitutions:
  - description: 'Append Brave entries before kMaxValue'
    re_pattern: '(enum class RequestType \{.+?,)(\s+)kMaxValue = \w+'
    re_flags: [DOTALL]
    replace: |-
      \1
        kBraveOne,
        kBraveTwo,
        kMaxValue = kBraveTwo
```

**Don't**

```yaml
# bad - anchors on a specific key, that could very well be removed later on.
substitutions:
  - description: 'Append Brave entries after kStorageAccess'
    re_pattern: '(kStorageAccess,)'
    replace: |-
      \1
        kBraveOne,
        kBraveTwo,
```

## Extending switch statements

When adding new entries to a `switch`, first match the function in which the
switch resides, and then the switch block itself, in order to specify where the
insertion belongs.

Do:

```yaml
substitutions:
  # Good: matches `VerifyInit`, and then the switch block within it.
  - description: 'Add ECDSA_SHA384 handling to VerifyInit'
    re_pattern: '(VerifyInit\([^)]*\)\s*\{.*?\n\s*switch[^{]*\{.*?)(\n\s*\})'
    re_flags: [DOTALL]
    replace: |-
      \1
          case ECDSA_SHA384:
            pkey_type = EVP_PKEY_EC;
            digest = EVP_sha384();
            break;\2
```

**Don't**.

```yaml
# bad - anchors on a entry value, that could be renamed, reordered, or removed
# later on.
substitutions:
  - description: 'Add ECDSA_SHA384 case'
    pattern: 'case ECDSA_SHA256:'
    replace: |-
      case ECDSA_SHA256:
          case ECDSA_SHA384:
```

### Be mindful of `default:` when anchoring at the end

Consider whether the `switch` has a `default:` clause, or could acquire one,
when inserting at the end: the `default:` clause should be treated as the
boundary up to where insertions can occur.

Do:

```yaml
substitutions:
  - description: 'Add ECDSA_SHA384 handling to AlgorithmToString'
    re_pattern: '(AlgorithmToString\([^)]*\)\s*\{.*?\n\s*switch[^{]*\{.*?)(\n\s*default:)'
    re_flags: [DOTALL]
    replace: |-
      \1
          case ECDSA_SHA384:
            return "ECDSA";\2
```

### Grouping a new case with existing fall-through siblings

Where a new case must be grouped with existing fall-through siblings, anchor the
insertion on the desired outcome itself, typically a `return` statement, rather
than on any one of the case keys. Anchoring on a key produces a fragile plaster,
and risks an undesired substitution should that key later be removed or shuffled
into another group.

Do:

```yaml
substitutions:
  - description: 'Add ECDSA_SHA384 to the EC key case in AlgorithmToType'
    re_pattern:
      '(AlgorithmToType\([^)]*\).*?)(\n\s*return
      connectors_internals::mojom::KeyType::EC;)'
    re_flags: [DOTALL]
    replace: |-
      \1
          case crypto::SignatureVerifier::ECDSA_SHA384:\2
```

## Extend `gn` arrays with `+=` after instantiation, and scope the match

Extend keys such as `sources`, `deps`, or `public_deps` with `+=` immediately
after they are instantiated. Such keys are frequently passed along to other
arrays, and so must be extended before any such use occurs. Even where no such
use is present today, nothing prevents an upstream change from introducing one,
and it is wise to guard against that possibility.

Furthermore, avoid bundling extensions together. Each extension should sit close
to the declaration it extends. Bundling forces the insertion to a single point
at which every array involved is both valid to extend, and yet not read yet.
That constraint itself could be a source of future breakage.

Finally, constrain the match to a particular target.

Do:

```yaml
substitutions:
  - description: 'Append Brave sources to component("foo")'
    re_pattern: '(component\("foo"\).*?sources\s*=\s*\[.*?\])'
    re_flags: [DOTALL]
    replace: '\1\n  sources += brave_foo_sources'

  - description: 'Append Brave deps to component("foo")'
    re_pattern: '(component\("foo"\).*?\bdeps\s*=\s*\[.*?\])'
    re_flags: [DOTALL]
    replace: '\1\n  deps += brave_foo_deps'
```

Don't:

```yaml
substitutions:
  - description: 'Append Brave sources and deps to component("foo")'
    re_pattern: '(component\("foo"\) \{.*?)(\n\})'
    re_flags: [DOTALL]
    replace: |-
      \1
        sources += brave_foo_sources
        deps += brave_foo_deps\2
```

## Use `count = 0` only when you have a very good reason

Plaster defaults to `count = 1`, and that does not need to be spelled for every
substitution. Other values do need to be spelled out though. The value for
`count` must be chosen deliberately, since in most cases we _want_ the plaster
to break once the count no longer matches, so that the change may be reviewed.

There are however rare cases in which `count = 0` is sound: where the number of
occurrences is known to vary over time, and where every such occurrence is, by
its nature, safe to substitute. In those cases, accompany the `count` line with
a comment explaining _why_ it is safe to do so.

Do:

```yaml
substitutions:
  - re_pattern: '\s*<cr-unwanted-buttons[\s\S]+?</cr-unwanted-buttons>'
    replace: ''
    count: 0 # Any new uses will always be removed for this class.
```
