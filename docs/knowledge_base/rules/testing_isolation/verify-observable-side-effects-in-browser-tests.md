---
name: brave/verify-observable-side-effects-in-browser-tests
description:
  Browser tests assert the visible result of an action (the tab, the loaded URL,
  the dialog), not only the value the action returned.
rubric: testing
severity: should_fix
---

**Rule.** After a browser test performs an action, it checks what the action
should have done to the browser: that the tab actually appeared, that the URL
actually loaded, that the dialog was shown. Flag a browser test whose only check
of an action is `EXPECT_TRUE(DoAction())` on its return value.

**Why.** A browser test exists to catch the integration problems unit tests
miss. The return value only says the call thought it succeeded; whether the tab,
page or dialog is really there is the integration that needs checking.

## Examples

Bad:

```cpp
EXPECT_TRUE(OpenNewTab(url));
```

Good:

```cpp
EXPECT_TRUE(OpenNewTab(url));
EXPECT_EQ(browser()->tab_strip_model()->count(), initial_count + 1);
EXPECT_EQ(GetActiveWebContents()->GetURL(), url);
```
