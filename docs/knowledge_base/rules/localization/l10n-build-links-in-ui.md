---
name: brave/l10n-build-links-in-ui
description:
  Links in localized text are built in the UI, with the URL passed separately,
  never embedded into the string through C++ formatting.
rubric: design
severity: should_fix
---

**Rule.** When localized text needs a link, build the link element in the UI
(TypeScript/HTML) and pass the URL separately, via `AddString` or a mojom
constant. Do not substitute URLs into the localized string with
`l10n_util::GetStringFUTF16()` or similar formatting.

**Why.** A URL formatted into a string mixes markup into translations and ties
every locale's text to the URL's value.

## Examples

Bad:

```cpp
html_source->AddString(
    "sectionDescription",
    l10n_util::GetStringFUTF16(IDS_SECTION_DESCRIPTION, kLearnMoreURL));
```

Good:

```cpp
html_source->AddString("sectionDescription",
                       l10n_util::GetStringUTF16(IDS_SECTION_DESCRIPTION));
html_source->AddString("learnMoreUrl", kLearnMoreURL);
```
