# Plaster

<!-- See also: patches.md for general patch best practices -->

<a id="PLSTR-001"></a>

## ✅ Plaster Patch Patterns Should Match Specific Context

**Plaster patch config `re_pattern` should generally match method names or other
relevant context to ensure a single, targeted match.** Simple patterns can be
used when the intention is to match all instances of a particular pattern in a
file.

```yaml
# ❌ WRONG - overly broad pattern that might match multiple locations
re_pattern: 'return false;'

# ✅ CORRECT - matches method name and context for targeted replacement
re_pattern: 'bool IsFeatureEnabled[\(\)\S\s\{\}]+?(return false);\s+^\}'

# ✅ ALSO CORRECT - simple pattern when all instances should match
# (Use this intentionally when you want to replace every occurrence)
re_pattern: 'kOldConstant'
```

Matching specific context (method names, surrounding code) makes patches more
maintainable and prevents accidental matches during Chromium updates. Use broad
patterns only when you explicitly intend to replace all occurrences.

---

<a id="PLSTR-002"></a>

## ✅ Use `pattern` When the Literal Is Exactly the Replaced Text, `re_pattern` for Context-Aware Matches

**Use `pattern` when the literal is exactly the text being replaced — no leading
indentation, and no adjacent tokens that are not themselves changing.** That
covers a bare symbol name and also a complete statement. Once a match needs to
reach beyond the replaced text (surrounding structure, flexible whitespace,
neighbouring conditions), `re_pattern` is the right tool.

```yaml
# ✅ CORRECT - simple symbol replacement with pattern for all instances of a constant
pattern: 'kOldConstantName'
replace: 'kNewConstantName'

# ✅ CORRECT - simple symbol replacement with pattern for all instances of a method call
pattern: 'ChromiumMethod'
replace: 'BraveMethod'

# ✅ CORRECT - a whole statement is fine: every token is text being replaced
pattern: 'return NavigateWebAppUsingParams(nav_params);'
replace:
  'return BraveNavigateWebAppUsingParams(&profile_.get(), *params_, nav_params);'

# ✅ CORRECT - re_pattern handles flexible whitespace
re_pattern: '(^\s+)(ChromiumMethod\(\))'
replace: '\1BraveMethod()'

# ❌ WRONG - pattern requires exact whitespace match, fragile to upstream changes
pattern: '    MyMethod()'  # Breaks if upstream changes indentation
replace: '    BraveMethod()'

# ✅ CORRECT - re_pattern handles flexible whitespace
re_pattern: '(if\s+\()MyMethod\(\)([\S\s]+?{)'
replace: '\1BraveMethod()\2'

# ❌ WRONG - drags in adjacent tokens that aren't being replaced; breaks if
# upstream edits the condition or the brace placement
pattern: 'if (MyMethod() && my_bool) {'
replace: 'if (BraveMethod() && my_bool) {'
```

The test is not "how long is the literal" but "is every token in it part of what
I am changing". `return Foo(bar);` passes; `if (Foo() && my_bool) {` does not,
because `&& my_bool` and the brace can change upstream without affecting the
rewrite. Reserve `re_pattern` for when you need regex features like flexible
whitespace, character classes, or structural anchors.

---

<a id="PLSTR-003"></a>

## ❌ Never Use a Macro to Edit Upstream Code in New Code — Use a Plaster

**All new uses of a macro to edit upstream code are banned. Use a plaster
rewrite instead.** A `#define` rewrites _every_ occurrence of the identifier in
all sources included after it, invisibly and often unintentionally.

```cpp
// ❌ WRONG - chromium_src/.../web_app_launch_process.cc
// Silently rewrites every NavigateWebAppUsingParams call in the TU, including
// ones pulled in by later #includes.
#define NavigateWebAppUsingParams(nav_params) \
  NavigateWebAppInContainerUsingParams(&profile_.get(), *params_, nav_params)
#include <chrome/browser/ui/web_applications/web_app_launch_process.cc>
```

```yaml
# ✅ CORRECT - rewrite/chrome/browser/ui/web_applications/web_app_launch_process.cc.yaml
# The helper (here renamed Brave*) still lives in chromium_src; only the call
# redirection moves into the plaster.
substitutions:
  - description: |
      Route command line PWA launches through the containers storage partition.
    regex:
      pattern: 'return NavigateWebAppUsingParams(nav_params);'
      replace:
        'return BraveNavigateWebAppUsingParams(&profile_.get(), *params_,
        nav_params);'
```

Only the one intended call is changed, the change is visible in the generated
patch, and `count` (default `1`) fails loudly if the premise stops holding.
Symptoms that a `#define` is fighting you: needing to reorder `#include`s so a
macro does not mangle unrelated declarations, or `#undef`-ing right after the
include.

This covers macros that merely decorate a declaration, too.
`#define SomeMethod virtual SomeMethod` is banned in new code — use
`make_virtual`, alongside `drop_final` and `add_friend` where subclassing needs
them:

```yaml
# ✅ CORRECT - rewrite/chrome/browser/download/bubble/download_display_controller.h.yaml
substitutions:
  - description: 'Let BraveDownloadDisplayController override the button state.'
    make_virtual:
      class_name: DownloadDisplayController
      method_name: UpdateToolbarButtonState
```

Prefer an AST rewriter over a regex where one fits, and never rename an upstream
function to `_ChromiumImpl` just to wrap it. Run `plaster --help` to discover
the available rewriters and `plaster --help <rewriter>` for a specific one's
full docs. See also [plaster.md](../plaster.md) and
[plaster_dos_and_donts.md](../plaster_dos_and_donts.md).

---

<a id="PLSTR-004"></a>

## ✅ Anchor Rewrites on Stable Context and Wildcard the Rest

**A rewrite should match the anchor that carries its intent, and use wildcards
for surrounding content that may change upstream without affecting the
rewrite.** Spelling out incidental detail makes the match fail for reasons
unrelated to its purpose.

```yaml
# ❌ WRONG - pins the exact current contents of the deps list. Any upstream
# addition to it breaks the match, for a reason unrelated to the rewrite.
re_pattern: '(public_deps = \[ "//chrome/browser:browser_public_dependencies" \])'

# ✅ CORRECT - anchors on the target being changed, wildcards the list body
re_pattern: '(source_set\("web_applications"\).*?\bpublic_deps\s*=\s*\[.*?\])'
re_flags: [DOTALL]
replace: '\1\n  public_deps += brave_web_applications_deps'
```

Name the thing being changed (the target, the function, the switch variable) and
let `.*?` absorb the rest. See
[rewrite/chrome/browser/extensions/BUILD.gn.yaml](../../rewrite/chrome/browser/extensions/BUILD.gn.yaml)
for a real example of this shape.

---

<a id="PLSTR-005"></a>

## ✅ Prefer a Plain `pattern` Over a Regex When the Anchor Is a Literal

**If the anchor is a single literal statement, use `pattern` — do not reach for
`re_pattern` with escaped punctuation and capture groups to reproduce
whitespace.**

```yaml
# ❌ WRONG - regex escaping and a capture group only to preserve indentation
re_pattern: '\n([^\S\n]*)return NavigateWebAppUsingParams\(nav_params\);'
replace: |-
  \n\1return BraveNavigateWebAppUsingParams(&profile_.get(), *params_, nav_params);

# ✅ CORRECT - the literal statement is its own anchor
pattern: 'return NavigateWebAppUsingParams(nav_params);'
replace:
  'return BraveNavigateWebAppUsingParams(&profile_.get(), *params_,
  nav_params);'
```

`pattern` escapes the string for you, so there is nothing to get wrong. This is
the same test as [PLSTR-002](#PLSTR-002): the literal is safe here because every
token in it is text being replaced. Do not extend it with leading indentation or
neighbouring tokens that are not changing — that is what reintroduces fragility
and calls for `re_pattern`.

---
