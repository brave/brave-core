---
name: brave/hide-conditional-preferences-in-xml
description:
  A preference declared in XML that code removes or hides behind a feature flag
  is declared with app:isPreferenceVisible="false".
rubric: correctness
severity: should_fix
---

**Rule.** When a preference declared in a preferences XML file is removed or
hidden by code depending on a feature flag (`removePreference()`,
`setVisible(false)`), its XML declaration MUST set
`app:isPreferenceVisible="false"`, and the code shows it when the flag allows.

**Why.** A preference visible by default in XML is drawn before the code that
hides it runs, so the user sees it flash on the screen and disappear.

## Examples

Bad:

```xml
<org.chromium.chrome.browser.settings.BraveAccountPreference
    android:key="brave_account"
    android:title="@string/brave_account_title" />
```

Good:

```xml
<org.chromium.chrome.browser.settings.BraveAccountPreference
    android:key="brave_account"
    app:isPreferenceVisible="false"
    android:title="@string/brave_account_title" />
```
