# Android Settings Search Index

Android's settings search feature lets users type in the Settings search bar and find any setting
by name or summary. Every concrete `PreferenceFragment` subclass must declare a
`SEARCH_INDEX_DATA_PROVIDER` field and register it in the central registry. A presubmit check
enforcing this was added in Chromium cr146.

This document explains how to implement this for Brave-specific fragments.

## Overview

The indexing pipeline has two phases:

1. **`initPreferenceXml`** — runs once at app startup. Parses the fragment's preference XML,
   creates index entries, and establishes parent–child links so the search results show the correct
   breadcrumb path.

2. **`updateDynamicPreferences`** — runs on every search. Removes or updates entries that depend
   on runtime state (feature flags, user prefs, policies).

Both phases write into a `SettingsIndexData` object. The search UI then queries this data.

---

## Choosing the right pattern

### Pattern 1: `INDEX_OPT_OUT`

Use when the fragment has **no static preferences to index** — e.g. it inflates a custom View
layout instead of a preference XML, or all its content is populated programmatically at runtime.

```java
// No static preferences to index — custom view / dynamic content only.
public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
        new BaseSearchIndexProvider(
                MyFragment.class.getName(), BaseSearchIndexProvider.INDEX_OPT_OUT);
```

Examples: `BraveSyncScreensPreference`, `BraveWalletNetworksPreferenceFragment`,
`BlockedCredentialFragmentView`, `QuickSearchEnginesFragment`.

---

### Pattern 2: No `initPreferenceXml` override (Brave sub-screens in `brave_main_preferences.xml`)

Use when the fragment is reachable from `brave_main_preferences.xml` **and** its entry has a
correct `android:fragment` attribute. `BraveMainPreferencesBase.SEARCH_INDEX_DATA_PROVIDER`
processes `brave_main_preferences.xml` via `PreferenceParser.parseAndPopulate`, which creates the
parent entry. `resolveIndex()` then establishes the child–parent link automatically from the
`android:fragment` value — no `initPreferenceXml` override is needed.

```java
public static final BaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
        new BaseSearchIndexProvider(
                MyFragment.class.getName(), R.xml.my_fragment_preferences) {

            @Override
            public void updateDynamicPreferences(
                    Context context, SettingsIndexData indexData) {
                // Remove prefs that are hidden by feature flags, policies, etc.
                if (!ChromeFeatureList.isEnabled(BraveFeatureList.MY_FEATURE)) {
                    indexData.removeEntryForKey(MainSettings.class.getName(), PREF_MY_KEY);
                    return;
                }
                indexData.removeEntryForKey(MyFragment.class.getName(), "some_widget_key");
            }
        };
```

Examples: `BraveLeoPreferences`, `BravePlaylistPreferences`, `BravePrivacySettings`,
`BraveSearchEnginesPreferences`, `BraveWalletPreferences`, `BraveVpnPreferences`.

---

### Pattern 3: `addChildParentLink`

Use when the child–parent link **cannot** be established automatically from XML. This happens in
two cases:

- The entry in `brave_main_preferences.xml` has **no `android:fragment`** (e.g. `brave_origin`
  uses a runtime click listener instead of fragment navigation).
- The upstream XML references a **different class** than the Brave fragment (e.g. the `passwords`
  entry uses `BravePasswordsPreference` — a custom `Preference` subclass with no
  `android:fragment` — so `resolveIndex()` never finds a link to `PasswordSettings`).

```java
public static final ChromeBaseSearchIndexProvider SEARCH_INDEX_DATA_PROVIDER =
        new ChromeBaseSearchIndexProvider(
                MyBraveFragment.class.getName(), R.xml.my_brave_preferences) {

            @Override
            public void initPreferenceXml(
                    Context context,
                    Profile profile,
                    SettingsIndexData indexData,
                    Map<String, SearchIndexProvider> providerMap) {
                super.initPreferenceXml(context, profile, indexData, providerMap);
                String parentId = PreferenceParser.createUniqueId(
                        MainSettings.class.getName(), PREF_MY_KEY);
                indexData.addChildParentLink(MyBraveFragment.class.getName(), parentId);
            }
        };
```

Use `ChromeBaseSearchIndexProvider` (instead of `BaseSearchIndexProvider`) when you need access to
a `Profile` in `initPreferenceXml` or `updateDynamicPreferences`.

Examples: `BraveOriginPreferences`, `PasswordSettings`.

---

## `updateDynamicPreferences` recipes

### Removing a preference gated by a feature flag

```java
if (!ChromeFeatureList.isEnabled(BraveFeatureList.MY_FEATURE)) {
    indexData.removeEntryForKey(MyFragment.class.getName(), PREF_MY_KEY);
}
```

### Removing the whole screen when a feature is disabled

Remove the **parent entry** — `resolveIndex()` will automatically prune all child entries as
orphans:

```java
if (!ChromeFeatureList.isEnabled(BraveFeatureList.MY_FEATURE)) {
    indexData.removeEntryForKey(MainSettings.class.getName(), PREF_MY_KEY);
    return;
}
```

### Dynamic summary (On/Off state)

```java
boolean enabled = UserPrefs.get(profile).getBoolean(Pref.MY_PREF);
indexData.updateEntrySummaryForKey(
        MyFragment.class.getName(),
        PREF_MY_KEY,
        enabled ? R.string.text_on : R.string.text_off);
```

### Removing a preference with no title (custom widget)

Widgets that use `android:layout` but have no `android:title` appear in the index with an empty
title. Remove them explicitly:

```java
indexData.removeEntryForKey(MyFragment.class.getName(), PREF_MY_WIDGET_KEY);
```

### `TextMessagePreference`

These are automatically excluded from the index — no explicit removal needed.

---

## Registering in the registry

All Brave providers are added to a **single line** in
`chrome/android/java/src/org/chromium/chrome/browser/settings/search/SearchIndexProviderRegistry.java`.
This file lives outside the brave repo and is maintained as a patch, so keeping all Brave entries
on one `+` line minimises upgrade friction.

Add your provider to the end of the existing Brave one-liner (before
`TracingCategoriesSettings`):

```java
// existing Brave entries ..., org.chromium.chrome.browser.settings.MyFragment.SEARCH_INDEX_DATA_PROVIDER,
```

---

## Presubmit check

The check `CheckSettingsChanges` in `src/PRESUBMIT.py` verifies:

1. Every concrete settings fragment defines `SEARCH_INDEX_DATA_PROVIDER`.
2. The provider is registered in `SearchIndexProviderRegistry.java`.
3. UI patterns in **changed lines** (`.setSummary`, `.setVisible`, `.addPreference`, bundle puts/gets)
   are mirrored by corresponding index API calls in the provider body.

| UI trigger | Required in provider body |
|---|---|
| `.setSummary(` | `updateEntrySummaryForKey`, `addEntryForKey`, or `.setSummary` |
| `.setVisible(` / `.removePreference(` | `removeEntry` or `removeEntryForKey` |
| `.addPreference(` | `addEntry`, `addEntryForKey`, `updateEntry`, or `updateEntryForKey` |
| `.put(String\|Int\|...)(` | `getExtras` |
| `getArguments().get` / `.containsKey(` | `getExtras` |

Run locally against specific files:

```bash
npm run presubmit -- --files "path/to/MyFragment.java"
```

Run against multiple files (semicolon-separated):

```bash
npm run presubmit -- --files "path/to/File1.java;path/to/File2.java"
```

---

## Pitfalls

### Javadoc `"This class is responsible"`

The presubmit's fallback class-name regex is `class\s+(\w+)`, which matches anywhere in the file —
including inside javadoc comments. The phrase `"This class is responsible"` produces a false match,
extracting `is` as the class name and causing a spurious "Provider not registered" warning.

**Fix:** reword the javadoc to start with the class name instead:

```java
// Bad — triggers false match:
/** This class is responsible for rendering... */

// Good:
/** FooFragment is responsible for rendering... */
```

### `brave_main_preferences.xml` requires correct `android:fragment`

`BraveMainPreferencesBase.SEARCH_INDEX_DATA_PROVIDER` processes `brave_main_preferences.xml` via
`PreferenceParser.parseAndPopulate`. For each entry with a correct `android:fragment`, `resolveIndex()`
establishes the child–parent link automatically. If `android:fragment` is missing or wrong, the
link is never established and the fragment's entries are pruned as orphans — use `addChildParentLink`
in `initPreferenceXml` to register the link manually (Pattern 3).

### Wrong `android:fragment` class name causes silent non-clickable results

If `android:fragment` in an XML contains a typo or wrong package (e.g. missing `.settings`
subpackage), `Class.forName()` silently swallows the `ClassNotFoundException`. The preference
still appears in search results but clicking it does nothing.

**Fix:** double-check the fully-qualified class name in the XML matches the actual fragment class.

### Remove the upstream provider when Brave replaces an upstream fragment

When a Brave fragment fully replaces an upstream one (e.g. `BraveMainPreferencesBase` replaces
`MainSettings`, `BravePrivacySettings` replaces `PrivacySettings`), **remove the upstream
`SEARCH_INDEX_DATA_PROVIDER` from `SearchIndexProviderRegistry`** and register only the Brave one.

If both are registered, the upstream provider still runs and its `updateDynamicPreferences` may
call `updateEntrySummaryForKey` on entries that the Brave provider has already removed with
`removeEntry`. At that point `resolveIndex()` has pruned those entries and the upstream call throws
`IllegalStateException: Existing ID cannot be found`.

Concretely:
- `MainSettings.SEARCH_INDEX_DATA_PROVIDER` is replaced by
  `BraveMainPreferencesBase.SEARCH_INDEX_DATA_PROVIDER`, which internally calls the upstream
  `updateDynamicPreferences` first and then applies Brave-specific removals on top.
- `PrivacySettings.SEARCH_INDEX_DATA_PROVIDER` is replaced by
  `BravePrivacySettings.SEARCH_INDEX_DATA_PROVIDER`. Keeping the upstream one would cause it to
  call `updateEntrySummaryForKey("privacy_sandbox")` after `BraveMainPreferencesBase` has already
  removed the `"privacy"` parent entry, crashing the index build.

### Leaf prefs with no sub-screen are non-clickable in search results

The search result click handler navigates to a sub-screen fragment. Preferences that have no
`android:fragment` (switches, simple actions) cannot be navigated to — clicking the search result
does nothing.

Chrome never places such leaf preferences directly in main settings, so there is no upstream
infrastructure for handling them. **Remove them from the index.**

The following prefs in `brave_main_preferences.xml` are excluded for this reason in
`BraveMainPreferencesBase.SEARCH_INDEX_DATA_PROVIDER`:

| Key | Description |
|---|---|
| `closing_all_tabs_closes_brave` | Toggle switch — no sub-screen |
| `rate_brave` | Rate-app action — no sub-screen |
| `autofill_private_window` | Toggle switch — no sub-screen |
| `use_custom_tabs` | Toggle switch — no sub-screen |
| `home_screen_widget` | Conditionally excluded when `isRequestPinAppWidgetSupported()` is false |

### Upstream XML references upstream class, not Brave subclass

When Brave subclasses an upstream fragment (e.g. `BravePrivacySettings extends PrivacySettings`),
the upstream XML still has `android:fragment="...PrivacySettings"`. The recursive link resolution
follows the upstream class, not the Brave one. Use `addChildParentLink` in `initPreferenceXml` to
wire up the Brave subclass explicitly.
