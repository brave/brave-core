# Runtime storage partition proxies

`brave::StoragePartitionProxyService` is an internal, UI-sequence API for
overriding an individual storage partition's proxy configuration. It does not
depend on Containers and has no preferences, sync data, Mojo settings API, or
UI.

Obtain the service with
`StoragePartitionProxyServiceFactory::GetForProfile(profile)`. Regular and
off-the-record profiles have separate instances; Tor profiles return `nullptr`.

## Implementation

- [Service](../browser/net/storage_partition_proxy_service.h): runtime
  overrides, policy observation, and network context lifetime.
- [Factory](../browser/net/storage_partition_proxy_service_factory.h): profile
  selection and service construction.
- [Context bridge](../browser/net/storage_partition_proxy_context.h): private
  Mojo proxy-config and error forwarding for a single network context.

## API

- `SetProxyConfig(partition_config, config, precedence)` installs a parsed
  `net::ProxyConfigWithAnnotation`. Callers supply the traffic annotation and
  validate any untrusted inputs before constructing the config. Chromium's
  fixed-server, bypass, direct, and PAC configurations are supported; the
  service does not reject configurations that can fall back to direct
  connections. `precedence` decides whether extension and command-line proxy
  settings take precedence over the override.
- `ClearProxyConfig(partition_config)` resumes inheriting the latest profile
  proxy settings.
- `GetProxyConfig(partition_config)` returns the requested override, or
  `std::nullopt` for inheritance. This is not the effective policy
  configuration.
- `AddObserver(observer)` reports PAC script errors and requests that may have
  failed because of proxy settings, for partitions whose override is in effect.

An off-the-record service CHECKs that partition configs are in-memory, because
`StoragePartitionConfig::Create` forces that for off-the-record contexts; an
on-disk config must belong to another profile.

Overrides are keyed by `content::StoragePartitionConfig`: the partition domain,
name, and `in_memory` flag. Use `StoragePartitionConfig::CreateDefault` for the
default partition. Disk-backed and in-memory partitions with the same domain and
name are distinct. The blob URL fallback mode is ignored, because content can
create partitions that differ only in that mode and share the same storage.

Content only passes the partition's relative path and `in_memory` flag to
`ContentBrowserClient::ConfigureNetworkContextParams`.
`BraveContentBrowserClient` maps them back to the config of the loaded
partition, which content registers before creating its network context.

Overrides may be set before a network context is created and updated at runtime.
They survive network-service restarts, but not profile shutdown or browser
restart. Owners must clear overrides when they no longer need them;
disconnecting a network context alone does not remove its override.

## Precedence and scope

Proxy preferences set by enterprise policy or a supervised-user custodian, in
the profile or local state, and policy dynamic-routing rules always take
precedence. Proxy preferences, including proxy override rules, set by an
extension or the command line take precedence only if the override was set with
`Precedence::kYieldToExtensionAndCommandLine`. Inherited proxy override rules
from an unknown source are kept. Removing those settings reactivates the
requested partition override. Global updates continue to reach inheriting
partitions and are retained while an override is active.

If Chrome has not yet determined the profile proxy configuration (for example,
while the system proxy settings are loading) and a partition must stop using an
override, its requests fail until that configuration arrives. They never fall
back to the stale override or to direct connections.

Proxy errors for inheriting partitions are reported to Chrome's proxy error
handling; errors for partitions using an override are reported only to
observers.

Updates affect subsequent requests without forcibly closing active connections.
Proxy failures and fallback behavior follow the supplied Chromium configuration;
this API does not add a direct fallback. Normal implicit localhost/link-local
bypass applies unless the supplied rules subtract it.

This only configures the partition's network context, not a VPN. WebRTC/UDP,
DNS, and browser services using other network contexts are not guaranteed to use
the partition proxy. Proxy authentication uses Chromium's existing challenge
handling.
