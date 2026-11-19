---
name: brave/fail-loudly-on-unhandled-test-server-requests
description:
  A test server handler or URL interceptor meant to serve every request ends
  with NOTREACHED() streaming the request, not a silent empty response.
rubric: testing
severity: should_fix
---

**Rule.** In a request handler of a test server, or a URL interceptor, that is
meant to serve the requests the test makes, the fall-through for a request no
branch handles is `NOTREACHED()` streaming the request's URL and body
(`<< request.relative_url << request.content`, or the interceptor's equivalent).
It MUST NOT silently return `nullptr` or an empty response.

**Why.** A silent default turns a request the test did not expect (a wrong path,
a new endpoint, a changed body) into a vague failure much later, or into a pass.
Failing at the unhandled request, with its body in the message, shows at once
which request it was.

## Examples

Bad:

```cpp
std::unique_ptr<HttpResponse> HandleRequest(const HttpRequest& request) {
  if (request.relative_url == "/api/v1") {
    return MakeResponse();
  }
  return nullptr;
}
```

Good:

```cpp
std::unique_ptr<HttpResponse> HandleRequest(const HttpRequest& request) {
  if (request.relative_url == "/api/v1") {
    return MakeResponse();
  }
  NOTREACHED() << "Unhandled request: " << request.relative_url
               << " body: " << request.content;
}
```
