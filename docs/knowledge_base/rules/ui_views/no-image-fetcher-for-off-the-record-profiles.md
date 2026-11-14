---
name: brave/no-image-fetcher-for-off-the-record-profiles
description:
  ImageFetcher must not fetch for an off-the-record or Tor profile, since it
  uses the system network context; skip the fetch and fall back, or use a
  profile-scoped SimpleURLLoader.
rubric: security
severity: blocking
---

**Rule.** Every new use of `ImageFetcher` (or `ImageFetcherService`) MUST be
checked for the profile it fetches for. Fetch only for a regular profile: when
`profile->IsOffTheRecord()` (which includes Tor), NEVER fetch; skip it and fall
back to a default, such as the first letter of the hostname. When the request
has to be made for a profile-scoped context, use a `SimpleURLLoader` with the
profile's `URLLoaderFactory` instead.

**Why.** `ImageFetcherService` sends requests through the browser-wide
`SystemNetworkContextManager` network context, which belongs to no profile. For
an off-the-record profile the request leaves the profile's network partition,
and in a Tor window it bypasses the Tor proxy and goes straight to the host,
leaking the user's IP address and the site visited.

## Examples

Bad:

```cpp
void SidebarModel::FetchFaviconFromNetwork(const SidebarItem& item) {
  image_fetcher_->FetchImage(item.url, ...);
}
```

Good:

```cpp
void SidebarModel::FetchFaviconFromNetwork(const SidebarItem& item) {
  if (profile_->IsOffTheRecord()) {
    return;  // Caller falls back to a default favicon.
  }
  image_fetcher_->FetchImage(item.url, ...);
}
```
