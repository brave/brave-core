# C++ Buildflag Guards

Rules for `#if BUILDFLAG(...)` usage inside C++ source and header files. GN-side
buildflag rules (declaring flags, guarding `deps`/`sources`) live in
[build-system.md](./build-system.md).

<a id="BS-014"></a>

## ✅ Add `#endif` Comments for Clarity

**Add `#endif` comments to clarify what each `#endif` is closing.** See the
"Refined Rule: `#endif` Comments Based on Block Length" section below for
specific guidance on when to include vs omit these comments.

```cpp
#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "components/spellcheck/common/spellcheck_features.h"
#endif  // BUILDFLAG(ENABLE_SPELLCHECK)
```

---

<a id="BS-034"></a>

## ✅ Guard an Include Only When the Header Is Conditionally Available

**Move an `#include` inside a `#if BUILDFLAG(...)` block only when the header
itself is conditionally available** — i.e. its target/sources are gated behind
the same buildflag in GN, or the header sits behind its own buildflag. In that
case an unconditional include breaks the build when the feature is disabled,
because the header does not exist in that configuration.

**Do NOT guard an include just because it is only _used_ inside a
`#if BUILDFLAG(...)` block.** Most Chromium headers (e.g. `web_contents.h`,
`tab_interface.h`, `history_service.h`) are always built regardless of any
feature flag. An unconditional include of an always-available header does not
break a disabled-feature build — it merely pulls in a header you don't use under
that config, which is harmless. Wrapping such includes in guards adds churn with
no benefit. Before suggesting a guard, confirm the header is actually gated in
GN (check the relevant `BUILD.gn` `sources` lists / buildflag conditions); if it
is always built, leave the include unconditional.

**IMPORTANT: Only apply this rule when the BUILDFLAG actually exists.** Before
suggesting that code be wrapped in a `#if BUILDFLAG(...)` guard, verify the
buildflag is defined in the codebase (check `buildflags.gni` files or existing
usage). Never fabricate or assume a buildflag name — if no buildflag exists for
a feature, do not invent one. Instead, check if the feature uses
`base::FeatureList` runtime checks or has no compile-time guard at all.

```cpp
// ❌ WRONG - unconditional include of a GN-gated header
// extension_web_ui.h is only built when ENABLE_EXTENSIONS is set, so this
// breaks the build when extensions are disabled.
#include "chrome/browser/extensions/extension_web_ui.h"
// ...
#if BUILDFLAG(ENABLE_EXTENSIONS)
  IsChromeURLOverridden(...);  // uses extension_web_ui.h
#endif

// ✅ CORRECT - include inside the same guard, because the header is GN-gated
#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_web_ui.h"
  IsChromeURLOverridden(...);
#endif

// ✅ ALSO CORRECT - leave always-built headers unconditional even when only
// used inside the guard. content/public/browser/web_contents.h is always
// available, so guarding its include is unnecessary.
#include "content/public/browser/web_contents.h"
// ...
#if BUILDFLAG(ENABLE_AI_CHAT)
  SearchTabsByContent(...);  // uses web_contents.h
#endif
```

---

<a id="BS-035"></a>

## ✅ Merge Consecutive Identical BUILDFLAG Blocks

**When multiple consecutive code regions use the same `#if BUILDFLAG(...)`
condition, merge them into a single guard block.**

```cpp
// ❌ WRONG - redundant guards
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
namespace rewards { ... }
#endif

// ✅ CORRECT - single merged block
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
namespace rewards { ... }
#endif
```

**Only merge a guarded include with guarded code when it's the single guard
block immediately following the regular (non-guarded) includes.** Includes must
always come first; only actual code may follow. When more than one distinct
buildflag block has its own guarded include, do NOT merge each include down into
its code block — that would place a later include after code (e.g. after a
`namespace {...}`), which is incorrect ordering. Keep the guarded includes
grouped with the other includes at the top instead.

```cpp
// ❌ WRONG - the second include ends up after code
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
namespace rewards { ... }
#endif

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/components/brave_ads/core/buildflags/buildflags.h"
namespace brave_ads { ... }
#endif

// ✅ CORRECT - guarded includes stay grouped at the top with other includes
#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#endif
#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/components/brave_ads/core/buildflags/buildflags.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
namespace rewards { ... }
#endif
#if BUILDFLAG(ENABLE_BRAVE_ADS)
namespace brave_ads { ... }
#endif
```

---

<a id="BS-045"></a>

## ✅ Refined Rule: `#endif` Comments Based on Block Length

**Clarification of the `#endif` comment rule:**

- **Always add** for blocks > 3 lines
- **Always add** when inside nested `#if` blocks
- **Always add** when surrounding code already uses them (consistency)
- **Can omit** for short (1-2 line), unambiguous blocks

---

<a id="BS-049"></a>

## ✅ Add `static_assert` in Public Headers for Build Flag Guards

**When introducing a build flag for a component, add `static_assert` in
public-facing headers** to catch accidental inclusion when the feature is
disabled.

```cpp
// In brave/components/brave_wallet/browser/brave_wallet_service.h
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
static_assert(BUILDFLAG(ENABLE_BRAVE_WALLET));
```

---

<a id="BS-051"></a>

## ✅ Forward Declarations Don't Need `BUILDFLAG` Guards

**Only `#include` directives and actual usages need to be wrapped in
`#if BUILDFLAG(...)` guards.** Forward declarations are harmless — they don't
pull in dependencies and cost nothing at compile time if unused.

```cpp
// ❌ UNNECESSARY - guarding a forward declaration
#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT)
class HistoryTool;
#endif

// ✅ CORRECT - forward declaration needs no guard
class HistoryTool;

// Guard the includes and usage instead:
#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT)
#include "brave/browser/ai_chat/tools/history_tool.h"
#endif
```

---

<a id="BS-054"></a>

## ❌ Don't Include Buildflag Headers in Conditionally-Compiled Files

**If a file is only compiled when a buildflag is enabled (guarded by
`if(enable_feature)` in BUILD.gn), that file should not `#include` the buildflag
header just to re-check the enable/disable flag with `#if BUILDFLAG(...)`.** The
file is never compiled when the flag is disabled, so re-checking that same flag
inside it is redundant and misleading.

This applies only to the enable/disable flag that already gates the file.
Buildflags also carry other GN-passed parameters (keys, URLs, numeric limits,
etc.), and reading those values is perfectly valid — in that case including the
buildflag header is expected, not redundant.

```cpp
// In brave/components/foo/foo_impl.cc
// (only in sources when enable_foo = true in BUILD.gn)

// ❌ WRONG - redundant buildflag include and guard
#include "brave/components/foo/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_FOO)
void DoFoo() { ... }
#endif

// ✅ CORRECT - file is already conditionally compiled, no guard needed
void DoFoo() { ... }
```

Note: Public headers may still benefit from a `static_assert(BUILDFLAG(...))` as
a safety net against accidental inclusion (see BS-049). This rule applies to
implementation files and internal headers that are strictly behind the BUILD.gn
guard.
