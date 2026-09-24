# 🩹 Plaster Dos and Don'ts

This document collects guidance for authoring plasters. The recommendations that
follow are not hard rules. These are observations gathered from practical use,
and offered as context for how 🩹 Plaster is best used.

## Write the match to convey the right intent

Where a regex is the right tool, the regex itself is the vehicle through which
the intent of a change is expressed. A robust match describes _what_ is being
changed and, where it can, offers some indication of _why_, rather than
concerning itself merely with the mechanics of _how_ the substitution happens to
take effect today.

- **Robustness follows from intent.** A regex that does not reflect the intent
  of a change will break on upstream edits unrelated to it. The fewer extraneous
  particulars a match depends on, the less likely it is to fail for reasons
  unrelated to the change itself. A match that breaks owing to unrelated reason
  to its original intent is, by definition, a fragile match.

- **Context conveys intent, to the appropriate degree.** "The last entry of the
  switch on `type` within `Foo()`" is a useful specification: function name,
  switch variable, position. Any further context dilutes the intent and makes
  the match fragile to unrelated changes. Inversely, any less makes it too
  broad, and equally fragile. By the same token, `pattern: 'testonly = true'` in
  a `BUILD.gn` may be the right amount of context, even though it may appear
  minimal, as anything more would only weaken the match.

- **Intentional breakage is a safety feature.** A patch failing because its
  premise no longer holds (for example `count: 1` finding zero or several
  matches) can be a safety attribute of a particular match, since it prompts
  review. This is the opposite of fragility, which is failure unrelated to
  intent. Guarding against unforeseeable future changes is not straightforward,
  but it is one of the concerns when patching upstream code.

- **Intent informs recovery.** When a plaster breaks, the intent encoded in the
  regex tells the next reader what to look for: for example, the function name,
  the switch variable, the key being added, etc. By contrast, meagre
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
  - regex:
      pattern: 'testonly = true'
      replace: 'testonly = false'
```

The former will break from time to time as adjacent lines shift. The latter
survives any reformatting around it, and `count: 1` (the default) flags any
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
  - regex:
      re_pattern: '(int Foo::)GetFoo\b'
      replace: '\1GetFoo_ChromiumImpl'
```

Only the definition line of `Foo::GetFoo` is rewritten. Other uses of `GetFoo`
in the same file are left untouched, and in case a new overload is introduced,
`count: 1` (the default) raises the failure to a reviewer rather than silently
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

## Write the `description:` to document what and why

A `description:` is read by a reviewer at the time of the change, and later by
whoever is staring at a failed match during a rebase, serving as an important
clue as to what the substitution was for. A good description does two things.
First, a short subject line conveys, at a glance, the high-level view of what
the substitution does. Second, where the _why_ is not obvious from the _what_, a
blank line followed by one paragraph or more that elaborates on the reasoning
behind the change.

Do:

```yaml
substitutions:
  - description: |-
      Force ranker querying off.

      Brave never downloads or uses a translate ranker model, regardless of
      the upstream field trial state, so RankerModelLoaderImpl must never be
      reached.
    ...
```

Don't:

```yaml
substitutions:
  - description: |-
      Force ranker querying off: Brave never downloads or uses a translate
      ranker model, regardless of the upstream field trial state.
    ...
```

A subject line on its own is fine where there is genuinely nothing more to say.
Do not pollute the descript with long paragraphs explaining **how** a certain
substitution ended up on its final state.

## Prefer a rewriter over a regex

AST rewriters have some conceptual understanding of the code syntax, and
therefore are better suited for use for the transformations they were designed
for. They also convey intent of their own clearly, stating _what_ is being done,
and leaving the _how_ to the tool.

Do:

```yaml
substitutions:
  - description: 'Let the Brave subclass reach private members.'
    add_friend:
      class_name: MultiContentsView
      friend_type: class BraveMultiContentsView
```

Don't:

```yaml
substitutions:
  - description: 'Let the Brave subclass reach private members.'
    regex:
      re_pattern: '( private:)'
      replace: '\1\n  friend class BraveMultiContentsView;'
```

## Don't rename a function to `_ChromiumImpl`

Inserting a customisation on an upstream function has customarily meant
splitting it in two: renaming the definition in the `.cc`, adding a second
declaration to the `.h`, and then adding the definition in the separate
`chromium_src` file.

`preempt_function_impl` and `after_function_impl` remove the need for the split
altogether. They insert code at the top or at the bottom of the upstream body
itself, so the change is visible in the generated patch, at the function it
concerns, and no declaration anywhere is disturbed.

Do — a single substitution, in the file that holds the definition:

```yaml
# rewrite/foo/foo.cc.yaml
substitutions:
  - description: 'Leave DoSomething alone when Brave has the feature off.'
    preempt_function_impl:
      function_name: Foo::DoSomething
      code: |-
        if (!IsBarEnabled(prefs())) {
          return;
        }
```

Don't — renaming the function for wrapping it with custom code:

```yaml
# rewrite/foo/foo.h.yaml — the duplicate declaration the rename now requires
substitutions:
  - description: 'Declare DoSomething_ChromiumImpl'
    regex:
      re_pattern: '(void DoSomething\(\);)'
      replace: '\1\n  void DoSomething_ChromiumImpl();'
```

```cpp
// chromium_src/foo/foo.cc — the rename, and the replacement
#define DoSomething DoSomething_ChromiumImpl
#include <foo/foo.cc>
#undef DoSomething

void Foo::DoSomething() {
  if (!IsBarEnabled(prefs())) {
    return;
  }
  DoSomething_ChromiumImpl();
}
```

## Prefer inserting in place to subclassing, where appropriate

Subclassing carries a comparable cost. Making a method virtual, dropping
`final`, befriending the Brave class and then arranging for the subclass to be
constructed in place of the original is a good deal of apparatus, and where its
whole purpose is to bracket one upstream method, the same thing can be said in
place instead.

Do — state the one thing we add, where the upstream body ends:

```yaml
# rewrite/chrome/browser/download/bubble/download_display_controller.cc.yaml
substitutions:
  - description: 'Show the toolbar button for in-progress Brave downloads.'
    after_function_impl:
      function_name: DownloadDisplayController::UpdateToolbarButtonState
      code: |-
        if (ShouldShowToolbarButtonForInProgressDownload(
                info, display_, bubble_controller_, browser_)) {
          ShowToolbarButton();
        }
```

Don't — open up the class in order to override a single method:

```yaml
# rewrite/chrome/browser/download/bubble/download_display_controller.h.yaml
substitutions:
  - drop_final:
      class_name: DownloadDisplayController

  - make_virtual:
      class_name: DownloadDisplayController
      method_name: UpdateToolbarButtonState

  - add_friend:
      class_name: DownloadDisplayController
      friend_type: class BraveDownloadDisplayController
```

```cpp
// brave/browser/download/bubble/brave_download_display_controller.cc
void BraveDownloadDisplayController::UpdateToolbarButtonState(...) {
  DownloadDisplayController::UpdateToolbarButtonState(...);
  // ... the one thing Brave actually wanted ...
}
```

### When subclassing is appropriate

Unlike the `_ChromiumImpl` rename, subclassing can be the more attractive
option, and there are rewriters to assist with that (e.g. `drop_final`,
`make_virtual`, `add_friend`). The following may help in deciding when to use
subclassing:

- **The customisation carries state of its own.** Members can be injected into
  an upstream class with `add_to_protected`, but where a customisation needs
  several of them, or maintains them across calls, gathering them in a subclass
  conveys far more than scattering them through upstream.

- **The customisation has operations of its own.** Where we add methods that
  other code goes on to call, those methods need a type to belong to.

- **A fair number of methods are overridden.** Several insertions into as many
  upstream bodies say less, taken together, than one class that gathers them.

- **Construction or lifetime is itself part of the change.** These are
  properties of a type, and an insertion into a function body is a poor place to
  express them.

## Don't use plaster for mere additions

A `chromium_src` shadow file can introduce any file-scope code (headers,
anonymous-namespace helpers, types, etc) before and after re-including the
upstream source. They serve us well as hosts for code that does not need to be
placed in a plaster to achieve the same effect.

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
    regex:
      re_pattern: '(#include "foo/foo.h"\n)'
      replace: '\1#include "brave/bar/bar.h"\n'

  - description: 'Add kFooMax constant'
    regex:
      re_pattern: '(constexpr int kBaz = 1;\n)'
      replace: '\1constexpr int kFooMax = 42;\n'
```

## Use `chromium_src` to host substantive C++

Where the C++ accompanying a plaster amounts to more than a trivial
substitution, place it in a `chromium_src` shadow file and keep the plaster
itself as small as possible. This applies as much to a rewriter's `code` field
as it does to a regex `replace`. It improves the legibility of what is being
done to upstream code, and keeps the C++ where it can benefit from the tooling
we have in place:

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
    preempt_function_impl:
      function_name: Foo::DoSomething
      code: |-
        MaybeApplyBar(this);
```

Don't:

```yaml
# rewrite/foo/foo.cc.yaml
substitutions:
  - description: 'Apply bar in Foo::DoSomething'
    preempt_function_impl:
      function_name: Foo::DoSomething
      code: |-
        if (base::FeatureList::IsEnabled(bar::features::kBar)) {
          ApplyBar();
        }
```

Data members are no obstacle to this: the insertion runs inside the upstream
body, where they are in scope, so they can be passed to the free function by
reference, private ones included.

### Be pragmatic when deciding to use `chromium_src` or not

Where the substitution is trivial enough to be expressed plainly in the plaster
itself, moving it into a shadow file only adds indirection, and another file to
read. Avoid such convoluted uses of `chromium_src` that offer no real gain: when
the plaster alone can express the change clearly, let it.

Do — let the plaster carry the change directly:

```yaml
# rewrite/chrome/browser/ssl/ask_before_http_dialog_controller.cc.yaml
substitutions:
  - description: 'Point the Learn More link at the Brave support article.'
    regex:
      re_pattern: '(kLearnMoreLink[^=]*=\s*)(?:"[^"]*"\s*)+;'
      replace: '\1"https://support.brave.app/hc/en-us/articles/15513090104717";'
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
    regex:
      re_pattern: '^[^\n]*kLearnMoreLink\[\] =[^;]*;'
      replace: 'inline constexpr auto& kLearnMoreLink = kBraveLearnMoreLink;'
      re_flags: [MULTILINE]
```

## Whitespace in matches

This is a concern for `regex` alone; the AST rewriters match tree nodes and are
indifferent to spacing. Where a regex is warranted, `re_pattern` should, as a
rule, avoid depending on particular whitespace. Prefer the generic forms (`\s*`,
`\s+`, etc) so that a match is not made fragile by irrelevant changes in
spacing.

Literal whitespace is nonetheless acceptable, but only where _all_ of the
following hold:

- readability of the regular expression is materially improved
- the whitespace is a single space
- the file is covered by a formatting tool that enforces consistent spacing
- the chance of the whitespace ever becoming a newline is close to none

Pair flexible whitespace matching with `re_flags: [DOTALL]` so that `.` matches
across newlines as well. This often makes the regex considerably easier to read.

Do:

```yaml
# good - a reasonable mix of literal spaces and generic whitespace matching.
regex:
  re_pattern: '(enum class RequestType \{.+?,)(\s+)kMaxValue = \w+'
  re_flags: [DOTALL]
```

Don't:

```yaml
# bad - matching arbitrary newlines and whitespace throughout.
regex:
  re_pattern: '(enum class RequestType\n\{\s*\n\s+.+?,\s*\n)\s+kMaxValue *=    \w+'
  re_flags: [DOTALL]

# bad - avoiding literal single spaces at all costs, at the expense of
#       readability, and for no real gain.
regex:
  re_pattern: '(enum\s*class\s*RequestType\s*\{.+?,)(\s+)kMaxValue\s*=\s*\w+'
  re_flags: [DOTALL]
```

## Extending enums

Prefer `add_enum_entries` for a C++ enum extended at its end:

```yaml
substitutions:
  - description: 'Add the Brave properties, keeping kMaxValue last.'
    add_enum_entries:
      enum_name: RequestType
      max_value: kMaxValue
      entries:
        - kBraveOne
        - kBraveTwo
```

This rewriter does not cover every reasonable way of extending an enum, and a
regex may still be needed. Where it is, anchor the insertion either at the top
or at the end, and avoid anchoring to a particular existing key wherever
possible: any given key may be dropped or renamed at some future point. When
extending at the end, take due care of `kMaxValue`, and trailing commas.

> [!WARNING]
>
> When an enum type has its underlying value serialised and converted to and
> from a persistent storage, the keys must remain stable. Relative entry
> positioning cannot be relied upon, and extensions must instead be handled case
> by case.

Do:

```yaml
substitutions:
  - description: 'Prepend Brave entries to RequestType'
    regex:
      re_pattern: '(enum class RequestType \{)'
      replace: |-
        \1
          kBraveOne,
          kBraveTwo,
```

Don't:

```yaml
# bad - anchors on a specific key, that could very well be removed later on.
substitutions:
  - description: 'Append Brave entries after kStorageAccess'
    regex:
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
  # Good: matches `VerifyInit`, and then the switch block's opening brace.
  - description: 'Add ECDSA_SHA384 handling to VerifyInit'
    regex:
      re_pattern: '(VerifyInit\([^)]*\)\s*\{.*?\n\s*switch[^{]*\{)'
      re_flags: [DOTALL]
      replace: |-
        \1
            case ECDSA_SHA384:
              pkey_type = EVP_PKEY_EC;
              digest = EVP_sha384();
              break;
```

Don't:

```yaml
# bad - anchors on a entry value, that could be renamed, reordered, or removed
# later on.
substitutions:
  - description: 'Add ECDSA_SHA384 case'
    regex:
      pattern: 'case ECDSA_SHA256:'
      replace: |-
        case ECDSA_SHA256:
            case ECDSA_SHA384:
```

### Prefer the top of the switch over the end, on account of `default:`

Prefer anchoring the insertion right after the switch's opening brace over
anchoring it at the end. A `default:` clause may or may not be present, or may
be added in the future, producing an unintended outcome.

Where a new entry belongs at the end regardless, treat any `default:` clause as
the boundary up to which insertions may occur, present or not.

Do:

```yaml
substitutions:
  - description: 'Add ECDSA_SHA384 handling to AlgorithmToString'
    regex:
      re_pattern: '(AlgorithmToString\([^)]*\)\s*\{.*?\n\s*switch[^{]*\{.*?)(\n\s*default:)'
      re_flags: [DOTALL]
      replace: |-
        \1
            case ECDSA_SHA384:
              return "ECDSA";\2
```

### Grouping a new case with existing fall-through siblings

Where a new case must be grouped with existing fall-through siblings due to an
existing outcome in the switch, when appropriate, try to anchor the insertion on
the desired outcome itself, typically a `return` statement, rather than on any
one of the case keys. Anchoring on a key risks an undesired substitution should
that key later be removed or shuffled into another group.

Do:

```yaml
substitutions:
  - description: 'Add ECDSA_SHA384 to the EC key case in AlgorithmToType'
    regex:
      re_pattern:
        '(AlgorithmToType\([^)]*\).*?)(\n\s*return
        connectors_internals::mojom::KeyType::EC;)'
      re_flags: [DOTALL]
      replace: |-
        \1
            case crypto::SignatureVerifier::ECDSA_SHA384:\2
```

Anchoring on a key is nonetheless the more stable choice where specific key
grouping is desirable. This is especially pronounced in certain sources where
the same enum type is switched over by different functions, and key grouping has
the potential to offer a single substitution for several switch blocks across
various functions.

```yaml
substitutions:
  - description: 'Group kBraveOnly with kStorageAccess wherever it is handled.'
    count: 3 # RequestType is switched on in three places in this file.
    regex:
      re_pattern: '(case RequestType::kStorageAccess:)'
      replace: |-
        \1
          case RequestType::kBraveOnly:
```

## Use `count: 0` only when you have a very good reason

`count` sits at the substitution level and applies to every rewriter, not only
to `regex`. It defaults to `1`, and that does not need to be spelled for every
substitution. Other values do need to be spelled out though. The value for
`count` must be chosen deliberately, since in most cases we _want_ the plaster
to break once the count no longer matches, so that the change may be reviewed.

There are however rare cases in which `count: 0`, meaning one or more matches,
is sound: where the number of occurrences is known to vary over time, and where
every such occurrence is, by its nature, safe to substitute. In those cases,
accompany the `count` line with a comment explaining _why_ it is safe to do so.

Do:

```yaml
substitutions:
  - count: 0 # Any new uses will always be removed for this class.
    regex:
      re_pattern: '\s*<cr-unwanted-buttons[\s\S]+?</cr-unwanted-buttons>'
      replace: ''
```
