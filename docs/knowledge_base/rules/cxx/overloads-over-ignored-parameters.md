---
name: brave/overloads-over-ignored-parameters
description:
  'A function does not take a parameter that some of its callers must pass only
  for it to be ignored; it is split into overloads.'
rubric: design
severity: should_fix
---

**Rule.** When a parameter means something only for some calls (a request body
that is ignored for GET), do not make every caller pass it. Provide an overload
for each call pattern, each taking only the parameters it uses.

**Why.** A silently ignored parameter misleads the caller, who expects it to
have an effect, and lets meaningless arguments pile up at call sites.

## Examples

Bad:

```cpp
// body_value is silently ignored for GET and HEAD.
void ApiFetch(const std::string& verb,
              const std::string& url,
              const base::Value& body_value,
              Callback cb);
```

Good:

```cpp
void ApiFetch(const std::string& url, Callback cb);  // GET
void ApiFetch(const std::string& url,
              const base::Value& body,
              Callback cb);  // POST
```
