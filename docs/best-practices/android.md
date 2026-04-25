# Android Best Practices (Java/Kotlin)

<a id="AND-001"></a>

## ✅ Check `isActivityFinishingOrDestroyed()` Before UI Operations in Async Callbacks

**Always check `isActivityFinishingOrDestroyed()` before performing UI operations (showing dialogs, starting activities, manipulating views) in async callbacks, animation listeners, or lambdas.** Activities can be destroyed between when a callback is scheduled and when it executes.

```java
// ❌ WRONG - no lifecycle check in async callback
private void maybeRequestDefaultBrowser() {
    showDefaultBrowserDialog();
}

// ✅ CORRECT - guard against destroyed activity
private void maybeRequestDefaultBrowser() {
    if (isActivityFinishingOrDestroyed()) return;
    showDefaultBrowserDialog();
}

// ✅ CORRECT - guard in animation callbacks
animator.addListener(new AnimatorListenerAdapter() {
    @Override
    public void onAnimationEnd(Animator animation) {
        if (isActivityFinishingOrDestroyed()) return;
        mSplashContainer.setVisibility(View.GONE);
        showPager();
    }
});
```

---

<a id="AND-002"></a>

## ✅ Check Fragment Attachment Before Async UI Updates

**When async callbacks update UI through a Fragment, verify the fragment is still added and its host Activity is available.** Fragments can be detached or their Activity destroyed while async work is in progress.

```java
// ❌ WRONG - no fragment state checks
void onServiceResult(Result result) {
    updateUI(result);
}

// ✅ CORRECT - verify fragment is still attached
void onServiceResult(Result result) {
    if (!isAdded() || isDetached()) return;
    Activity activity = getActivity();
    if (activity == null || activity.isFinishing()) return;
    updateUI(result);
}
```

This applies to any asynchronous path: service callbacks, `PostTask.postTask()`, Mojo responses, etc.

---

<a id="AND-003"></a>

## ✅ Disable Interactive UI During Async Operations

**Disable buttons, preferences, and other interactive elements while an async operation is in progress to prevent double-clicks.** Re-enable when the callback completes.

```java
// ❌ WRONG - allows double-clicks during async operation
preference.setOnPreferenceClickListener(pref -> {
    accountService.resendConfirmationEmail(callback);
    return true;
});

// ✅ CORRECT - disable during async
preference.setOnPreferenceClickListener(pref -> {
    preference.setEnabled(false);
    accountService.resendConfirmationEmail(result -> {
        preference.setEnabled(true);
        // handle result
    });
    return true;
});
```

---

<a id="AND-004"></a>

## ✅ Apply Null Checks Consistently

**If a member field (e.g., a View reference) is checked for null in some code paths, check it in all code paths that use it.** Inconsistent null checking suggests some paths may crash.

```java
// ❌ WRONG - null check in some places but not others
private void showSplash() {
    if (mSplashContainer != null) {
        mSplashContainer.setVisibility(View.VISIBLE);
    }
}
private void hideSplash() {
    mSplashContainer.setVisibility(View.GONE);  // crash if null!
}

// ✅ CORRECT - consistent null checks
private void hideSplash() {
    if (mSplashContainer != null) {
        mSplashContainer.setVisibility(View.GONE);
    }
}
```

---

<a id="AND-005"></a>

## ✅ Add Null Checks for Services Unavailable in Incognito

**Services accessed through bridges or native code may be null in incognito profiles.** Always add explicit null checks at the point of use, even if upstream logic theoretically handles this.

```cpp
// ❌ WRONG - assumes service is always available
void NTPBackgroundImagesBridge::GetCurrentWallpaperForDisplay() {
  view_counter_service_->GetCurrentWallpaperForDisplay();
}

// ✅ CORRECT - explicit null check
void NTPBackgroundImagesBridge::GetCurrentWallpaperForDisplay() {
  if (!view_counter_service_)
    return;
  view_counter_service_->GetCurrentWallpaperForDisplay();
}
```

---

<a id="AND-006"></a>

## ✅ Use LazyHolder Pattern for Singleton Factories

**Use the LazyHolder idiom for singleton service factories instead of explicit `synchronized` blocks with a lock `Object`.** This is more compact and thread-safe by leveraging Java's class loading guarantees.

```java
// ❌ WRONG - explicit lock-based singleton
public class BraveAccountServiceFactory {
    private static final Object sLock = new Object();
    private static BraveAccountServiceFactory sInstance;

    public static BraveAccountServiceFactory getInstance() {
        synchronized (sLock) {
            if (sInstance == null) {
                sInstance = new BraveAccountServiceFactory();
            }
            return sInstance;
        }
    }
}

// ✅ CORRECT - LazyHolder pattern
public class BraveAccountServiceFactory {
    private static class LazyHolder {
        static final BraveAccountServiceFactory INSTANCE =
                new BraveAccountServiceFactory();
    }

    public static BraveAccountServiceFactory getInstance() {
        return LazyHolder.INSTANCE;
    }
}
```

---

<a id="AND-007"></a>

## ✅ Resolve Theme Colors at Bind Time

**When a custom Preference or view resolves colors from theme attributes, do so at `onBindViewHolder` time (or equivalent), not during construction.** This ensures colors update correctly when the user switches between light and dark themes without the view being recreated.

```java
// ❌ WRONG - resolve color during construction
public class MyPreference extends Preference {
    private final int mTextColor;

    public MyPreference(Context context) {
        super(context);
        mTextColor = resolveThemeColor(context, R.attr.textColor);  // stale if theme changes
    }
}

// ✅ CORRECT - resolve at bind time
public class MyPreference extends Preference {
    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        int textColor = resolveThemeColor(getContext(), R.attr.textColor);
        ((TextView) holder.findViewById(R.id.title)).setTextColor(textColor);
    }
}
```

---

<a id="AND-008"></a>

## ✅ Use `app:isPreferenceVisible="false"` for Conditionally Shown Preferences

**When a preference in XML will be programmatically removed or hidden based on a feature flag, set `app:isPreferenceVisible="false"` in XML to avoid a brief visual flash before the code hides it.**

```xml
<!-- ✅ Prevents flash of preference before programmatic removal -->
<org.chromium.chrome.browser.settings.BraveAccountPreference
    android:key="brave_account"
    app:isPreferenceVisible="false"
    android:title="@string/brave_account_title" />
```

---

<a id="AND-009"></a>

## ✅ Use `assert` Alongside `Log` for Validation

**Pair defensive null/validation checks with `assert` statements.** Assertions crash in debug builds making problems immediately visible, while graceful handling still protects release builds. Log-only guards are easily missed in logcat output.

```java
// ❌ WRONG - log-only guard, easily missed
if (contractAddress == null || contractAddress.length() < MIN_LENGTH) {
    Log.e(TAG, "Invalid contract address");
    return "";
}

// ✅ CORRECT - assert for debug + graceful fallback for release
assert contractAddress != null && contractAddress.length() >= MIN_LENGTH
        : "Invalid contract address";
if (contractAddress == null || contractAddress.length() < MIN_LENGTH) {
    Log.e(TAG, "Invalid contract address");
    return "";
}
```

---

<a id="AND-010"></a>

## ✅ Cache Expensive System Service Lookups

**When a method internally fetches system services (e.g., `PackageManager`, `AppOpsManager`), avoid calling it repeatedly in a hot path.** Compute the value once and store it in a member variable.

**Exception:** Don't cache values that can change without notification in multi-window or configuration-change scenarios (e.g., PiP availability can change when a second app starts).

```java
// ❌ WRONG - repeated expensive service lookup
@Override
public void onResume() {
    if (hasPipPermission()) { /* fetches PackageManager + AppOpsManager */ }
}

// ✅ CORRECT - cache on creation
private boolean mHasPipPermission;

@Override
public void onCreate() {
    mHasPipPermission = hasPipPermission();
}
```

---

<a id="AND-011"></a>

## ✅ Prefer Core/Native-Side Validation

**Before implementing validation logic in Android/Java code, check whether unified validation exists on the core/native side.** Prefer core-side validation to avoid cross-platform inconsistencies between Android, iOS, and desktop.

---

<a id="AND-012"></a>

## ✅ Skip Native/JNI Checks in Robolectric Tests

**Robolectric tests do not have native/JNI available.** When code paths hit JNI calls, use conditional checks (like `FeatureList.isNativeInitialized()`) to gracefully handle the test environment.

```java
// ✅ CORRECT - guard JNI calls for Robolectric compatibility
if (FeatureList.isNativeInitialized()) {
    ChromeFeatureList.isEnabled(BraveFeatureList.SOME_FEATURE);
}
```

See `BraveDynamicColors.java` for an existing example of this pattern.

---

<a id="AND-013"></a>

## ✅ Use Direct Java Patches When Bytecode Patching Fails

**Bytecode (class adapter) patching fails when a class has two constructors.** In these cases, use direct `.java.patch` files instead. Also use direct `BUILD.gn` patches to add sources when circular dependencies prevent using `java_sources.gni`.

---

<a id="AND-014"></a>

## ✅ `ProfileManager.getLastUsedRegularProfile()` Is Acceptable in Widgets

**While the presubmit check flags `ProfileManager.getLastUsedRegularProfile()` as a banned pattern, it is acceptable in Android widget providers** (e.g., `QuickActionSearchAndBookmarkWidgetProvider`) where no Activity or WebContents context is available. This matches upstream Chromium's approach in their own widgets.

---

<a id="AND-015"></a>

## ✅ Remove Unused Interfaces and Dead Code

**Do not leave unused interfaces, listener patterns, or helper methods in the codebase.** If scaffolded during development but never actually called, remove before merging.

```java
// ❌ WRONG - interface defined but never used
public interface OnAnimationCompleteListener {
    void onAnimationComplete();
}

// ✅ CORRECT - remove if nothing implements or calls it
```

---

<a id="AND-016"></a>

## ❌ Don't Set `clickable`/`focusable` on Non-Interactive Views

**Avoid setting `android:clickable="true"` or `android:focusable="true"` on purely decorative or display-only views** (like animation containers). These attributes affect accessibility and touch event handling.

---

<a id="AND-017"></a>

## ✅ Share Identical Assets Across Platforms

**When Android and iOS use identical asset files (e.g., Lottie animation JSON), reference a single shared copy rather than maintaining duplicates.** This ensures future changes only need to be made once.

---

<a id="AND-019"></a>

## ✅ Group Feature-Specific Java Sources into Separate Build Targets

**When Java sources for a specific feature (e.g., `crypto_wallet`) accumulate in `brave_java_sources.gni`, consider creating a separate build target.** This improves build isolation and dependency tracking.

---

<a id="AND-021"></a>

## ✅ Prefer Early Returns Over Deep Nesting

**When a condition check determines whether the rest of a method should execute, return early rather than wrapping logic in nested `if` blocks.** This reduces nesting depth and improves readability.

```java
// ❌ WRONG - deep nesting
private void handleState() {
    if (!isFinished) {
        if (hasData) {
            // ... lots of code ...
        }
    }
}

// ✅ CORRECT - early return
private void handleState() {
    if (isFinished) return;
    if (!hasData) return;
    // ... lots of code at top level ...
}
```

---

<a id="AND-022"></a>

## ✅ Bytecode Adapter Changes Require Bytecode Tests

**When adding or modifying a bytecode class adapter (files in `build/android/bytecode/java/org/brave/bytecode/`), add a corresponding bytecode test** in `android/javatests/org/chromium/chrome/browser/BytecodeTest.java`. Tests should verify both class existence (in `testClassesExist`) and method existence with correct return types (in `testMethodsExist`). This ensures upstream refactors are caught at test time rather than causing silent runtime failures.

---

<a id="AND-023"></a>

## ✅ Proguard Rules: Separate Runtime vs Test Keep Rules

**Proguard keep rules must go in the correct file:**
- `android/java/proguard.flags` — Only for classes/methods accessed via reflection at runtime
- `android/java/apk_for_test.flags` — For rules needed only during testing

Putting test-only keep rules in `proguard.flags` unnecessarily increases the production APK size by preventing code shrinking.

---

<a id="AND-025"></a>

## ✅ Use `@VisibleForTesting` for Package-Private Test Accessors

**When a field or method is made package-private solely for testing purposes, annotate it with `@VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)`.** This communicates intent to other developers and allows IDEs to flag improper usage from non-test code.

---

<a id="AND-026"></a>

## ✅ Prefer Programmatic `addView` Over Full XML Layout Replacement

**When customizing Android UI, prefer adding views programmatically (via `addView` in Java) over replacing entire upstream XML layout files.** Replacing full XML files creates maintenance burden during Chromium upgrades. However, if programmatic addition requires extending final classes or leads to equally invasive changes, document the trade-off and accept the XML replacement.

---

<a id="AND-027"></a>

## ✅ Brave Resources Go in `brave-res`, Not Upstream Folders

**Brave-specific Android resources (drawables, layouts, etc.) should be placed in a dedicated `brave-res` folder, not copied into upstream Chromium resource directories.** The upstream resource directories should only be used when intentionally overriding an existing upstream resource. Adding new Brave-only resources to upstream folders creates confusion about whether a resource is a Brave addition or an upstream override.

---

<a id="AND-028"></a>

## ✅ Comprehensively Clean Up All Artifacts When Removing a Feature

**When removing a feature from Android, audit and remove all related artifacts:** bytecode class adapters, ProGuard rules, bytecode test entries, Java source files, resource files, JNI bindings, feature flags, and build system references. A feature removal PR should account for all integration points across the codebase.

---

<a id="AND-029"></a>

## ✅ Remove Dead API-Level Checks Below Min SDK

**Remove Android version checks for API levels below the app's minimum SDK version.** Dead code checking for Lollipop (API 21) or Marshmallow (API 23) should be cleaned up since Brave's minimum SDK is higher.

---

<a id="AND-031"></a>

## ✅ Direct Patches: Add New Lines, Don't Modify Existing

**When creating direct patches (non-chromium_src overrides), prefer adding entirely new lines rather than modifying existing upstream lines.** This reduces the risk of patch conflicts during Chromium version upgrades, because new lines have no upstream anchor that might change.

---

<a id="AND-032"></a>

## ✅ Patches Are Acceptable for Anonymous Inner Classes

**When the Brave override system cannot handle anonymous inner classes in upstream Chromium Java code, a `.patch` file is the accepted fallback.** Document the reason in the PR and link to the tracking issue for better override support.

---

<a id="AND-033"></a>

## ✅ Verify Shared Resource Changes Don't Break Other UIs

**When modifying shared Android resource values (colors, dimensions, styles in files like `brave_colors.xml`), verify the impact on ALL UIs that reference those resources.** Shared resources can affect multiple screens — always cross-check usages before modifying.

---

<a id="AND-034"></a>

## ✅ Use Baseline Colors Over Java Code Patches for Theming

**When fixing Android color/theming issues, prefer following upstream's approach of using baseline colors (XML color resources) for non-dynamic color states rather than patching Java code to programmatically set colors.** This is more maintainable and aligns with upstream's theming system. Always verify fixes work with the Dynamic Colors flag both enabled and disabled.

---

<a id="AND-035"></a>

## ❌ Don't Modify Upstream String Resource Files

**Never modify upstream Chromium string resource files (e.g., `chrome_strings.grd`, upstream `strings.xml`).** Add Brave-specific strings to `android_brave_strings.grd` or the appropriate Brave-owned resource file instead. Modifying upstream files creates patch conflicts during Chromium upgrades.

---

<a id="AND-036"></a>

## ✅ Match Upstream Nullability Annotations in Overridden Methods

**When overriding upstream methods in Brave Java code, match the upstream nullability annotations (e.g., `@NullUnmarked`, `@Nullable`, `@NonNull`).** Mismatched annotations cause NullAway build failures and prevent merging.

```java
// ❌ WRONG - upstream method has @NullUnmarked but override doesn't
@Override
public void onResult(Profile profile) { ... }

// ✅ CORRECT - match upstream annotations
@NullUnmarked
@Override
public void onResult(Profile profile) { ... }
```

---

<a id="AND-037"></a>

## ❌ Don't Repeat Class Name in Log TAG

**Android Log TAG fields should use a short, descriptive string — not repeat the full class name when it adds no value.** Keep TAGs concise and informative.

```java
// ❌ WRONG - redundant, TAG just repeats class name
private static final String TAG = "BraveVpnProfileController";

// ✅ CORRECT - short and clear
private static final String TAG = "BraveVPN";
```

---

<a id="AND-038"></a>

## ✅ Use Layout Shorthand Attributes

**Use shorthand XML layout attributes (`paddingHorizontal`, `marginHorizontal`, `paddingVertical`, `marginVertical`) instead of setting start/end or top/bottom separately.** This reduces XML verbosity and improves readability.

```xml
<!-- ❌ WRONG - verbose -->
<View
    android:paddingStart="16dp"
    android:paddingEnd="16dp" />

<!-- ✅ CORRECT - shorthand -->
<View
    android:paddingHorizontal="16dp" />
```

---

# NullAway (Java Null Safety)

Chromium uses [NullAway](https://github.com/uber/NullAway) to enforce [JSpecify](https://jspecify.dev/docs/user-guide/)-style `@Nullable` annotations. NullAway is an [Error Prone](https://errorprone.info/) plugin that runs as a static analysis step for targets without `chromium_code = false`. Null checking is enabled only for classes annotated with `@NullMarked`. Migration progress: [crbug.com/389129271](https://crbug.com/389129271).

Key configuration facts:
- JSpecify mode is enabled — `@Nullable` is `TYPE_USE`.
- Non-annotated types default to non-null (no `@NonNull` needed).
- Nullness of local variables is inferred automatically.
- Annotations live under `org.chromium.build.annotations` (part of `//build/android:build_java`, a default dep).
- Java collections and Guava's `Preconditions` are modeled directly in NullAway.
- Android's `onCreate()` (and similar) methods are implicitly `@Initializer`.

---

<a id="AND-039"></a>

## ✅ Place `@Nullable` Immediately Before the Type

**`@Nullable` is `TYPE_USE` in JSpecify mode — it must appear immediately before the type it annotates.** Placing it on a separate line or before modifiers does not compile. For nested types, the annotation goes before the inner type name.

```java
// ❌ WRONG - annotation on separate line
@Nullable
private String mValue;

// ✅ CORRECT - immediately before the type
private @Nullable String mValue;
private Outer.@Nullable Inner mNestedType;
```

For arrays and generics, position matters for what is nullable:

```java
// Nullable array of non-null strings
private String @Nullable[] mNullableArrayOfNonNullString;

// Non-null array of nullable strings
private @Nullable String[] mNonNullArrayOfNullableString;

// Non-null list of nullable strings
private List<@Nullable String> mNonNullListOfNullableString;

// Nullable callback of nullable strings
private @Nullable Callback<@Nullable String> mNullableCallbackOfNullableString;
```

---

<a id="AND-040"></a>

## ✅ Use Method Annotations for Pre/Post Conditions

**NullAway analyzes code per-method. Use `@RequiresNonNull`, `@EnsuresNonNull`, `@EnsuresNonNullIf`, and `@Contract` to communicate nullability conditions across method boundaries.**

```java
// @RequiresNonNull — only makes sense on private methods
@RequiresNonNull("mNullableString")
private void usesNullableString() {
    if (mNullableString.isEmpty()) { ... }  // No warning
}

// @EnsuresNonNull — guarantees field is non-null after call
@EnsuresNonNull("mNullableString")
private void initializeString() {
    assert mNullableString != null;  // Warns if nullable at any exit
}

// @EnsuresNonNullIf — ties nullness to boolean return
@EnsuresNonNullIf("mThing")
private boolean isThingEnabled() {
    return mThing != null;
}

// With multiple fields and negated result
@EnsuresNonNullIf(value={"sThing1", "sThing2"}, result=false)
private static boolean isDestroyed() {
    return sThing1 == null || sThing2 == null;
}

// @Contract — limited forms supported
@Contract("null -> false")
private boolean isParamNonNull(@Nullable String foo) {
    return foo != null;
}

@Contract("_, !null -> !null")
@Nullable String getOrDefault(String key, @Nullable String defaultValue) {
    return defaultValue;
}
```

**Note:** NullAway's validation of `@Contract` correctness is buggy and disabled, but contracts still apply to callers (they are assumed to be true). See [NullAway#1104](https://github.com/uber/NullAway/issues/1104).

---

<a id="AND-041"></a>

## ✅ Use `@MonotonicNonNull` for Late-Initialized Fields

**When a field starts as null but must never be set back to null after initialization, use `@MonotonicNonNull` instead of `@Nullable`.** This allows NullAway to trust the field is non-null after its first assignment, even in lambdas.

```java
private @MonotonicNonNull String mSomeValue;

public void doThing(String value) {
    // Emits a warning — mSomeValue is still nullable here:
    helper(mSomeValue);

    mSomeValue = value;
    // No warning — even in a lambda, NullAway trusts it stays non-null:
    PostTask.postTask(TaskTraits.USER_BLOCKING, () -> helper(mSomeValue));
}
```

---

<a id="AND-042"></a>

## ✅ Choose the Right Assert/Assume Pattern

**Use the correct null assertion mechanism depending on whether you need a statement or expression and what safety guarantees you want.** Always use `import static` for `assumeNonNull` / `assertNonNull`.

```java
import static org.chromium.build.NullUtil.assumeNonNull;
import static org.chromium.build.NullUtil.assertNonNull;

public String example() {
    // Prefer statements for preconditions — keeps them separate from usage
    assumeNonNull(mNullableThing);
    assert mOtherThing != null;

    // Works with nested fields and getters
    assumeNonNull(someObj.nullableField);
    assumeNonNull(someObj.getNullableThing());

    // Use expression form when it improves readability
    someHelper(assumeNonNull(Foo.maybeCreate(true)));

    // Use assertNonNull when you need an assert as an expression
    mNonNullField = assertNonNull(dict.get("key"));

    String ret = obj.getNullableString();
    if (willJustCrashLaterAnyways) {
        // Use "assert" when not locally dereferencing the object
        assert ret != null;
    } else {
        // Use requireNonNull for production safety
        // (asserts are only enabled on Canary as dump-without-crashing)
        Objects.requireNonNull(ret);
    }
    return ret;
}

// Use assertNonNull(null) for unreachable code paths
public String describe(@MyIntDef int validity) {
    return switch (validity) {
        case MyIntDef.VALID -> "okay";
        case MyIntDef.INVALID -> "not okay";
        default -> assertNonNull(null);
    };
}
```

| Mechanism | Form | Crashes in prod? | Use when |
|-----------|------|------------------|----------|
| `assumeNonNull()` | Statement or expression | No | You're confident the value is non-null |
| `assertNonNull()` | Expression | No (Canary only) | You need a non-null expression |
| `assert x != null` | Statement | No (Canary only) | You're not dereferencing locally |
| `Objects.requireNonNull()` | Expression | Yes | Null would cause worse problems later |

---

<a id="AND-043"></a>

## ✅ Handle Object Destruction Correctly

**For classes with `destroy()` methods that null out otherwise non-null fields, choose one of two strategies:**

<a id="AND-053"></a>

### Strategy 1: `@Nullable` fields with `@EnsuresNonNullIf` guards (preferred for complex cases)

```java
private @Nullable SomeService mService;

@EnsuresNonNullIf(value = {"mService"}, result = false)
private boolean isDestroyed() {
    return mService == null;
}

public void doWork() {
    if (isDestroyed()) return;
    mService.execute();  // No warning — NullAway trusts @EnsuresNonNullIf
}

public void destroy() {
    mService = null;
}
```

<a id="AND-054"></a>

### Strategy 2: Suppress warnings on `destroy()` (simpler cases)

```java
private SomeService mService;  // stays non-null

@SuppressWarnings("NullAway")
public void destroy() {
    mService = null;
}
```

---

<a id="AND-044"></a>

## ✅ Use `assertBound()` for View Binders, Not `@Initializer`

**Do not mark `onBindViewHolder()` with `@Initializer` — it is not called immediately after construction.** Instead, add an `assertBound()` helper that uses `@EnsuresNonNull` to verify fields are initialized.

```java
// ❌ WRONG - onBindViewHolder is not a real initializer
@Initializer
@Override
public void onBindViewHolder(ViewHolder holder, int position) {
    mField1 = holder.field1;
    mField2 = holder.field2;
}

// ✅ CORRECT - use assertBound() in methods that need the fields
@EnsuresNonNull({"mField1", "mField2"})
private void assertBound() {
    assert mField1 != null;
    assert mField2 != null;
}

private void updateView() {
    assertBound();
    mField1.setText("hello");  // No warning
}
```

---

<a id="AND-045"></a>

## ✅ Initialize Struct-Like Class Fields via Constructor

**NullAway has no special handling for classes with public fields — it warns on non-primitive, non-`@Nullable` public fields not set by a constructor.** Create a constructor that sets all fields. Use `/* paramName= */` comments if readability suffers.

```java
// ❌ WRONG - NullAway warns about uninitialized public fields
public class TabInfo {
    public String title;
    public String url;
}

// ✅ CORRECT - constructor sets all non-null fields
public class TabInfo {
    public final String title;
    public final String url;

    public TabInfo(String title, String url) {
        this.title = title;
        this.url = url;
    }
}

// At call sites, add comments for clarity:
new TabInfo(/* title= */ "Home", /* url= */ "https://brave.com");
```

---

<a id="AND-046"></a>

## ✅ Use "Checked" Companion Methods for Effectively Non-Null Returns

**Some methods are technically `@Nullable` but practically always non-null (e.g., `Activity.findViewById()`, `Context.getSystemService()`).** For Chromium-authored code, create a "Checked" companion instead of mis-annotating the return type.

```java
// When you're not sure if the tab exists:
public @Nullable Tab getTabById(String tabId) {
    ...
}

// When you know the tab exists:
public Tab getTabByIdChecked(String tabId) {
    return assertNonNull(getTabById(tabId));
}
```

Do not annotate `@Nullable` methods as `@NonNull` just because callers expect non-null — this hides real nullability and defeats the purpose of static analysis.

---

<a id="AND-047"></a>

## ✅ Handle Supplier Nullability Variance

**`Supplier<T>` and `Supplier<@Nullable T>` are not assignment-compatible due to Java generics invariance.** Explicit casts or utilities are required.

```java
// Passing Supplier<T> to Supplier<@Nullable T> — explicit cast required
Supplier<String> nonNullSupplier = () -> "hello";
acceptsNullableSupplier((Supplier<@Nullable String>) nonNullSupplier);

// Passing Supplier<@Nullable T> to Supplier<T> — use SupplierUtils
Supplier<@Nullable String> nullableSupplier = () -> maybeNull();
acceptsNonNullSupplier(SupplierUtils.asNonNull(nullableSupplier));

// If the value might actually be null, change the parameter type instead:
void betterApproach(Supplier<@Nullable String> supplier) {
    String value = assumeNonNull(supplier.get());
}
```

See [NullAway#1356](https://github.com/uber/NullAway/issues/1356) for background on why casts are necessary.

---

<a id="AND-048"></a>

## ✅ Use `@Initializer` for Two-Phase Initialization

**When a class uses two-phase initialization (e.g., `onCreate()`, `initialize()`), annotate the second-phase method with `@Initializer`.** NullAway will then validate non-null fields as if the initializer runs right after the constructor. Android's `onCreate()` is implicitly `@Initializer`.

```java
public class MyComponent {
    private SomeService mService;

    public MyComponent() {
        // mService not set here — that's OK because initialize() is @Initializer
    }

    @Initializer
    public void initialize(SomeService service) {
        mService = service;
    }
}
```

**Caveat:** NullAway does not verify that `@Initializer` methods are actually called. When multiple setters are always called together, prefer a single `initialize()` method.

---

<a id="AND-049"></a>

## ✅ Understand `@SuppressWarnings("NullAway")` vs `@NullUnmarked`

**Both suppress NullAway warnings, but they differ in how callers see the method's signature.**

| Annotation | Method body | Callers see |
|-----------|-------------|-------------|
| `@SuppressWarnings("NullAway")` | Warnings suppressed | Method remains `@NullMarked` — callers get full null checking |
| `@NullUnmarked` | Warnings suppressed | Parameters and return types have **unknown** nullability — callers also lose null checking |

**Prefer `@SuppressWarnings("NullAway")`** when you want to silence a false positive inside a method without degrading the caller experience. Use `@NullUnmarked` only for classes/methods not yet migrated to null safety.

---

<a id="AND-050"></a>

## ❌ Don't Use Intermediate Booleans for Null Checks

**NullAway cannot track nullness through intermediate boolean variables.** Always use the null check directly in the `if` condition.

```java
// ❌ WRONG - NullAway loses track of nullness
boolean isNull = thing == null;
if (!isNull) {
    thing.doWork();  // NullAway still warns!
}

// ✅ CORRECT - direct null check
if (thing != null) {
    thing.doWork();  // No warning
}
```

This is a known NullAway limitation: [NullAway#98](https://github.com/uber/NullAway/issues/98).

---

<a id="AND-051"></a>

## ✅ JNI: `@CalledByNative` Skips Checks, Java-to-Native Is Checked

**Nullness is not checked for `@CalledByNative` methods** ([crbug/389192501](https://crbug.com/389192501)). However, Java-to-Native method calls **are checked** via `assert` statements when `@NullMarked` is present.

Ensure native-bound parameters have correct nullability annotations so that callers on the Java side are properly checked.

---

<a id="AND-052"></a>

## ✅ Use JSpecify Annotations for Mirrored Code

**For code that will be mirrored and built in other environments, use JSpecify annotations directly** instead of Chromium's copies under `org.chromium.build.annotations`. Configure the build target accordingly:

```gn
deps += [ "//third_party/android_deps:org_jspecify_jspecify_java" ]

# Prevent automatic dep on build_java.
chromium_code = false

# Do not let chromium_code = false disable Error Prone.
enable_errorprone = true
```
