---
name: brave/ui-text-currency-format
description:
  Prices in user-facing text use the currency symbol of the country, with the
  country specified outside the US, such as "$9.99" or "HK$9.99".
rubric: readability
severity: nit
---

**Rule.** Write prices with the currency symbol of the country ("$9.99"), and
outside the US qualify the symbol with the country ("HK$9.99"). Do not spell out
the currency name.

**Why.** A bare "$" is ambiguous outside the US; the qualified symbol tells
users exactly what they pay.

## Examples

Bad:

```xml
<message name="IDS_VPN_PRICE_HK" desc="VPN price for Hong Kong">
  9.99 dollars per month
</message>
```

Good:

```xml
<message name="IDS_VPN_PRICE_HK" desc="VPN price for Hong Kong">
  HK$9.99 per month
</message>
```
