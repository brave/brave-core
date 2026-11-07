---
name: security/new-request-and-storage-paths
rubric: security
severity: regression
---

**Rule.** New network request and storage paths go through Brave's choke points:
`WillCreateURLLoaderFactory`, URL loader and navigation throttles, the ephemeral
storage nonce, and Tor's `ProxyConfigService` and
`ConfiguredProxyResolutionService` hooks.

**Why.** A request path that skips them skips Brave's shields, Tor proxying and
storage partitioning, with no error.

**How to check.** For each candidate commit, which adds a
`CreateURLLoaderFactory(`, `SimpleURLLoader::Create`, `URLLoaderFactoryType`
value, fetch type, proxy resolver or `DefineNetworkTrafficAnnotation(`, trace
whether the new path reaches Brave's hooks. For designs where the renderer reads
something directly, check whether a consumer exists yet, and pin the flag off if
it would bypass Brave.

## Examples

- M157: the system proxy resolver skips both Tor hooks.
