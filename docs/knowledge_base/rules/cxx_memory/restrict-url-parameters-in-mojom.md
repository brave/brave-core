---
name: brave/restrict-url-parameters-in-mojom
description:
  'Mojom methods taking URLs from less-privileged processes are reviewed for
  restricting them to an allowlist or enum; NEVER ask for URL-validation
  comments when the URL is already validated downstream.'
rubric: security
severity: should_fix
---

**Rule.** When a mojom interface implemented in a more privileged process takes
a URL parameter (`url.mojom.Url`, or a URL in a `string`) from a renderer or
another less-privileged process, consider restricting the input to an allowlist
or an enum of the endpoints the caller may reach, rather than accepting an
arbitrary URL. NEVER request a documentation comment about URL validation when
the implementation already validates or filters the URL downstream, and do not
ask for one that similar code nearby does not have.

**Why.** An unrestricted URL parameter lets a compromised renderer make the
privileged process send requests to any endpoint. A comment about validation the
code already performs adds nothing but inconsistency and verbosity.

## Examples

Bad:

```
interface FooPageHandler {
  // The renderer can make the browser fetch any URL.
  FetchResource(url.mojom.Url url) => (string body);
};
```

Good:

```
enum FooResource { kTerms, kPrivacyPolicy };

interface FooPageHandler {
  FetchResource(FooResource resource) => (string body);
};
```
