# Localization & String Resources

Technical conventions for GRD/GRDP string resources, placeholders, and i18n patterns. For voice, capitalization, punctuation, product naming, and general writing style, see the [Brave Style Guide](./style-guide.md).

<a id="L10N-001"></a>

## ❌ Don't Add `translateable="true"` on GRD Strings

**In `.grdp` string resource files, `translateable="true"` is the default.** Never add it explicitly. The tooling only looks for `translateable="false"` when excluding strings from translation.

---

<a id="L10N-002"></a>

## ✅ Use Proper Ellipsis Characters in UI Strings

**In user-facing strings, use the proper Unicode ellipsis character (`…`) instead of three periods (`...`).** This is a standard typographic convention for UI text.

---

<a id="L10N-003"></a>

## ✅ Provide Sufficient Context in Localization String Descriptions

**Localization string descriptions should contain enough context for translators who only see the description.** For example, "History" alone might be ambiguous across languages (browser history vs. event history). Add specifics like "Title for the browser visits history section of a URL picker".

For simple, self-evident strings where the meaning is unambiguous in any language (e.g., "Summary", "Settings", "Cancel"), a brief description is fine — do not insist on verbose descriptions when the string speaks for itself.

Additional requirements for `desc` attributes:
- **Never leave `desc` empty** — every `<message>` element must have a non-empty description.
- **Specify the UI element type** — include whether the string is a title, button label, description, tooltip, placeholder, menu item, etc. (e.g., `desc="Title for the about section"` not `desc="About section text"`).
- **Disambiguate terms with multiple meanings** — words like "History" (browser vs. conversation), "Wallet" (crypto vs. payment), "Send" (where?) need qualification.
- **Each description must be unique** — do not copy-paste the same `desc` across different strings that serve different purposes.
- **Proofread descriptions** — typos in `desc` confuse translators and affect translation quality.

---

<a id="L10N-004"></a>

## ✅ Use `I18nMixinLit` for Localization in Lit Components

**In Lit-based WebUI components, use `I18nMixinLit` instead of calling `loadTimeData.getString()` directly.** The mixin provides consistent i18n patterns and is the standard approach.

---

<a id="L10N-005"></a>

## ❌ Never Modify Upstream GRD/GRDP Files Directly

**Never add or modify strings in upstream Chromium GRD/GRDP files** (e.g., `app/generated_resources.grd`, `app/settings_strings.grdp`, `components/browser_ui/strings/android/browser_ui_strings.grd`). These files are auto-replaced during every Chromium version bump, so any changes will be lost on the next rebase.

To customize an upstream string:
1. Add a new string in the corresponding Brave GRD/GRDP file (e.g., `app/brave_generated_resources.grd`, `app/brave_settings_strings.grdp`, `browser/ui/android/strings/android_brave_strings.grd`).
2. Create a `chromium_src` override that `#undef`s the upstream `IDS_` constant and `#define`s it to your Brave-specific `IDS_` constant.

```cpp
// chromium_src/chrome/browser/ui/webui/password_manager/password_manager_ui.cc
#undef IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS
#define IDS_PASSWORD_MANAGER_UI_EMPTY_STATE_SYNCING_USERS \
  IDS_BRAVE_PASSWORD_MANAGER_UI_EMPTY_STATE_SIGNEDOUT_USERS
```

---

<a id="L10N-006"></a>

## ✅ Use `grd_string_replacements.py` for Simple Upstream Text Substitutions

**For simple word/phrase substitutions across upstream strings in all locales, use `brave/script/lib/l10n/grd_string_replacements.py`** with `default_replacements` and run `npm run chromium_rebase_l10n`. This applies the replacement consistently across all localized versions without needing to manually edit GRD files or create chromium_src overrides.

---

<a id="L10N-007"></a>

## ✅ Include `formatter_data="webui=X"` Matching the WebUI Component

**When adding strings to a WebUI GRDP file, include the `formatter_data="webui=ComponentName"` attribute.** The value must exactly match the WebUI component identifier so the build system generates the correct string constants for the right bundle.

```xml
<!-- ✅ CORRECT - matches the AI Chat WebUI component -->
<message name="IDS_CHAT_UI_WELCOME" desc="Welcome message" formatter_data="webui=AiChat">
  Welcome to Brave AI
</message>

<!-- ❌ WRONG - typo or mismatch means string won't be available in the component -->
<message name="IDS_CHAT_UI_WELCOME" desc="Welcome message" formatter_data="webui=AIChat">
```

---

<a id="L10N-008"></a>

## ✅ Use SCREAMING_SNAKE_CASE Placeholder Names with Example Tags

**Placeholders in GRD/GRDP strings must have descriptive `name` attributes in SCREAMING_SNAKE_CASE and include `<ex>` example tags** showing translators what value will appear at runtime.

```xml
<!-- ✅ CORRECT - descriptive name and example -->
<message name="IDS_SHRED_CONFIRMATION" desc="Confirmation dialog for shredding site data">
  Shredding will delete site data and close all
  <ph name="SITE_NAME">$1<ex>example.com</ex></ph> tabs.
</message>

<!-- For link placeholders, use LINK_BEGIN/LINK_END pattern -->
<message name="IDS_LEARN_MORE" desc="Text with learn more link">
  <ph name="LINK_BEGIN">$1</ph>Learn more<ph name="LINK_END">$2</ph>
</message>

<!-- ❌ WRONG - no descriptive name, no example -->
<message name="IDS_SHRED_CONFIRMATION" desc="Confirmation">
  Shredding will delete site data for <ph name="PH_1">$1</ph>.
</message>
```

---

<a id="L10N-009"></a>

## ✅ Capitalization Consistency Across Feature Strings

**All strings within a single feature or UI surface must use a consistent capitalization style.** Do not mix title case ("Background Image") and sentence case ("Brave backgrounds") within the same group of UI controls at the same hierarchy level.

- **Title case** for: page titles, section headers, button labels, menu items
- **Sentence case** for: descriptions, body text, tooltips, helper text

---

<a id="L10N-010"></a>

## ✅ UI Text Voice and Style Conventions

**Follow these conventions for user-facing string text:**

- **Use "Brave's" instead of "our"** — e.g., "access Brave's premium products" not "access our premium products"
- **Use "and more" instead of "etc."** — e.g., "bookmarks, passwords, and more" not "bookmarks, passwords, etc."
- **Match established feature terminology** — e.g., Shields uses "up/down" not "on/off" (`"with Shields up"` not `"with Shields on"`). Check existing strings for a feature's vocabulary before adding new ones.
- **Use natural word order around placeholders** — test by substituting an example value and reading the sentence aloud
- **Avoid possessive constructions** — prefer "the content of this page" over "this page's contents" for better translatability
- **Be explicit about data-sharing actions** — "send your tabs to Brave AI" not just "send your tabs" (where?)

---

<a id="L10N-011"></a>

## ❌ Never Add Unused Strings to GRD/GRDP Files

**Only add strings to GRD/GRDP files when they are actively referenced in the codebase.** Every string in a GRD/GRDP file gets sent to translators and translated into all supported languages, which costs time and money. If a string is no longer used after a refactor, remove it. When reviewing string additions, verify the `IDS_` constant is actually used in the code.

---

<a id="L10N-012"></a>

## ✅ Replace Upstream Product Names with Brave Equivalents During Rebases

**During Chromium rebases, review all new strings in upstream GRD/GRDP files for Google product references** (Gemini, Google Pay, Chrome, etc.) that need to be replaced with Brave equivalents. New user-facing strings introduced by rebases should be reviewed to ensure they show correct Brave branding.

Also verify correct product name usage: "Brave Wallet" refers specifically to the cryptocurrency wallet, not general autofill/payment card features.

---

<a id="L10N-013"></a>

## ❌ Avoid Dynamic String Key Construction in `getLocale()`

**Never dynamically construct string keys for `getLocale()` calls.** This makes static analysis impossible, prevents dead string detection, and breaks if key formats change.

```tsx
// ❌ WRONG - dynamic key construction
getLocale(`CHAT_UI_${model.key.toUpperCase().replaceAll('-', '_')}_SUBTITLE`)

// ✅ CORRECT - explicit string reference
getLocale(S.CHAT_UI_CLAUDE_SUBTITLE)

// ✅ ALSO CORRECT - data source provides the string or key
model.description  // provided by server API / mojom struct
```

---

<a id="L10N-014"></a>

## ✅ Build Links in UI, Not in Localized Strings

**When a localized string needs a clickable link, build the link element in the UI (TypeScript/HTML) and pass the URL separately** via `AddString` or a mojom constant. Do not embed URLs into localized strings via C++ string formatting.

```cpp
// ❌ WRONG - URL embedded in localized string
html_source->AddString(
    "sectionDescription",
    l10n_util::GetStringFUTF16(IDS_SECTION_DESCRIPTION, kLearnMoreURL));

// ✅ CORRECT - URL passed separately, link built in UI
html_source->AddString("sectionDescription",
    l10n_util::GetStringUTF16(IDS_SECTION_DESCRIPTION));
html_source->AddString("learnMoreUrl", kLearnMoreURL);
```

---

<a id="L10N-015"></a>

## ✅ Punctuation Rules for Settings Toggle Descriptions

**Settings toggle descriptions follow specific punctuation rules:**

- **Single-sentence descriptions** should NOT end with a period.
- **Multi-sentence descriptions** SHOULD end with a period on the last sentence.

---

<a id="L10N-016"></a>

## ✅ String Text Must Be Complete and Grammatically Correct

**User-facing strings must be complete sentences (not truncated), grammatically correct, and use proper punctuation.** String reviewers verify:

- Sentences are not cut off mid-thought
- Compound adjectives are hyphenated (e.g., "non-compatible" not "non compatible")
- Prepositions are appropriate for context (e.g., "in" vs. "on" for UI locations)

This matters because translators use the English source as a reference, and errors propagate to translations.

---

<a id="L10N-017"></a>

## ✅ Include UI Screenshots When Adding or Modifying Strings

**When a PR adds or modifies user-facing strings, the developer must include a screenshot of the affected UI in the PR description.** This allows string reviewers to verify the string in context — checking for truncation, capitalization consistency, layout issues, and correct terminology. Screenshots should show the actual rendered UI, not just the GRD/GRDP diff.

Do not add screenshots as inline code comments. Add them to the PR description body.

See also: [Brave Style Guide](./style-guide.md) for voice, capitalization, punctuation, and product naming rules that apply to all user-facing strings.
