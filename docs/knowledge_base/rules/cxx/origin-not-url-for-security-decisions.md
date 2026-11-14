---
name: brave/origin-not-url-for-security-decisions
description:
  "A decision to grant or limit a site's capabilities is made from the
  GetLastCommittedOrigin() of the RenderFrameHost concerned, never from a GURL
  or an origin derived from one."
rubric: security
severity: blocking
---

**Rule.** When code extends or limits what a site may do (permissions, shields,
farbling, access to browser functionality), it MUST decide from a `url::Origin`
or `SecurityOrigin`: that of the exact `RenderFrameHost` making the request,
read with `GetLastCommittedOrigin()` (or from the render process when there is
no frame, as for `WebSockets` and `WebTransport`). NEVER decide from a `GURL`
(`GetLastCommittedURL()`, a request URL), and do not derive the origin from one
with `url::Origin::Create(url)` or `url::SchemeHostPort(url)`. For `blob:`,
`about:blank` and `about:srcdoc` frames, the frame's committed origin is the
embedder's, which is the origin to use.

**Why.** Converting a `GURL` to an origin is often not the origin the frame has:
a `blob:` or `about:blank` frame's URL says nothing of the embedder whose origin
it runs with. brave-browser#56048 let any embedder of a `blob:` URL bypass
farbling for this reason.

## Examples

Bad:

```cpp
const GURL& url = render_frame_host->GetLastCommittedURL();
if (ShouldFarble(url::Origin::Create(url))) { ... }
```

Good:

```cpp
const url::Origin& origin = render_frame_host->GetLastCommittedOrigin();
if (ShouldFarble(origin)) { ... }
```
