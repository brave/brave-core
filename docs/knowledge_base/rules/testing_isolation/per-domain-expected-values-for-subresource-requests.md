---
name: brave/per-domain-expected-values-for-subresource-requests
description:
  A test that checks the headers of requests a page with subresources makes keys
  its expected values by domain, not one shared expected value.
rubric: testing
severity: blocking
---

**Rule.** When a request monitor or handler checks the headers of requests made
by a page with subresources, keep the expected values in a map keyed by the
request's domain (taken from its `Host` header) and look each request up by its
own domain. NEVER compare every request against a single fixture-wide expected
value that the test overwrites between loads.

**Why.** Subresource requests arrive out of order and can still be in flight
when the test moves on and changes the expected value, so a single shared value
makes the check race against the network and the test flaky.

## Examples

Bad:

```cpp
std::string expected_header_;

void OnRequest(const HttpRequest& request) {
  EXPECT_EQ(request.headers["Accept-Language"], expected_header_);
}
```

Good:

```cpp
std::map<std::string, std::string> expected_headers_;

void OnRequest(const HttpRequest& request) {
  std::string domain = ExtractDomain(request.headers["Host"]);
  EXPECT_EQ(request.headers["Accept-Language"], expected_headers_[domain]);
}
```
