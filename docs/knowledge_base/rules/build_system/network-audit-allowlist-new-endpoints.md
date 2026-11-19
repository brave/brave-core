---
name: brave/network-audit-allowlist-new-endpoints
description:
  'A new network endpoint contacted without user opt-in must be added to the
  network audit allowed list; endpoints only contacted on explicit user action
  need no entry.'
rubric: security
severity: blocking
---

**Rule.** When a change adds a network endpoint that the browser contacts
without user opt-in, the same change MUST add it to the network audit allowed
list in `brave/browser/net/brave_network_audit_allowed_lists.h`. NEVER require
an entry for an endpoint that is only requested on explicit user action, such as
clicking a button.

**Why.** The network audit check fails on any request to an endpoint missing
from the list, and the list is how Brave keeps track of what the browser
contacts on its own, which is a privacy commitment.

## Examples

Bad:

```cpp
// Fetched at startup; brave_network_audit_allowed_lists.h is not updated.
constexpr char kFooConfigUrl[] = "https://foo-config.brave.com/v1/config";
```

Good:

```cpp
// brave/browser/net/brave_network_audit_allowed_lists.h
inline constexpr auto kAllowedUrlPrefixes = std::to_array<std::string_view>({
    ...
    "https://foo-config.brave.com/",
});
```
