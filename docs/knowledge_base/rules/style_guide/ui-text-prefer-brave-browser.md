---
name: brave/ui-text-prefer-brave-browser
description:
  Where the text means the browser, it says "Brave browser" rather than just
  "Brave" where possible.
rubric: readability
severity: nit
---

**Rule.** When a string refers to the browser itself, write "Brave browser"
rather than just "Brave" where the space allows. Keep "Brave" alone for the
company and the brand as a whole.

**Why.** "Brave" is the whole brand (Search, VPN, Wallet and more); using it for
the browser alone narrows the brand to one product.

## Examples

Bad:

```xml
<message name="IDS_DEFAULT_BROWSER_BODY" desc="Body of the default browser prompt">
  Any web links you click will open in Brave.
</message>
```

Good:

```xml
<message name="IDS_DEFAULT_BROWSER_BODY" desc="Body of the default browser prompt">
  Any web links you click will open in the Brave browser.
</message>
```
