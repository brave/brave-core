# chromium_src Overrides

<a id="CSRC-001"></a>

## ✅ Avoid Modifying Chromium Source When Possible

**Changes to Chromium code in `src/` should be avoided whenever possible.**
Implement features and fixes entirely within brave-core (`src/brave/`) using
Brave's own code and APIs.

When Chromium changes are unavoidable, follow this preference order:

1. **chromium_src overrides** (strongly preferred) — Override files in
   `src/brave/chromium_src/` that replace or wrap upstream behavior at compile
   time. See the sections below for patterns and conventions.

2. **Patches** (last resort) — When a chromium_src override is not feasible
   (e.g., you need to add a `virtual` keyword where `#define` tricks don't work,
   or modify a GN file), directly edit the files in `src/` and then generate
   patches:

   ```bash
   cd src/brave
   pnpm run update_patches
   ```

   This reads the changes you made to files in `src/` and updates the
   corresponding patch files in `src/brave/patches/`. Always keep patches
   minimal — one-line additions are ideal.

---

<a id="CSRC-002"></a>

## ❌ Don't Use chromium_src Overrides to Disable Tests

**Never use `#define TestName DISABLED_TestName` in chromium_src overrides to
disable upstream tests.** Use filter files in `test/filters/` instead, and move
any Brave-specific replacement tests into the appropriate Brave test target
(`brave_unit_tests`, `brave_browser_tests`, `brave_components_unittests`).

---

<a id="CSRC-003"></a>

## ✅ Minimize Code Duplication in Overrides

**When overriding Chromium code via `chromium_src/`, prefer wrapping only the
changed section and falling back to `ChromiumImpl` for everything else.**

Don't duplicate entire functions when only part of the logic needs to change.

**BAD:**

```cpp
// ❌ WRONG - duplicating the entire function
SkColor ChromeTypographyProvider::GetColor(...) {
  // 50 lines copied from upstream...
  // only 3 lines are actually different
}
```

**GOOD:**

```cpp
// ✅ CORRECT - wrap only the changed section
SkColor ChromeTypographyProvider::GetColor(...) {
  if (!ShouldIgnoreHarmonySpec(*native_theme)) {
    return ChromiumImpl::GetColor(...);  // Fallback to upstream
  }
  // Only our custom logic here
}
```

---

<a id="CSRC-004"></a>

## ✅ Prefer chromium_src Overrides Over Patches

**Always prefer a chromium_src override over adding a patch.** Patches are
harder to maintain, more likely to conflict, and harder to review. Required
header files should be added through chromium_src overrides, not patches.

```cpp
// ❌ WRONG - adding a patch to include a header
// In patches/chromium/some_patch.patch
+#include "brave/components/my_feature/my_header.h"

// ✅ CORRECT - chromium_src override
// In chromium_src/chrome/browser/some_file.cc
#include "brave/components/my_feature/my_header.h"
```

When you need to add virtual to a method, add a class method, or intercept
behavior, always use chromium_src overrides instead of patches.

---

<a id="CSRC-005"></a>

## ❌ Never Copy Entire Files or Methods

**Never copy entire Chromium files or methods into chromium_src.** Only override
the specific part that needs to change, and call the superclass for everything
else.

```cpp
// ❌ WRONG - copying entire method (50+ lines) when only 3 lines differ
void SomeClass::LargeMethod() {
  // ... 50 lines copied from Chromium ...
  // Only 3 lines actually changed
}

// ✅ CORRECT - override just the changed part, call super for the rest
void SomeClass::LargeMethod() {
  // Call the original for most of the work
  auto* toast = [builder buildUserNotification];
  // Add only our custom logic
  [toast setCustomField:value];
}
```

---

<a id="CSRC-007"></a>

## ✅ Prefer Subclassing Over Patching

**When possible, subclass Chromium classes using chromium_src overrides instead
of patching.** This applies to both C++ and Java.

**C++ example - changing a class via chromium_src:**

```cpp
// ❌ WRONG - patching tab_strip.cc to change behavior
+  BraveNewTabButton* new_tab_button = new BraveNewTabButton(...);

// ✅ CORRECT - chromium_src override that changes the class name
// In chromium_src/chrome/browser/ui/views/tabs/tab_strip.cc
#define NewTabButton BraveNewTabButton
```

**Java example - subclassing instead of patching:**

```java
// ❌ WRONG - patching IncognitoNewTabPageView directly

// ✅ CORRECT - create BraveIncognitoNewTabPage as subclass
// Patch only changes the superclass reference
```

Subclassing is better for long-term maintenance and makes changes easier to
understand. One patch to change a superclass is better than multiple patches to
modify individual methods.

---

<a id="CSRC-012"></a>

## ✅ Always Use Original Header Paths

**In chromium_src overrides, always `#include` the original header path, not the
chromium_src version.**

```cpp
// ❌ WRONG
#include "brave/chromium_src/net/proxy_resolution/proxy_resolution_service.h"

// ✅ CORRECT
#include "net/proxy_resolution/proxy_resolution_service.h"
```

---

<a id="CSRC-014"></a>

## ✅ Replace Entire Classes with Dummy chromium_src Files

**When you need to disable or replace a Chromium class entirely, create a
minimal no-op dummy replacement in chromium_src for the `.h` and `.cc` files.**
This avoids needing a patch and makes maintenance easier.

```cpp
// ❌ WRONG - two patches to disable a class
patches/components-translate-core-browser-translate_url_fetcher.cc.patch
patches/components-translate-core-browser-translate_url_fetcher.h.patch

// ✅ CORRECT - chromium_src replacement with no-op implementation
// chromium_src/components/translate/core/browser/translate_url_fetcher.h
class TranslateURLFetcher {
 public:
  TranslateURLFetcher() = default;
  bool Request(const GURL& url, Callback callback) { return false; }
};
```

---

<a id="CSRC-015"></a>

## ❌ Don't Use `#define` to Add `virtual` — Use `make_virtual`

**When a Chromium method needs to be made virtual for override, use the
`make_virtual` plaster rewriter, not a `#define` in a chromium_src header
override.**

```cpp
// ❌ WRONG - chromium_src define; rewrites every SomeMethod in the TU
#define SomeMethod virtual SomeMethod
#include "src/path/to/original_header.h"
#undef SomeMethod
```

```yaml
# ✅ CORRECT - rewrite/path/to/original_header.h.yaml
substitutions:
  - description: 'Let BraveSomeClass override SomeMethod.'
    make_virtual:
      class_name: SomeClass
      method_name: SomeMethod
```

The rewriter also handles what the `#define` could not: a pointer or reference
return type (`T* Method()`), and a destructor (quote it — `'~SomeClass'`). See
[PLSTR-003](./plaster.md#PLSTR-003).

---

<a id="CSRC-016"></a>

## ❌ Never Use `#define final` to Remove the `final` Keyword

**Redefining `final` via `#define` is undefined behavior per the C++ standard
and is highly viral.** It can cause build failures in unrelated code that uses
`final` in different contexts. Use a patch to remove `final` when subclassing is
required, or find alternative approaches.

```cpp
// ❌ WRONG - undefined behavior, viral side effects
#define final
#include "src/chrome/browser/ui/views/side_panel/side_panel_coordinator.h"
#undef final

// ✅ CORRECT - use a minimal patch if no alternative exists
// Or use #define only for specific method names that won't collide
```

---

<a id="CSRC-017"></a>

## ✅ Add Explanation Comments in chromium_src Override Files

**When creating a chromium_src override, include comments explaining why the
override is needed.** The override's purpose is not always self-evident from the
code alone. Per-function comments explaining each overridden function are
sufficient — a global file-level comment is not required if the per-function
comments adequately explain the purpose.

**Exception:** Do not request doc comments on method declarations injected
inside `#define` macros (e.g., macros that add virtual methods to an upstream
class). Comments cannot be practically added inside macro bodies. If
documentation is needed, it belongs in the header file that defines the macro or
in the implementation file, not inline in the macro expansion.

```cpp
// chromium_src/chrome/browser/ui/views/tabs/tab_view.cc

// Add Brave-specific tab context menu items. The upstream class doesn't
// support extensibility here, so we replace the menu construction logic.
void TabView::BuildContextMenu() {
  ...
}
```

---

<a id="CSRC-018"></a>

## ✅ Verify chromium_src Header GN Dependencies

**When adding new `#include` directives in chromium_src override files, verify
they have required GN dependencies.** The override file is compiled as part of
the upstream target, which may not have deps on your Brave headers.

**How to verify:** Temporarily add the same headers to the original upstream
file and run `gn check`. If it fails, the upstream target needs the dependency
added (typically via a patch or `sources.gni`).

```bash
# Test: add the header to the original file temporarily, then:
gn check out/Default
# If it fails, the dep is missing
```

---

<a id="CSRC-019"></a>

## ❌ chromium_src Should Avoid Depending on Brave Targets

**A chromium_src override should avoid adding a GN dependency on, or a direct
`#include` of, any downstream `//brave/...` target** (`//brave/components/...`,
`//brave/browser/...`, `//brave/chrome/...`, etc.). The override is compiled as
part of the upstream target, so pulling a Brave target into it creates an
upstream → Brave dependency that breaks whenever upstream modularizes, grows the
patched target's dependency graph, and forces us to patch Brave deps into the
upstream GN config.

Prefer keeping the override minimal: **forward-declare a free function** (or
factory) in the override and call it. Put the hook implementation in a matching
Brave source file outside `chromium_src` (for example,
`brave/browser/ui/window_feature_controller/window_feature_controller.cc`) and
put that source in a dedicated Brave target in the same directory. That target
owns the Brave `#include` and its implementation dependencies. Finally, make the
relevant existing top-level Brave target (for example, `//brave/browser:core`)
depend on the dedicated implementation target so the hook is linked into the
same binary as the upstream target. The override itself should include no Brave
header and the upstream target should not need a patched dependency on Brave
implementation details.

**BAD:**

```cpp
// ❌ WRONG - the override includes Brave implementation details
// chromium_src/chrome/browser/ui/window_feature_controller/window_feature_controller.cc
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"

bool WindowFeatureController::UsesImmersiveFullscreenMode() const {
  if (tabs::utils::ShouldUseImmersiveFullscreen(browser_)) {
    return true;
  }
  return WindowFeatureController::UsesImmersiveFullscreenMode_ChromiumImpl();
}
```

```diff
# ❌ WRONG - the upstream GN file knows about Brave implementation dependencies
# chrome/browser/ui/window_feature_controller/BUILD.gn (patched upstream file)
 source_set("window_feature_controller") {
   sources = [
     ...
     "window_feature_controller.cc",
+    ...
   ]

   deps = [
     ...
   ]
+  deps += [ "//brave/browser/ui/views/tabs" ]
 }
```

**GOOD:**

```cpp
// ✅ CORRECT - the override calls a forward-declared hook
// chromium_src/chrome/browser/ui/window_feature_controller/window_feature_controller.cc
class Browser;

std::optional<bool> BraveUsesImmersiveFullscreenMode(bool disabled_at_startup,
                                                     Browser* browser);

bool WindowFeatureController::UsesImmersiveFullscreenMode() const {
  if (auto result =
          BraveUsesImmersiveFullscreenMode(disabled_at_startup_, browser_)) {
    return *result;
  }
  return WindowFeatureController::UsesImmersiveFullscreenMode_ChromiumImpl();
}

// brave/browser/ui/window_feature_controller/window_feature_controller.cc
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"

std::optional<bool> BraveUsesImmersiveFullscreenMode(bool disabled_at_startup,
                                                     Browser* browser) {
  if (!disabled_at_startup &&
      tabs::utils::ShouldUseImmersiveFullscreen(browser)) {
    return true;
  }
  return std::nullopt;
}
```

```gn
# ✅ CORRECT - a Brave target owns the include and dependency wiring
# brave/browser/ui/window_feature_controller/BUILD.gn
source_set("chromium_impl") {
  sources = [
    "window_feature_controller.cc",
  ]
  deps = [
    "//brave/browser/ui/views/tabs",
  ]
}

# brave/browser/BUILD.gn
source_set("core") {
  ...
  deps += [
    "//brave/browser/ui/window_feature_controller:chromium_impl",
  ]
}
```

---

<a id="CSRC-020"></a>

## ✅ Add Comments for Non-Obvious nullptr Assignments in chromium_src

**Add comments explaining non-obvious nullptr assignments in chromium_src
overrides,** especially for dangling pointer prevention. Since chromium_src code
is out of context from the original file, the intent is not always clear.

```cpp
// ❌ WRONG - unclear why setting to nullptr
provider_ = nullptr;

// ✅ CORRECT - explains the intent
// Reset to prevent dangling pointer after BrowserContext shutdown.
provider_ = nullptr;
```

---

<a id="CSRC-021"></a>

## ✅ Use `static_assert` to Protect Against Upstream Enum Changes

**When adding custom values after an upstream enum's last value, add a
`static_assert`** to detect if upstream changes their enum count. This prevents
value clashes when upstream adds new values.

```cpp
// ✅ CORRECT - protected against upstream changes
constexpr int kBravePolicySource = 10;
static_assert(static_cast<int>(policy::POLICY_SOURCE_COUNT) <= kBravePolicySource,
              "Upstream added new policy sources - update kBravePolicySource");
```

---

<a id="CSRC-022"></a>

## ✅ Use `runtime_enabled_features.override.json5` for Blink Features

**When adding Brave-specific Blink runtime-enabled features, add them to
`runtime_enabled_features.override.json5` instead of patching
`runtime_enabled_features.json5`.** The override file is designed for downstream
additions and never changes upstream, preventing patch conflicts.

```json5
// ❌ WRONG - patching runtime_enabled_features.json5 (causes conflicts)

// ✅ CORRECT - add to override file
// third_party/blink/renderer/platform/runtime_enabled_features.override.json5
{
  name: "BraveGlobalPrivacyControl",
  public: true,
  base_feature: "none",
},
```

---

<a id="CSRC-023"></a>

## ❌ `chromium_src` Overrides Cannot Handle Private Methods in `.cc` Files

**The `chromium_src` override mechanism only works for symbols with external
linkage.** Private (static or anonymous-namespace) methods in upstream `.cc`
files cannot be overridden via `chromium_src` because they are not visible
outside the translation unit. Use a direct `.patch` file for these cases
instead.

```cpp
// Upstream file: chrome/browser/feature.cc
namespace {
void PrivateHelper() { ... }  // Cannot override via chromium_src
}

// ✅ Use a .patch file to modify PrivateHelper directly
```

---

<a id="CSRC-024"></a>

## ⚠️ Override Replacement Strings Must Have Compatible Placeholders

**When creating a Brave replacement string that is substituted for an upstream
string via `#undef`/`#define`, the replacement string must have compatible
placeholders.** The upstream code calling the string supplies specific
placeholder values (`$1`, `$2`, etc.). If your replacement string has fewer
placeholders, extras are silently ignored. If it has more or different
placeholders, it can cause runtime errors. Always check what placeholders the
upstream caller provides and match them.

---

<a id="CSRC-025"></a>

## ✅ Guard Overridden String IDs with `#ifndef`/`#error`

**When overriding an upstream string ID via `#undef`/`#define`, add a
compile-time guard to detect if the upstream ID is removed or renamed during a
rebase.** The `check_chromium_src.py` tool verifies symbols are still used in
the overridden file, but a `#ifndef`/`#error` guard provides an additional
safety net.

```cpp
// ✅ CORRECT - compile-time guard catches upstream removals
#ifndef IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS
#error "IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS is not defined"
#endif
#undef IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS
#define IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS \
  IDS_BRAVE_PASSWORD_MANAGER_UI_EMPTY_STATE
```

---

<a id="CSRC-026"></a>

## ✅ Fallback Methods Must Call the Correct Base Class

**When overriding a chromium_src fallback method, ensure the fallback calls the
correct base class method.** A common bug is calling the wrong superclass method
(e.g., calling `ChromeTypographyProvider::Method()` when the override class uses
a `#define` rename that changes the class hierarchy).

```cpp
// ❌ WRONG - calls wrong base class (should be ChromiumImpl, not original)
SkColor BraveProvider::GetColor(int id) {
  return ChromeTypographyProvider::GetColor(id);  // wrong!
}

// ✅ CORRECT - calls the right base class
SkColor BraveProvider::GetColor(int id) {
  return ChromiumImpl::GetColor(id);  // correct fallback
}
```
