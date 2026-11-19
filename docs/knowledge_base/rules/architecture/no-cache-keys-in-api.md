---
name: brave/no-cache-keys-in-api
description:
  'APIs do not take or expose internal cache keys; the implementation generates
  them.'
rubric: design
severity: should_fix
---

**Rule.** Do not add a cache key parameter, or a cache key in a returned value,
to a public interface (a header's API, a mojom method). Derive the key inside
the implementation from the request's own parameters.

**Why.** A cache key in the API exposes an implementation detail, and makes
every caller responsible for producing unique keys correctly.

## Examples

Bad:

```cpp
void FetchData(const std::string& url, const std::string& cache_key,
               Callback cb);
```

Good:

```cpp
void FetchData(const std::string& url, Callback cb);
// Internally: cache_key = GenerateKey(url, params)
```
