---
name: brave/hsts-pin-list-new-endpoints
description:
  'Every new Brave endpoint must be added to both the pins and the HSTS entries
  of the transport security state generator override, whether or not it needs
  opt-in.'
rubric: security
severity: blocking
---

**Rule.** When a change adds a new Brave endpoint (a host under `brave.com`,
`bravesoftware.com`, `brave.software` or another Brave domain), it MUST also add
the host to
`chromium_src/net/tools/transport_security_state_generator/input_file_parsers.cc`,
in both the pins `"entries"` list and the HSTS `"entries"` list. This applies
whether or not the endpoint requires user opt-in.

**Why.** The pin list makes the browser reject a certificate for a Brave host
whose chain holds none of Brave's pinned keys, and the HSTS list keeps the host
on HTTPS. A Brave endpoint missing from either is open to interception that the
other endpoints are protected from.

## Examples

Bad:

```cpp
// A new endpoint, with input_file_parsers.cc unchanged.
inline constexpr char kFooApiUrl[] = "https://foo-api.brave.com/v1/";
```

Good:

```cpp
// chromium_src/net/tools/transport_security_state_generator/input_file_parsers.cc
// Added to the pins "entries" list and to the HSTS "entries" list:
"foo-api.brave.com"
```
