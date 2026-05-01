# C++ Coding Standards

<!-- See also: coding-standards.md, coding-standards-memory.md, coding-standards-apis.md -->

<a id="CS-001"></a>

## ✅ Always Include What You Use (IWYU)

**Always include the headers for types you directly use.** Don't rely on transitive includes from other headers - they can change at any time and break your code.

```cpp
// ❌ WRONG - relying on transitive includes
#include "base/memory/ref_counted.h"
// Uses std::string but doesn't include <string>

// ✅ CORRECT - include what you use
#include <string>
#include "base/memory/ref_counted.h"
```

Also remove includes you don't actually use. Double-check all includes in new files.

**For type aliases:** include the header that declares the type alias, not the headers for the underlying types. Same principle as class inheritance — if class B inherits from A, include B's header, not A's.

---

<a id="CS-002"></a>

## Naming Conventions

<a id="CS-003"></a>

### ✅ Use Positive Form for Booleans and Methods

**Always use the positive form ("Enabled" not "Disabled") for readability and consistency.**

```cpp
// ❌ WRONG - negative form is confusing
bool IsTorDisabled();
pref: kTorDisabled

// ✅ CORRECT - positive form
bool IsTorEnabled();
pref: kTorEnabled
```

<a id="CS-004"></a>

### ✅ Consistent Naming Across Layers

**Use the same name for a concept everywhere - C++, JS, prefs, and UI.** Don't arbitrarily use different names in different places for the same thing.

```cpp
// ❌ WRONG - different names for same concept
C++: IsTorDisabledManaged()
JS:  getTorManaged()

// ✅ CORRECT - consistent naming
C++: IsTorManaged()
JS:  isTorManaged()
```

<a id="CS-005"></a>

### ✅ Use Conventional Method Prefixes

- `Should*` methods for queries (not `Get*` for bool queries)
- `Record*` for histogram/P3A recording
- `Load*`/`Save*` pairs for persistence

```cpp
// ❌ WRONG
bool GetShouldShowBrandedWallpaper();
void SendSavingsDaily();

// ✅ CORRECT
bool ShouldShowBrandedWallpaper();
void RecordSavingsDaily();
void LoadSavingsDaily();
void SaveSavingsDaily();
```

<a id="CS-006"></a>

### ✅ Grammatical Correctness

```cpp
// ❌ WRONG
bool IsBraveCommandIds(int id);  // "Ids" is not grammatically correct

// ✅ CORRECT
bool IsBraveCommandId(int id);
```

---

<a id="CS-007"></a>

## ✅ Naming: Only Use `Brave*` Prefix When Overriding Chromium

**Only add the `Brave*` prefix to class names when overriding or subclassing Chromium classes.** For purely Brave-originated code, use the feature name directly.

```cpp
// ❌ WRONG - Brave prefix on a new Brave-only class
class BraveWebcompatReporterService { ... };

// ✅ CORRECT - no prefix needed for Brave-only code
class WebcompatReporterService { ... };

// ✅ CORRECT - Brave prefix when overriding Chromium
class BraveOmniboxClientImpl : public ChromeOmniboxClient { ... };
```

Also: **filename should match the class name.** `WebcompatReporterService` -> `webcompat_reporter_service.h`.

---

<a id="CS-008"></a>

## ✅ Naming: `Maybe*` for Conditional Actions

**Use `Maybe*` prefix for functions that conditionally perform an action.**

```cpp
// ❌ WRONG
void ShowFirstLaunchNotification();  // always sounds like it shows

// ✅ CORRECT
void MaybeShowFirstLaunchNotification();  // clear that it may not show
void MaybeHideReferrer();
```

---

<a id="CS-009"></a>

## ✅ C++ Variable Naming - Underscores, Not camelCase

**C++ variables use underscores (snake_case), not camelCase.** camelCase is only for class names and method names.

```cpp
// ❌ WRONG
bool isTorDisabled = false;
std::string userName;

// ✅ CORRECT
bool is_tor_disabled = false;
std::string user_name;
```

---

<a id="CS-010"></a>

## Lint and Style

- **Opening brace** goes at the end of the previous line (K&R style)
- **Continuation lines** should be indented 4 spaces
- **No `{}` when not required** in C++ (e.g., single-line if/for bodies) — **Note:** This is a Brave-specific deviation from upstream Chromium, which requires `{}` braces on all conditionals and loops. Follow Brave convention.
- **Do NOT enforce include order** — include ordering is handled by code formatting tools and lint, not by code review

---

<a id="CS-011"></a>

## ✅ Copyright Rules

**Never copy a Chromium file and use Brave's copyright.** If you copy or derive from Chromium code, you must include their copyright notice. The original year should remain unchanged - don't bump existing copyright years.

---

<a id="CS-012"></a>

## ✅ Copyright Year in New Files Must Be Current Year

**New files must use the current year in the copyright header.** Always determine the current year from the system date (e.g., `date +%Y`), never from training data or memory — the training cutoff year is often outdated. Don't copy-paste old copyright years from other files.

---

<a id="CS-013"></a>

## ❌ Don't Define Methods in Headers

**Move method definitions to .cc files.** Headers should only contain declarations. Keep headers minimal - only include what's strictly required for the declarations.

```cpp
// ❌ WRONG - method body in header
class RewardsProtocolHandler {
  static bool HandleURL(const GURL& url) {
    return url.scheme() == "rewards";
  }
};

// ✅ CORRECT - declaration in header, definition in .cc
// rewards_protocol_handler.h
bool HandleRewardsProtocol(const GURL& url);

// rewards_protocol_handler.cc
bool HandleRewardsProtocol(const GURL& url) {
  return url.scheme() == "rewards";
}
```

Also: `static` has no meaning for free functions in C++ (it's a C holdover). Use anonymous namespaces instead.

---

<a id="CS-014"></a>

## ✅ Use Forward Declarations in Headers, Include in `.cc`

**Headers should use forward declarations instead of `#include` for types only used as pointers or references.** Move the full `#include` to the `.cc` file.

**Exceptions — do NOT suggest forward declarations when:**
- The type is **not directly `#include`d** but comes transitively through a base class header that must be included anyway (e.g., `content::WebUI*` comes through `webui_config.h` or `mojo_web_ui_controller.h` — there's nothing to remove)
- The type is **not actually used** in the file — verify the type appears in the file before suggesting a forward declaration
- The `#include` is for a **base class** the current class inherits from — these cannot be forward-declared

```cpp
// ❌ WRONG - full include in header for pointer-only usage
// my_class.h
#include "components/foo/bar.h"

// ✅ CORRECT - forward declare in header, include in .cc
// my_class.h
namespace foo { class Bar; }
// my_class.cc
#include "components/foo/bar.h"

// ✅ OK - no change needed, content::WebUI* comes transitively
//    through base class headers that must be included anyway
#include "content/public/browser/webui_config.h"       // base class
#include "ui/webui/mojo_web_ui_controller.h"            // base class
// These headers transitively provide content::WebUI* —
// adding a forward declaration gains nothing.
```

---

<a id="CS-015"></a>

## ✅ `friend` Declarations Go Right After `private:`

**In class declarations, `friend` statements should be placed immediately after the `private:` access specifier,** before any member variables or methods.

```cpp
// ❌ WRONG
class MyClass {
 private:
  int value_ = 0;
  friend class MyClassTest;  // buried among members
};

// ✅ CORRECT
class MyClass {
 private:
  friend class MyClassTest;

  int value_ = 0;
};
```

---

<a id="CS-016"></a>

## ✅ Use Anonymous Namespaces for Internal Code

**If a function or class is strictly internal to a .cc file, put it in an anonymous namespace.**

```cpp
// ❌ WRONG - internal helper visible outside file
static void InternalHelper() { ... }

// ✅ CORRECT - anonymous namespace
namespace {
void InternalHelper() { ... }
}  // namespace
```

**No `static` on `constexpr` inside anonymous namespaces** — the namespace already provides internal linkage, so `static` is redundant.

```cpp
// ❌ WRONG - redundant static
namespace {
static constexpr int kMaxRetries = 3;
}

// ✅ CORRECT
namespace {
constexpr int kMaxRetries = 3;
}
```

---

<a id="CS-017"></a>

## ✅ Function Ordering in `.cc` Should Match `.h`

**Function definitions in `.cc` files should appear in the same order as their declarations in the corresponding `.h` file.**

---

<a id="CS-018"></a>

## ✅ Break Up Bloated Files

**Don't keep dumping code into already-large files.** Encapsulate related functionality into smaller, focused helper files and targets. This improves readability, reduces rebase conflicts, and makes dependency tracking easier.

```cpp
// ❌ WRONG - adding more P3A code to 5000-line RewardsServiceImpl
void RewardsServiceImpl::RecordP3AMetric1() { ... }
void RewardsServiceImpl::RecordP3AMetric2() { ... }

// ✅ CORRECT - separate helper file
// brave/browser/p3a/p3a_core_metrics.h
void RecordRewardsP3A(RewardsService* service);
```

---

<a id="CS-019"></a>

## ✅ Comments Must Make Sense to Future Readers of the Codebase

**Every comment should be meaningful to someone reading the code for the first time, with no knowledge of the PR or change history.** Do not add comments that reference removed code, prior behavior, or the change itself. Comments are part of the codebase, not a changelog.

```cpp
// ❌ WRONG - references removed code / change history
// Removed the old caching logic that was causing race conditions.
// Previously this used a raw pointer, now using unique_ptr.
// Changed from std::map to base::flat_map per review feedback.

// ❌ WRONG - describes what was removed rather than what exists
// The timeout parameter was removed since it's no longer needed.
int ProcessRequest(const GURL& url);

// ✅ CORRECT - describes the code as it is now
// Processes the request synchronously. Returns the HTTP status code.
int ProcessRequest(const GURL& url);

// ✅ CORRECT - explains current behavior, not history
// Uses base::flat_map for better cache locality with small key sets.
base::flat_map<std::string, int> lookup_;
```

---

<a id="CS-021"></a>

## ✅ Document Non-Obvious Failure Branches

**When a function has multiple early-return failure branches, add a brief comment before each summarizing what it handles.**

```cpp
// ❌ WRONG - unclear what each branch handles
if (!parent_hash) return std::nullopt;
if (!state_root) return std::nullopt;
if (!number) return std::nullopt;

// ✅ CORRECT
// Parent block hash is required for chain continuity.
if (!parent_hash) return std::nullopt;
// State root validates the block's state trie.
if (!state_root) return std::nullopt;
// Block number must be present and valid.
if (!number) return std::nullopt;
```

---

<a id="CS-022"></a>

## ✅ Feature Flag Comments Go in `.cc` Files

**Comments explaining what a `base::Feature` does should be placed in the `.cc` file where the feature is defined, not in the `.h` file.**

```cpp
// ❌ WRONG - feature comment in .h
// my_features.h
// Enables the new tab workaround for flash prevention.
BASE_DECLARE_FEATURE(kBraveWorkaroundNewWindowFlash);

// ✅ CORRECT - feature comment in .cc
// my_features.cc
// Enables the new tab workaround for flash prevention.
BASE_FEATURE(kBraveWorkaroundNewWindowFlash, ...);
```

---

<a id="CS-023"></a>

## ✅ Document Upstream Workarounds with Issue Links

**When adding a workaround for an upstream Chromium bug:**
1. Add a link to the upstream issue in a code comment
2. File details on the upstream issue explaining what's happening so they can fix it

This allows us to remove the workaround when the upstream fix lands.

```cpp
// ✅ CORRECT
// Workaround for https://crbug.com/123456 - upstream doesn't handle
// the case where X is null. Remove when the upstream fix lands.
if (!x) return;
```

---

<a id="CS-024"></a>

## ❌ Don't Use Positional Terms in Code Comments

**Do not use "above" or "below" in comments to reference other code.** Other developers may insert code between the comment and referenced item, breaking the meaning. Reference items explicitly by name or identifier instead.

```cpp
// ❌ WRONG - fragile positional reference
// Same root cause as the test above
-BrowserTest.SomeOtherTest

// ✅ CORRECT - explicit reference by name
// Same root cause as BrowserTest.FirstTest (kPromptForDownload override)
-BrowserTest.SomeOtherTest
```

---

<a id="CS-025"></a>

## ✅ Use CHECK for Impossible Conditions

**Use `CHECK` (not `DCHECK`) for conditions that should never happen in any build.**

```cpp
// ❌ WRONG - DCHECK for something that should never happen
DCHECK(browser_context);

// ✅ CORRECT - CHECK for impossible conditions
CHECK(browser_context);  // should never be null
```

Also: don't add unnecessary DCHECKs. For example, `DCHECK(g_browser_process)` is unnecessary because the browser wouldn't even be running without it.

---

<a id="CS-026"></a>

## ✅ `NOTREACHED`/`CHECK(false)` Only for Security-Critical Invariants

**`NOTREACHED`/`CHECK(false)` should only crash the browser for security-critical invariants.** For non-security cases (like invalid enum values from data processing), prefer returning `std::optional`/`std::nullopt` or a default value.

**Important:** `NOTREACHED()` is now fatal in all builds and terminates control flow. The compiler treats code after `NOTREACHED()` as dead code. Do not place executable statements after it.

**Gradual migration:** When migrating from `DCHECK` to fatal `NOTREACHED()`/`CHECK()`, you can use `NOTREACHED(base::NotFatalUntil::M140)` or `CHECK(cond, base::NotFatalUntil::M140)` to gather crash diagnostics before enforcing fatality. See [Chromium style guide](https://chromium.googlesource.com/chromium/src/+/HEAD/styleguide/c++/c++.md).

```cpp
// ❌ WRONG - crashes browser for non-security enum mismatch
mojom::AdType ToMojomAdType(const std::string& type) {
  // ...
  NOTREACHED();  // This isn't a security issue!
}

// ✅ CORRECT - return optional for non-security case
std::optional<mojom::AdType> ToMojomAdType(const std::string& type) {
  // ...
  return std::nullopt;  // Caller handles gracefully
}
```

---

<a id="CS-027"></a>

## ✅ Use `CHECK` Only for Invariants Within Code's Control

**Use `CHECK` only for conditions fully within the code's control.** For data from databases, user input, or external sources, use graceful error handling instead. `CHECK` failures crash the user's browser.

```cpp
// ❌ WRONG - crashes on external data
CHECK(db_value.has_value());  // Data from database!

// ✅ CORRECT - graceful handling of external data
if (!db_value.has_value()) {
  DLOG(ERROR) << "Missing expected database value";
  return std::nullopt;
}
```

See also: [Chromium CHECK style guide](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/styleguide/c++/checks.md)

---

<a id="CS-028"></a>

## ✅ Use Result Codes, Not bool, for Error Reporting

**Return result codes (enums) instead of `bool` for operations that can fail.** This allows providing additional error information and is more future-proof.

---

<a id="CS-029"></a>

## ✅ Don't Store Error State - Handle/Log and Store Only Success

**When a field can hold either a success or error, handle/log the error immediately and store only the success type.**

```cpp
// ❌ WRONG - storing error variant
base::expected<ChainMetadata, std::string> chain_metadata_;

// ✅ CORRECT - handle error at failure point, store only success
std::optional<ChainMetadata> chain_metadata_;
```

---

<a id="CS-030"></a>

## ❌ Don't Override Empty/No-Op Methods

**If you're overriding a virtual method but not implementing any behavior, don't define it at all.**

```cpp
// ❌ WRONG - pointless override
void OnSomethingHappened() override {}

// ✅ CORRECT - just don't override it
```

---

<a id="CS-031"></a>

## ✅ Combine Methods That Are Always Called Together

**If two methods are always called in sequence (especially in patches), combine them into a single method.** This reduces patch size and prevents callers from forgetting one of the calls.

```cpp
// ❌ WRONG - two methods always called together in a patch
+SignBinaries(params);
+CopyPreSignedBinaries(params);

// ✅ CORRECT - single combined method
+PrepareBinaries(params);  // internally calls both
```

---

<a id="CS-032"></a>

## ✅ Use Observer Pattern for UI Updates

**Don't make service-layer queries to update UI directly.** Instead, trigger observer notifications and let the UI respond. See also [ARCH-021](architecture.md#ARCH-021) for when to use observers vs. callbacks.

```cpp
// ❌ WRONG - service making UI queries
void RewardsService::SavePendingContribution(...) {
  SaveToDB(...);
  GetPendingContributionsTotal();  // updating UI from service
}

// ✅ CORRECT - observer pattern
void RewardsService::SavePendingContribution(...) {
  SaveToDB(...);
  for (auto& observer : observers_)
    observer.OnPendingContributionSaved();
}
// UI layer calls GetPendingContributionsTotal in its observer method
```

---

<a id="CS-033"></a>

## ✅ Platform-Specific Code Splitting

**When a method's implementation is completely different on a platform, split it into a separate file** like `my_class_android.cc` rather than filling the main file with `#if defined(OS_ANDROID)` blocks.

---

<a id="CS-034"></a>

## ✅ Use Feature Checks Over Platform Checks

**Prefer feature checks over platform checks when the behavior is feature-dependent, not platform-dependent.**

```cpp
// ❌ WRONG - platform check for feature behavior
#if defined(OS_ANDROID)
  // Don't show notifications
#endif

// ✅ CORRECT - feature check
if (IsDoNotDisturbEnabled()) {
  // Don't show notifications
}
```

---

<a id="CS-035"></a>

## ✅ Consolidate Feature Flag Checks to Entry Points

**Don't scatter `CHECK`/`DCHECK` for feature flag status in private helper functions.** Follow the upstream pattern: check at entry points only. Add comments on private helpers like "Only called when X is enabled".

**Exception:** Public API functions that can be called independently from multiple callsites should keep their own `CHECK`/`DCHECK` guards — they are entry points themselves.

```cpp
// ❌ WRONG - CHECK in private helper called from a single entry point
void TabStripModel::SetCustomTitle(...) {
  CHECK(base::FeatureList::IsEnabled(kRenamingTabs));  // redundant
}

// ✅ CORRECT - check at entry point, comment private helpers
void OnTabContextMenuAction(int action) {
  if (!base::FeatureList::IsEnabled(kRenamingTabs)) return;
  model->SetCustomTitle(...);  // Only called when kRenamingTabs enabled
}

// ✅ ALSO CORRECT - public function keeps its own CHECK
// Public API: callable from any context, not just one entry point
bool StoragePartitionUtils::IsContainersStoragePartition(...) {
  CHECK(base::FeatureList::IsEnabled(features::kContainers));
  ...
}
```

---

<a id="CS-036"></a>

## ❌ Don't Use Static Variables for Per-Profile Settings

**Never use static variables to store per-profile settings.** Static state is shared across all profiles and will cause incorrect behavior in multi-profile scenarios. Use `UserData` or profile-attached keyed services instead.

---

<a id="CS-037"></a>

## ❌ Don't Use Environment Variables for Configuration

**Configuration should come from GN args, not environment variables.** For runtime overrides, use command line switches.

```cpp
// ❌ WRONG
std::string api_url = std::getenv("BRAVE_API_URL");

// ✅ CORRECT - GN arg with command line override option
// In BUILD.gn: defines += [ "BRAVE_API_URL=\"$brave_api_url\"" ]
```

---

---

<a id="CS-040"></a>

## ❌ Don't Duplicate Enum/Constant Values Across Languages

**When values are defined in Mojo, use the generated bindings in C++, Java, and JS.** Don't manually duplicate constants - they easily drift out of sync.

---

<a id="CS-041"></a>

## ❌ Don't Use rapidjson

**Use base::JSONReader/JSONWriter, not rapidjson.** The base libraries are the standard in Chromium.

---

<a id="CS-042"></a>

## VLOG Macros Handle Their Own Checks

**Don't use `VLOG_IS_ON` before `VLOG` calls.** The VLOG macro already handles the level check internally and is smart enough to avoid evaluating inline expressions when the level is disabled.

```cpp
// ❌ WRONG - unnecessary check
if (VLOG_IS_ON(2)) {
  VLOG(2) << "Some message";
}

// ✅ CORRECT - VLOG handles it
VLOG(2) << "Some message";
```

Also: be judicious with VLOG - make sure each log statement has a specific purpose and isn't leftover from debugging.

---

<a id="CS-043"></a>

## ✅ VLOG Component Name Should Match Directory

**The component name used in VLOG messages should match the component directory name** (e.g., `policy` or `brave/components/brave_policy`).

---

<a id="CS-044"></a>

## ✅ Use `LOG(WARNING)` or `VLOG` Instead of `LOG(ERROR)` for Non-Critical Failures

**`LOG(ERROR)` should be reserved for truly unexpected and serious failures.** For expected or non-critical failure cases (e.g., a bad user-supplied filter list, a failed parse of optional data), use `VLOG` for debug info or `LOG(WARNING)` for noteworthy but non-critical issues.

**Do not suggest lowering log severity for failures you haven't confirmed are non-critical.** The developer knows whether a failure is critical for their service better than a reviewer can infer from the code alone. For example, failing to load a model file may look non-critical in isolation but could be a critical failure for the keyed service that depends on it.

```cpp
// ❌ WRONG
LOG(ERROR) << "Failed to parse filter list";

// ✅ CORRECT
VLOG(1) << "Failed to parse filter list";
```

---

<a id="CS-045"></a>

## ✅ Use `DLOG(ERROR)` for Non-Critical Debug-Only Errors

**Use `DLOG(ERROR)` instead of `LOG(ERROR)` for error conditions that are not critical in release builds.** This avoids polluting release build logs with non-actionable errors.

```cpp
// ❌ WRONG - release log noise for non-critical error
LOG(ERROR) << "Failed to parse optional field";

// ✅ CORRECT - debug-only logging
DLOG(ERROR) << "Failed to parse optional field";
```

---

<a id="CS-046"></a>

## ✅ Add `SCOPED_UMA_HISTOGRAM_TIMER` for Performance-Sensitive Paths

**When writing code that processes data on the UI thread or performs potentially slow operations, add `SCOPED_UMA_HISTOGRAM_TIMER` to measure performance.**

```cpp
void GetUrlCosmeticResourcesOnUI(const GURL& url) {
  SCOPED_UMA_HISTOGRAM_TIMER(
      "Brave.CosmeticFilters.GetUrlCosmeticResourcesOnUI");
  // ... potentially slow work ...
}
```

---

<a id="CS-047"></a>

## ✅ Emit Histograms from a Single Location

**When recording UMA histograms, emit to each histogram from a single location.** Create a helper function rather than duplicating histogram emission across multiple call sites.

```cpp
// ❌ WRONG - histogram emitted from multiple places
void OnButtonClicked() {
  base::UmaHistogramExactLinear("Brave.NTP.CustomizeUsage", 2, 7);
}

// ✅ CORRECT - single emission point via helper
void RecordNTPCustomizeUsage(NTPCustomizeUsage usage) {
  base::UmaHistogramExactLinear("Brave.NTP.CustomizeUsage",
                                static_cast<int>(usage),
                                static_cast<int>(NTPCustomizeUsage::kSize));
}
```

---

<a id="CS-048"></a>

## ❌ Don't Log Sensitive Information

**Never log sensitive data such as sync seeds, private keys, tokens, or credentials.** Even VLOG-level logging can expose data in debug builds.

```cpp
// ❌ WRONG
VLOG(1) << "Sync seed: " << sync_seed;

// ✅ CORRECT
VLOG(1) << "Sync seed set successfully";
```

---

<a id="CS-049"></a>

## ❌ Don't Use `public:` in Structs

**Do not use `public:` labels in struct declarations since struct members are public by default.** Either remove the label or change `struct` to `class` if access control is intended.

```cpp
// ❌ WRONG - redundant public label
struct TestData {
 public:
  std::string name;
  int value;
};

// ✅ CORRECT - struct is public by default
struct TestData {
  std::string name;
  int value;
};
```

---

<a id="CS-050"></a>

## ✅ Prefer `GlobalFeatures` Over `NoDestructor` for Global Services

**For global/singleton services, prefer registering in `GlobalFeatures` (the Chromium replacement for `BrowserProcessImpl`) over `base::NoDestructor`.** `NoDestructor` makes testing difficult since you can't reset the instance between tests.

```cpp
// ❌ WRONG - hard to test
BraveOriginService* BraveOriginService::GetInstance() {
  static base::NoDestructor<BraveOriginService> instance;
  return instance.get();
}

// ✅ CORRECT - register in GlobalFeatures for testability
// Access via g_browser_process or dependency injection
```

---

<a id="CS-051"></a>

## ✅ Multiply Before Dividing in Integer Percentage Calculations

**When computing percentages with integer arithmetic, multiply by 100 before dividing.** `(used * 100) / total` preserves precision, while `(used / total) * 100` truncates to 0 when `used < total`.

```cpp
// ❌ WRONG - truncates to 0 for used < total
int pct = (used / total) * 100;

// ✅ CORRECT - preserves precision
int pct = (used * 100) / total;
```

---

<a id="CS-052"></a>

## ✅ Prefer Free Functions Over Complex Inline Lambdas

**When a lambda is complex enough to make surrounding code harder to parse, extract it into a named free function in the anonymous namespace.**

```cpp
// ❌ WRONG - complex lambda obscures call site
DoSomething(base::BindOnce([](int a, int b, int c) {
  // 20 lines of complex logic...
}));

// ✅ CORRECT - named function in anonymous namespace
namespace {
void ProcessResult(int a, int b, int c) {
  // 20 lines of complex logic...
}
}  // namespace
DoSomething(base::BindOnce(&ProcessResult));
```

---

<a id="CS-053"></a>

## ⚠️ Use `auto` Judiciously — Prefer Explicit Types When Short and Descriptive

**Don't use `auto` merely to avoid writing a type name** when the explicit type is short and adds readability. Spell out types like `base::TimeDelta`, `base::Time`, etc. Per Google style guide: "Do not use [auto] merely to avoid the inconvenience of writing an explicit type."

However, `auto` **is appropriate** when the explicit type is verbose/complex and doesn't improve readability (e.g., nested templates, long type names). This is a preference, not a hard rule — always prefix with `nit:` and do not insist if the developer declines.

```cpp
// ❌ WRONG - auto hides a short, descriptive type
auto elapsed = timer.Elapsed();

// ✅ CORRECT - explicit type adds readability
base::TimeDelta elapsed = timer.Elapsed();

// ✅ ALSO CORRECT - auto avoids verbose type that adds no readability
auto weights_opt = base::ReadFileToBytes(weights_path);
// (return type is std::optional<std::vector<uint8_t>>)
```

---

<a id="CS-054"></a>

## ✅ Member Initialization - Don't Add Default When Constructor Always Sets

Per Chromium C++ dos and donts: "Initialize class members in their declarations, **except where a member's value is explicitly set by every constructor**."

```cpp
// ❌ WRONG - misleading, constructor always sets this
class TreeTabNode {
  raw_ptr<TabInterface> current_tab_ = nullptr;  // never actually nullptr
  explicit TreeTabNode(TabInterface* tab);  // always sets current_tab_
};

// ✅ CORRECT - constructor handles initialization
class TreeTabNode {
  raw_ptr<TabInterface> current_tab_;
  explicit TreeTabNode(TabInterface* tab) : current_tab_(tab) {}
};
```

---

<a id="CS-055"></a>

## ✅ Prefer Overloads Over Silently-Ignored Optional Parameters

**Don't force callers to provide parameters that are silently ignored.** Use function overloads. Similarly, prefer overloads over `std::variant` for distinct call patterns.

```cpp
// ❌ WRONG - body_value silently ignored for GET/HEAD
void ApiFetch(const std::string& verb, const std::string& url,
              const base::Value& body_value, Callback cb);

// ✅ CORRECT - separate overloads
void ApiFetch(const std::string& url, Callback cb);  // GET
void ApiFetch(const std::string& url, const base::Value& body, Callback cb);  // POST
```

---

<a id="CS-056"></a>

## ✅ Use `observers_.Notify()` Instead of Manual Iteration

**Use `observers_.Notify(&Observer::Method)` instead of manually iterating observer lists.**

```cpp
// ❌ WRONG - manual iteration
for (auto& observer : observers_) {
  observer.OnPoliciesChanged();
}

// ✅ CORRECT - use Notify helper
observers_.Notify(&Observer::OnPoliciesChanged);
```

---

<a id="CS-057"></a>

## ✅ Validate and Sanitize Data Before Injecting as JavaScript

**When constructing JavaScript from C++ data for injection, use JSON serialization (`base::JSONWriter`) for safe encoding.** String concatenation can lead to injection vulnerabilities.

```cpp
// ❌ WRONG - string concatenation
std::string script = "const selectors = [`" + selector + "`];";

// ✅ CORRECT - JSON serialization
std::string json_selectors;
base::JSONWriter::Write(selectors_list, &json_selectors);
std::string script = "const selectors = " + json_selectors + ";";
```

---

<a id="CS-058"></a>

## ✅ Use `EvalJs` Instead of Deprecated `ExecuteScriptAndExtract*`

**In browser tests, use `EvalJs` and `ExecJs` instead of the deprecated `ExecuteScriptAndExtractBool/String/Int` functions.**

```cpp
// ❌ WRONG
bool result;
ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
    web_contents, "domAutomationController.send(someCheck())", &result));

// ✅ CORRECT
EXPECT_EQ(true, content::EvalJs(web_contents, "someCheck()"));
```

---

<a id="CS-059"></a>

## ✅ Use `Profile::FromBrowserContext` for Conversion

**When you have a `BrowserContext*` and need a `Profile*`, use `Profile::FromBrowserContext()`.** Don't use `static_cast` - the proper method includes safety checks.

```cpp
// ❌ WRONG
Profile* profile = static_cast<Profile*>(browser_context);

// ✅ CORRECT
Profile* profile = Profile::FromBrowserContext(browser_context);
```

---

<a id="CS-060"></a>

## ✅ Validate URLs with `GURL::is_valid()` Before Use

**Always check `GURL::is_valid()` before using a constructed URL.** Passing invalid or malformed URLs to network functions can cause crashes or unexpected behavior.

```cpp
// ❌ WRONG - no validity check
GURL url(user_input);
loader->DownloadUrl(url);

// ✅ CORRECT - validate first
GURL url(user_input);
if (!url.is_valid())
  return;
loader->DownloadUrl(url);
```

---

<a id="CS-061"></a>

## ✅ Name Death Test Suites with `*DeathTest` Suffix

**Per GoogleTest conventions, death test suite names must end with `DeathTest`.** GoogleTest uses this suffix to apply special threading behavior needed for death tests to work reliably.

```cpp
// ❌ WRONG - missing DeathTest suffix
using MyFeatureTest = testing::Test;
TEST_F(MyFeatureTest, CrashesOnNull) {
  EXPECT_DEATH(Process(nullptr), "");
}

// ✅ CORRECT - DeathTest suffix
using MyFeatureDeathTest = testing::Test;
TEST_F(MyFeatureDeathTest, CrashesOnNull) {
  EXPECT_DEATH(Process(nullptr), "");
}
```

---

---

<a id="CS-063"></a>

## ✅ Use `constexpr` for Compile-Time Constant `base::TimeDelta` Values

**Declare time constants as `constexpr base::TimeDelta` rather than just `const`.** The `base::Milliseconds()`, `base::Seconds()`, and similar factory functions are constexpr-capable, so the value can be computed at compile time.

```cpp
// ❌ WRONG - runtime initialization
const base::TimeDelta kAnimationDuration = base::Milliseconds(200);

// ✅ CORRECT - compile-time constant
constexpr base::TimeDelta kAnimationDuration = base::Milliseconds(200);
```

---

<a id="CS-066"></a>

## ❌ Don't Commit Commented-Out Code

**Remove commented-out code, dead `#include` lines, and development leftovers before merging.** Commented-out code adds noise, confuses future readers, and is never cleaned up. If the code might be needed later, it lives in version control history.

```cpp
// ❌ WRONG - dead code left in
// #include "brave/components/old_feature/old_header.h"
// void OldMethod() { ... }

// ✅ CORRECT - remove entirely before merging
```

---

<a id="CS-065"></a>

## ✅ Migrate User Prefs When Removing or Renaming Features

**When removing a feature, model, or preference key that users may have selected, add a migration step to reset affected prefs to a sensible default.** Otherwise users who had the removed option selected are left with a stale value that may cause errors or confusing behavior.

```cpp
// ❌ WRONG - just delete the model, users with it selected get broken state
// (removed model code)

// ✅ CORRECT - migrate users off the removed model
void MigrateProfilePrefs(PrefService* prefs) {
  const std::string current = prefs->GetString(prefs::kDefaultModelKey);
  // Reset to default if user had a removed model selected
  static constexpr auto kRemovedModels = base::MakeFixedFlatSet<std::string_view>({
      "chat-old-model-1",
      "chat-old-model-2",
  });
  if (kRemovedModels.contains(current)) {
    prefs->ClearPref(prefs::kDefaultModelKey);
  }
}
```

<a id="CS-068"></a>

## ✅ Clean Up All Dead Code When Removing Features

**When removing a feature flag, model, or script, also remove all associated dead code:** unused helper functions, orphaned UI strings from `.grdp` files, related constants, and utility functions that were only referenced by the removed code. Leaving dead code behind creates maintenance burden and confusion.

---

<a id="CS-069"></a>

## ❌ Avoid Multiple Inheritance Beyond Interfaces

**Multiple inheritance is permitted in Chromium but discouraged beyond interface-style patterns.** Prefer composition over deep multiple inheritance hierarchies. If you inherit from multiple concrete classes, explore whether composition achieves the same goal more cleanly. See [Chromium C++ style guide](https://chromium.googlesource.com/chromium/src/+/HEAD/styleguide/c++/c++.md).

---

<a id="CS-067"></a>

## ✅ Use `DVLOG(1)` for Retained Logging, `ScopedCrashKeyString` for Crash Diagnostics

**Per the Chromium style guide, remove all logging before check-in.** When logging must remain (e.g., for rare bug investigation), use `DVLOG(1)` — it avoids release binary bloat and can be enabled via `--v=1` or `--vmodule=mod=1`.

**For crash diagnostics, use `base::debug::ScopedCrashKeyString` instead of logs.** Crash keys attach data to crash reports without polluting logs.

```cpp
// ❌ WRONG - LOG in production for diagnostics
LOG(ERROR) << "Unexpected state: " << state;

// ✅ CORRECT - DVLOG for retained debug logging
DVLOG(1) << "Processing state: " << state;

// ✅ CORRECT - crash key for crash diagnostics
static auto* crash_key = base::debug::AllocateCrashKeyString(
    "my_state", base::debug::CrashKeySize::Size64);
base::debug::ScopedCrashKeyString scoped_key(crash_key, state);
```

See [Chromium C++ style guide](https://chromium.googlesource.com/chromium/src/+/HEAD/styleguide/c++/c++.md).
