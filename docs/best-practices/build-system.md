# Build System

<a id="BS-001"></a>

## ✅ DEPS File - Use Commit Hashes

**In DEPS files, always use commit hashes rather than branch names or tags.** Commit hashes are immutable and ensure reproducible builds.

---

<a id="BS-003"></a>

## ✅ Reuse Existing GN Config Args

**Check for existing GN args before creating new ones.** Duplicating config arguments (e.g., creating `brave_android_keystore_path` when `android_keystore_path` already exists) adds confusion and maintenance burden.

---

<a id="BS-004"></a>

## ✅ Python Build Scripts

Python scripts used in the build system should follow these conventions:

- **Use `argparse`** for command-line arguments, not `sys.argv` directly
- **Use standard `Main()` pattern** to ensure proper error propagation to GN:
  ```python
  def Main():
      ...
      return 0

  if __name__ == '__main__':
      sys.exit(Main())
  ```

---

<a id="BS-005"></a>

## ✅ Group Sources and Deps Together in BUILD.gn

**Always group `sources` and `deps` in the same block.** Don't dump everything in one place - move files to separate BUILD.gn files when needed so it's clear which deps belong to which sources.

```gn
# ❌ WRONG - all deps in root BUILD.gn, hard to track
source_set("browser") {
  sources = [ ... 200 files ... ]
  deps = [ ... 100 deps ... ]
}

# ✅ CORRECT - grouped by feature
source_set("branded_wallpaper") {
  sources = [ "branded_wallpaper.cc" ]
  deps = [ "//brave/components/ntp_background_images" ]
}
```

---

<a id="BS-006"></a>

## ❌ Never More Than One Guard Per Target

**There should almost never be more than one of the same guard in any given BUILD.gn target.** If you find yourself repeating the same `if (enable_brave_foo)` block multiple times, consolidate.

---

<a id="BS-007"></a>

## ✅ Use Buildflags Instead of OS Guards for Features

**Use buildflags (`BUILDFLAG(ENABLE_FOO)`) instead of OS platform guards (`is_linux`, `is_win`) for feature-specific code.** Platform guards without buildflags are deprecated.

```gn
# ❌ WRONG - deprecated OS guard
if (is_linux) {
  sources += [ "tor_launcher_linux.cc" ]
}

# ✅ CORRECT - use buildflag
if (enable_tor) {
  sources += [ "tor_launcher.cc" ]
}
```

Also: only feature-specific header files should go inside feature guards. Don't put unrelated headers inside a feature guard block even if they're only currently used by that feature.

---

<a id="BS-008"></a>

## ✅ Buildflag Naming Convention

**Use `enable_brave_<feature>` as the naming convention for buildflags when the flag needs to be distinguished from a Chromium flag or is a top-level Brave feature.** The `enable_brave_` prefix is not required for flags that are clearly Brave-specific by context (e.g., a flag scoped within a Brave component's own buildflags file).

```gn
# ❌ WRONG - non-standard naming
brave_perf_predictor_enabled = true

# ✅ CORRECT - standard prefix
enable_brave_perf_predictor = true

# ✅ ALSO OK - scoped within a Brave component's buildflags, no ambiguity with Chromium
# (e.g., in components/ai_chat/core/common/buildflags/buildflags.gni)
enable_tab_management_tool = !is_android
```

---

<a id="BS-010"></a>

## ❌ Don't Duplicate License Files

**Never duplicate Chromium or other project license files.** Use special cases or references instead.

---

<a id="BS-011"></a>

## ✅ JSON Resources Should Go in GRD Files

**JSON data files should be packaged as resources in `.grd` files, not loaded from disk.** This allows the same data to be used from both C++ and JS.

See `brave/components/brave_rewards/resources/brave_rewards_static_resources.grd` for an example.

---

<a id="BS-012"></a>

## ✅ Always Double-Check Dependencies

**Always verify you have deps for all includes and used symbols.** Missing deps can work by accident through transitive dependencies but will break when those transitive deps change.

```gn
# Check for deps matching your includes:
# #include "url/gurl.h" -> needs dep "//url"
# #include "extensions/browser/..." -> needs dep "//extensions/browser/..."
```

---

<a id="BS-013"></a>

## ✅ Use `//brave/` Deps Instead of Modifying Visibility Lists

**When adding deps from Chromium targets to Brave code, use high-level `//brave/` targets (e.g., `//brave/utility`) instead of modifying Chromium visibility lists.** Visibility lists exist to prevent exactly this kind of cross-boundary dependency.

```gn
# ❌ WRONG - modifying Chromium visibility list
visibility += [ "//brave/components/brave_rewards/browser" ]

# ✅ CORRECT - use a brave target that already has visibility
deps += [ "//brave/utility" ]
```

---

<a id="BS-014"></a>

## ✅ Add `#endif` Comments for Clarity

**Add `#endif` comments to clarify what each `#endif` is closing.** See the "Refined Rule: `#endif` Comments Based on Block Length" section below for specific guidance on when to include vs omit these comments.

```cpp
#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "components/spellcheck/common/spellcheck_features.h"
#endif  // BUILDFLAG(ENABLE_SPELLCHECK)
```

---

<a id="BS-015"></a>

## ✅ Scripts Go in brave/scripts

**Build and utility scripts should go in `brave/scripts/`, not in `build/` or other Chromium directories.**

---

<a id="BS-016"></a>

## ❌ Avoid Separate Repositories

**Avoid creating separate repositories for Brave features.** Separate repos are harder to manage, code review, and maintain. Prefer keeping code within brave-core.

---

<a id="BS-017"></a>

## ✅ Add New URLs to the Network Audit Allowed List

**When adding any new network endpoint URL, it must be added to the network audit allowed list** in `brave/browser/net/brave_network_audit_allowed_lists.h`. Without this, the network audit check will fail.

---

<a id="BS-018"></a>

## ❌ Don't Use OS Guards as Proxy for Feature Guards

**Use the correct feature guard (`brave_wallet_enabled`, `enable_extensions`) instead of approximating with OS guards (`!is_android && !is_ios`).** OS guards can get out of sync with the actual feature flag logic.

```gn
# ❌ WRONG - OS guard as proxy for feature
if (!is_android && !is_ios) {
  sources += [ "brave_wallet_utils.cc" ]
}

# ✅ CORRECT - actual feature guard
if (brave_wallet_enabled) {
  sources += [ "brave_wallet_utils.cc" ]
}
```

---

<a id="BS-019"></a>

## ✅ Use `use_blink` for Content-Layer Dependencies, Not `!is_ios`

**Code that depends on the content layer (`//content/...`) must be guarded with `use_blink`, not `!is_ios`.** The `use_blink` flag is the correct way to detect content/Blink availability. Using `!is_ios` as a proxy is incorrect because `use_blink` is the actual flag that controls whether the content layer is built, and `!is_ios` may not always be equivalent.

```gn
# ❌ WRONG - using !is_ios as proxy for content availability
if (!is_ios) {
  sources += [ "content_helper.cc" ]
  deps += [ "//content/public/browser" ]
}

# ✅ CORRECT - use_blink indicates content layer availability
if (use_blink) {
  sources += [ "content_helper.cc" ]
  deps += [ "//content/public/browser" ]
}
```

```cpp
// ❌ WRONG
#if !BUILDFLAG(IS_IOS)
#include "content/public/browser/web_contents.h"
#endif

// ✅ CORRECT
#if BUILDFLAG(USE_BLINK)
#include "content/public/browser/web_contents.h"
#endif
```

---

<a id="BS-020"></a>

## ❌ Don't Have Both `BUILD.gn` and `sources.gni` in the Same Directory

**A directory should contain either a `BUILD.gn` file (preferred) or a `sources.gni` file, but not both.** Having both creates confusion about which is authoritative and makes dependency tracking harder.

---

<a id="BS-053"></a>

## ✅ Use `sources.gni` Only for Circular Dependencies with Upstream

**Only use `sources.gni` when inserting source files into upstream Chromium targets with circular deps.** For all other cases, use normal `BUILD.gn` targets. Putting everything in `sources.gni` hurts incremental builds because changes trigger rebuilds of large upstream targets.

---

<a id="BS-021"></a>

## ✅ Create Test Targets in Component BUILD.gn

**Unit test files should have a test target in the component's `BUILD.gn`, not be individually listed in the top-level `test/BUILD.gn`.** The top-level test target should depend on the component's test target.

```gn
# ❌ WRONG - individual test files in top-level test/BUILD.gn
sources += [ "//brave/components/ai_chat/core/credential_manager_unittest.cc" ]

# ✅ CORRECT - test target in component BUILD.gn
# components/ai_chat/core/BUILD.gn
source_set("unit_tests") {
  sources = [ "credential_manager_unittest.cc" ]
  deps = [ ... ]
}
# test/BUILD.gn
deps += [ "//brave/components/ai_chat/core:unit_tests" ]
```

---

<a id="BS-022"></a>

## ✅ Create `test_support` Targets for Reusable Fakes/Mocks

**When creating fake or mock implementations of services for testing, put them in a separate `test_support` BUILD.gn target rather than embedding them directly in a `unit_tests` target.** This allows multiple test targets across the codebase to reuse the same fakes.

```gn
# ❌ WRONG - fake service only available to one test target
source_set("unit_tests") {
  sources = [
    "my_service_unittest.cc",
    "test/fake_dependency_service.cc",
    "test/fake_dependency_service.h",
  ]
}

# ✅ CORRECT - separate test_support target for reusable fakes
source_set("test_support") {
  testonly = true
  sources = [
    "test/fake_dependency_service.cc",
    "test/fake_dependency_service.h",
  ]
  deps = [ ":dependency_service" ]
}

source_set("unit_tests") {
  sources = [ "my_service_unittest.cc" ]
  deps = [
    ":test_support",
    ...
  ]
}
```

---

<a id="BS-023"></a>

## ✅ Use `PlatformBrowserTest` for Cross-Platform Browser Tests

**Browser tests that should run on both desktop and Android should use `PlatformBrowserTest` as the base class instead of `InProcessBrowserTest`.**

```cpp
#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

// ❌ WRONG
class MyBrowserTest : public InProcessBrowserTest {};

// ✅ CORRECT
class MyBrowserTest : public PlatformBrowserTest {};
```

---

<a id="BS-024"></a>

## ✅ Use `public_deps` for Header-File Includes in BUILD.gn

**When a dependency's headers are included in your target's header files (not just .cc files), that dependency must be listed in `public_deps`, not `deps`.** This ensures consumers of your target also get the transitive include paths they need.

```gn
# ❌ WRONG - header-visible dependency in regular deps
source_set("my_service") {
  sources = [ "my_service.h", "my_service.cc" ]
  deps = [ "//components/prefs" ]  # prefs is used in my_service.h!
}

# ✅ CORRECT - header dependency in public_deps
source_set("my_service") {
  sources = [ "my_service.h", "my_service.cc" ]
  public_deps = [ "//components/prefs" ]  # used in header
  deps = [ "//base" ]  # only used in .cc
}
```

---

<a id="BS-026"></a>

## ✅ Utility Scripts Should Be Python, Not Node.js or Shell

**Build and utility scripts in brave-core should be written in Python (using `vpython` from depot tools), not Node.js or shell scripts.** This follows Chromium conventions, avoids additional runtime dependencies, and works on all platforms including Windows.

---

<a id="BS-027"></a>

## ✅ Unconditional Buildflags Deps with Conditional `deps +=`

**When adding `deps +=` inside an `if(enable_feature)` block in GN, always add an unconditional `buildflags` dependency outside the conditional.** The buildflags header is needed even when the feature is disabled (for the `#if BUILDFLAG(...)` check itself). Run `gn check` with the buildflag disabled to verify.

```gn
# ❌ WRONG - buildflags dep only inside conditional
if (enable_brave_rewards) {
  deps += [
    "//brave/components/brave_rewards/common/buildflags",
    "//brave/components/brave_rewards/browser",
  ]
}

# ✅ CORRECT - buildflags dep unconditional
deps += [
  "//brave/components/brave_rewards/common/buildflags",
]
if (enable_brave_rewards) {
  deps += [ "//brave/components/brave_rewards/browser" ]
}
```

---

<a id="BS-028"></a>

## ✅ Use source_set Only for Internal Targets (with Restricted Visibility)

**Public targets for a component should use `static_library` or `component`, not `source_set`.** Only internal deps should use `source_set`, and those must have restricted visibility to prevent external use.

```gn
# ❌ WRONG - internal source_set with default (public) visibility
source_set("internal_network") {
  sources = [ ... ]
}

# ✅ CORRECT - restricted visibility
source_set("internal_network") {
  visibility = [ ":*" ]  # Only usable within this BUILD.gn
  sources = [ ... ]
}
```

---

<a id="BS-029"></a>

## ✅ Python File Writes: Use `newline='\n'`

**When writing files from Python scripts, always specify `newline='\n'`** in `open()` to ensure consistent LF line endings across platforms (especially Windows).

```python
# ❌ WRONG - platform-dependent line endings
with open(output_path, 'w') as f:
    f.write(content)

# ✅ CORRECT - consistent LF endings
with open(output_path, 'w', newline='\n') as f:
    f.write(content)
```

---

<a id="BS-030"></a>

## ✅ Use `include_rules` for Common Includes, `specific_include_rules` for Edge Cases

**In DEPS files, use `include_rules` for generally allowed includes across all files in a directory.** Reserve `specific_include_rules` for edge cases where an include should only be allowed in specific files.

```python
# ❌ WRONG - using specific_include_rules for commonly needed includes
specific_include_rules = {
  ".*\.cc": [
    "+brave/components/content_settings/core/common",
    "+brave/components/content_settings/core/browser",
  ],
}

# ✅ CORRECT - include_rules for generally allowed includes
include_rules = [
  "+brave/components/content_settings/core/common",
  "+brave/components/content_settings/core/browser",
]
```

---

<a id="BS-031"></a>

## ✅ Bump `resource_ids.spec` by 5

**When adding new resource IDs in `resource_ids.spec`, bump the next ID by 5 (not 1)** to leave room for additions without conflicting with adjacent entries.

---

<a id="BS-032"></a>

## ✅ Use `component()` Not `static_library()` for Service GN Targets

**Mojo service targets should use `component()` instead of `static_library()` in BUILD.gn.** Components support dynamic linking and are the correct target type for service implementations.

---

<a id="BS-033"></a>

## ❌ Unit Tests for Components Must NOT Live in Browser

**Unit tests for code in `//brave/components` must not be placed in `//brave/browser`.** Tests should live alongside the code they test. Use `content::RenderViewHostTestHarness` for component-level tests that need content layer support.

```gn
# ❌ WRONG - component test in browser directory
# browser/ai_chat/associated_link_content_unittest.cc
# testing code from components/ai_chat/content/browser/

# ✅ CORRECT - test alongside the code
# components/ai_chat/content/browser/associated_link_content_unittest.cc
source_set("unit_tests") {
  sources = [ "associated_link_content_unittest.cc" ]
}
```

---

<a id="BS-034"></a>

## ✅ Place Includes Inside BUILDFLAG Guards When Only Used There

**When an `#include` is only used inside a `#if BUILDFLAG(...)` block, the include must also be inside that guard.** An unconditional include for a conditionally-used header breaks builds when the feature is disabled.

**IMPORTANT: Only apply this rule when the BUILDFLAG actually exists.** Before suggesting that code be wrapped in a `#if BUILDFLAG(...)` guard, verify the buildflag is defined in the codebase (check `buildflags.gni` files or existing usage). Never fabricate or assume a buildflag name — if no buildflag exists for a feature, do not invent one. Instead, check if the feature uses `base::FeatureList` runtime checks or has no compile-time guard at all.

```cpp
// ❌ WRONG - unconditional include for conditionally-used header
#include "chrome/browser/extensions/extension_web_ui.h"
// ...
#if BUILDFLAG(ENABLE_EXTENSIONS)
  IsChromeURLOverridden(...);  // uses extension_web_ui.h
#endif

// ✅ CORRECT - include inside the same guard
#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_web_ui.h"
  IsChromeURLOverridden(...);
#endif
```

---

<a id="BS-035"></a>

## ✅ Merge Consecutive Identical BUILDFLAG Blocks

**When multiple consecutive code regions use the same `#if BUILDFLAG(...)` condition, merge them into a single guard block.**

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

---

<a id="BS-036"></a>

## ✅ DEPS Allowlist Paths Must Exactly Match Include Paths

**DEPS file allowlist paths must exactly match the `#include` paths used in source files.** A mismatch (e.g., `common/` vs `core/`) means the DEPS check doesn't validate the right file.

---

<a id="BS-037"></a>

## ✅ Use `assert()` at Top of Entire-Feature BUILD.gn Files

**If an entire BUILD.gn file belongs to a feature, use `assert(enable_feature)` at the top** rather than wrapping individual blocks in `if (enable_feature)`.

```gn
# ❌ WRONG - wrapping everything inside a conditional
if (enable_brave_wallet) {
  source_set("wallet_tests") {
    sources = [ ... ]
  }
}

# ✅ CORRECT - assert at top
assert(enable_brave_wallet)
source_set("wallet_tests") {
  sources = [ ... ]
}
```

---

<a id="BS-038"></a>

## ❌ Don't Confuse Feature Flags with Build Flags

**Runtime feature flags (`base::FeatureList`) and compile-time build flags (`BUILDFLAG()`) are completely different mechanisms.** Do not confuse them in either direction: runtime feature flags cannot gate build-time deps, and buildflag-guarded code does not need a redundant runtime feature flag on top.

- **Build flags** (e.g., `enable_brave_wallet`, `BUILDFLAG(ENABLE_EXTENSIONS)`) are set at compile time via GN args. They can guard `deps`, `sources`, `#include` blocks, and code blocks.
- **Feature flags** (e.g., `base::FeatureList::IsEnabled(kMyFeature)`) are checked at runtime. They can only guard code execution, not build-time dependency inclusion.

```gn
# ❌ WRONG - runtime feature flag cannot gate build-time deps
if (kLocalAIModels) {  # Does not exist in GN!
  deps += [ "//brave/components/local_ai/resources" ]
}

# ✅ CORRECT - use a build flag if one exists
if (enable_local_ai) {
  deps += [ "//brave/components/local_ai/resources" ]
}

# ✅ ALSO CORRECT - if no build flag exists yet, don't invent one
# Include deps unconditionally and let runtime feature checks handle enablement
deps += [ "//brave/components/local_ai/resources" ]
```

---

<a id="BS-049"></a>

## ✅ Add `static_assert` in Public Headers for Build Flag Guards

**When introducing a build flag for a component, add `static_assert` in public-facing headers** to catch accidental inclusion when the feature is disabled.

```cpp
// In brave/components/brave_wallet/browser/brave_wallet_service.h
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
static_assert(BUILDFLAG(ENABLE_BRAVE_WALLET));
```

---

<a id="BS-039"></a>

## ❌ Don't Use `nogncheck` - Fix the Underlying Dep Guard

**Do not use `nogncheck` to suppress gn check failures.** Instead fix the underlying GN dependency to be correctly conditional. `nogncheck` is a code smell that masks real build dependency issues.

```cpp
// ❌ WRONG - suppressing the real problem
#include "brave/components/foo/bar.h"  // nogncheck

// ✅ CORRECT - fix the GN dep to be properly guarded
// Ensure the dep is conditionally added in BUILD.gn
```

---

<a id="BS-040"></a>

## ✅ GN Deps Guarded by Feature Flags Only, Not Platform Guards

**Within a single GN target, deps should be added behind the relevant buildflags only, not nested inside platform guards.** GN and C++ guards should always match.

```gn
# ❌ WRONG - nested inside platform guard
if (is_mac) {
  if (enable_tor) {
    deps += [ "//brave/components/tor" ]
  }
}

# ✅ CORRECT - feature flag only
if (enable_tor) {
  deps += [ "//brave/components/tor" ]
}
```

---

<a id="BS-041"></a>

## ❌ Don't Guard Deps with Unrelated Guards

**A dep should only be placed inside a guard if the dep itself is specific to that guard condition.** If a dep is needed regardless of the guard condition, include it unconditionally. Putting unrelated deps inside guards (e.g., placing a general utility dep inside `if (use_blink)` or `if (!is_ios)` when it has nothing to do with blink/iOS) creates incorrect build configurations and can cause missing symbol errors on platforms where the guard evaluates to false.

```gn
# ❌ WRONG - "//brave/components/constants" has nothing to do with blink
if (use_blink) {
  deps += [
    "//brave/components/constants",
    "//content/public/browser",
  ]
}

# ✅ CORRECT - only guard deps that actually relate to the guard
deps += [
  "//brave/components/constants",
]
if (use_blink) {
  deps += [ "//content/public/browser" ]
}
```

---

<a id="BS-042"></a>

## ✅ When Disabling a Feature, Compile Out Its Tests

**When a build flag disables a feature, exclude the feature-specific test files from the build** using conditionals rather than trying to fix compilation errors by adjusting test dependencies.

```gn
# ❌ WRONG - fixing deps to make disabled feature tests compile
deps += [ "//brave/components/brave_rewards" ]  # just for tests

# ✅ CORRECT - compile out disabled feature tests
if (enable_brave_rewards) {
  sources += [ "rewards_service_unittest.cc" ]
}
```

---

<a id="BS-043"></a>

## ✅ Build Flag Validation Requires Both States

**When adding a build flag, run `gn_check`, unit tests, component tests, browser tests, and presubmit with the flag both enabled AND disabled.** Issues frequently only appear in the disabled state.

---

<a id="BS-044"></a>

## ✅ Use Chromium UI Preprocessor for Conditional WebUI Code

**Use `// <if expr="enable_feature">` in JS and `<if expr="enable_feature">` in HTML to conditionally compile feature-specific WebUI code.** This completely removes the code from the build when the feature is disabled, rather than relying solely on runtime `loadTimeData` checks.

```js
// ✅ CORRECT - code removed at build time when feature disabled
// <if expr="enable_speedreader">
import { SpeedreaderPage } from './speedreader_page.js';
// </if>
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

<a id="BS-046"></a>

## ✅ Assert Build Flag Dependencies Between Features

**When Feature A depends on Feature B, assert Feature B's build flag is true when Feature A is enabled** rather than making Feature A work without Feature B for unsupported configurations.

```gn
# In brave/components/brave_rewards/BUILD.gn
assert(enable_brave_wallet,
       "Rewards requires Wallet (enable_brave_wallet=true)")
```

---

<a id="BS-048"></a>

## ✅ Share Constants via Common Headers

**Shared constants (like limits, URLs, keys) should be defined in a common header** to avoid duplicate definitions across implementation, test, and other files.

```cpp
// ❌ WRONG - same constant in 3 files
// model_service.cc
constexpr char kOllamaEndpoint[] = "http://localhost:11434";
// model_service_unittest.cc
constexpr char kOllamaEndpoint[] = "http://localhost:11434";
// ollama_model_fetcher.cc
constexpr char kOllamaEndpoint[] = "http://localhost:11434";

// ✅ CORRECT - shared header
// common/constants.h
inline constexpr char kOllamaEndpoint[] = "http://localhost:11434";
```

---

<a id="BS-050"></a>

## ❌ Never Add Anything to `components/constants` (Deprecated)

**The `components/constants` directory is not a real component — it is a deprecated legacy dumping ground that should be removed.** Never add new constants, headers, or targets there. Instead, place constants in a `common` target within the component that owns them.

```gn
# ❌ WRONG - adding to the legacy catch-all
# components/constants/brave_constants.h
inline constexpr char kGate3OAuthUrl[] = "...";

# ✅ CORRECT - in the owning component's common target
# components/brave_rewards/common/constants.h
inline constexpr char kGate3OAuthUrl[] = "...";
```

Do not add new `deps` on `//brave/components/constants` either — migrate existing usages to proper component-owned targets when you encounter them.

If a constant (especially a URL) is shared across multiple components with no clear owner, place it in a `brave_domains` target.

---

<a id="BS-051"></a>

## ✅ Forward Declarations Don't Need `BUILDFLAG` Guards

**Only `#include` directives and actual usages need to be wrapped in `#if BUILDFLAG(...)` guards.** Forward declarations are harmless — they don't pull in dependencies and cost nothing at compile time if unused.

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

<a id="BS-052"></a>

## ❌ Don't Use Compound Buildflags (Channel + Platform + Official)

**Do not create buildflags that combine channel, platform, and official_build conditions** (e.g., enabled only on nightly + desktop + official). These compound flags break during branch migration (nightly → beta → release) and create configurations that are nearly impossible to test locally. Use separate, independently testable flags instead.

---

<a id="BS-054"></a>

## ❌ Don't Include Buildflag Headers in Conditionally-Compiled Files

**If a file is only compiled when a buildflag is enabled (guarded by `if(enable_feature)` in BUILD.gn), that file should not `#include` the buildflag header or use `#if BUILDFLAG(...)` guards.** The file is never compiled when the flag is disabled, so checking the flag inside it is redundant and misleading.

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

Note: Public headers may still benefit from a `static_assert(BUILDFLAG(...))` as a safety net against accidental inclusion (see BS-049). This rule applies to implementation files and internal headers that are strictly behind the BUILD.gn guard.

