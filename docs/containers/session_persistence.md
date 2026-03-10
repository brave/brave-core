# Containers Session Persistence

This document explains how Brave Containers persist storage partition
information across browser sessions, enabling container tabs to be correctly
restored after browser restart, "Reopen closed tab", and via Sync.

## Container Storage Partition

Brave Containers provide isolated storage (cookies, localStorage, IndexedDB,
etc.) by using Chromium's `StoragePartitionConfig` system. Each container uses a
unique `StoragePartitionConfig` identified by:

- **partition_domain**: `"containers"` (constant for all containers)
- **partition_name**: The container's UUID (e.g.,
  `"550e8400-e29b-41d4-a716-446655440000"`)

When a container tab is closed and later restored (via browser restart, "Reopen
closed tab", or synced from another device), we need to restore it to the **same
storage partition**. However, Chromium's standard session serialization doesn't
preserve `StoragePartitionConfig` information.

Without a solution, restored container tabs would:

1. Open in the default storage partition
2. Mix container storage with default storage
3. Break the isolation that Containers provide

## Container Storage Partition Encoding

We encode the container's storage partition information into a **virtual URL
scheme prefix** during serialization:

```
Real URL:    https://example.com
Encoded URL: containers+550e8400-e29b-41d4-a716-446655440000:https://example.com
             ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
             Virtual prefix encoding the container UUID
```

This encoding is applied to:

1. **`virtual_url`** in `SerializedNavigationEntry` (saved to disk/sync)
2. **`PageState`** blob (Blink's internal serialized page state)

During deserialization, we detect this special scheme, extract the container
information, remove the prefix, and restore the original URL while setting the
correct `StoragePartitionConfig`.

### Why URL Encoding?

The special scheme format (`containers+<uuid>:`) provides multiple benefits:

1. **Unhandleable by Design**: Old browsers and browsers with Containers
   disabled cannot navigate to these URLs (treated as invalid schemes)
2. **Safe Degradation**: If the feature is disabled, tabs restore to a blank
   page with the prefix intact, preventing accidental mixing of container
   storage with default storage

## Serialization Flow

```
NavigationEntry (in-memory)
    │ has SiteInstance with StoragePartitionConfig
    │
    ├─> GetStoragePartitionKeyToRestore()
    │   Extracts: {"containers", "550e8400-e29b-41d4-a716-446655440000"}
    │
    ▼
ContentSerializedNavigationBuilder::FromNavigationEntry()
    │ Calls GetVirtualUrlPrefix() → "containers+550e8400-e29b-41d4-a716-446655440000:"
    │
    ├─> Sets storage_partition_key in SerializedNavigationEntry (runtime)
    ├─> Sets virtual_url_prefix in SerializedNavigationEntry (runtime)
    │
    ▼
ContentSerializedNavigationDriver::GetSanitizedPageStateForPickle()
    │ Adds prefix to PageState:
    │   PageState.URL = "containers+550e8400-e29b-41d4-a716-446655440000:https://example.com"
    │
    ▼
SerializedNavigationEntry::WriteToPickle() (written to disk)
    ├─> virtual_url prefixed: "containers+550e8400-e29b-41d4-a716-446655440000:https://example.com"
    ├─> encoded_page_state: <blob with prefixed URL>
    │
    ▼
SessionNavigationToSyncData() (written to sync)
    └─> virtual_url prefixed: "containers+550e8400-e29b-41d4-a716-446655440000:https://example.com"
```

## Deserialization Flow

```
SerializedNavigationEntry (read from disk/sync)
    │ virtual_url: "containers+550e8400-e29b-41d4-a716-446655440000:https://example.com"
    │
    ▼
ContentSerializedNavigationDriver::Sanitize()
    │ Calls RestoreStoragePartitionKeyFromUrl()
    │
    ├─> Parses "containers+550e8400-e29b-41d4-a716-446655440000:https://example.com"
    │   ├─> Extracts partition key: {"containers", "550e8400-e29b-41d4-a716-446655440000"}
    │   └─> Gets original URL: "https://example.com"
    │
    ├─> Sets virtual_url to "https://example.com"
    ├─> Sets storage_partition_key to {"containers", "550e8400-e29b-41d4-a716-446655440000"}
    ├─> Sets virtual_url_prefix to "containers+550e8400-e29b-41d4-a716-446655440000:"
    │
    ├─> Removes prefix from PageState:
    │   PageState.URL = "https://example.com"
    │
    ▼
ContentSerializedNavigationBuilder::ToNavigationEntry()
    │ Calls SetStoragePartitionKeyToRestore() on NavigationEntry
    │
    ▼
NavigationEntry (in-memory)
    │ Has storage_partition_key_to_restore_ = {"containers", "550e8400-e29b-41d4-a716-446655440000"}
    │
    ├─> Used to create SiteInstance with correct StoragePartitionConfig
    │
    ▼
Tab restored with correct container storage partition ✓
```

## Key Design Decisions

### 1. Why Not Just Store Partition Info Separately?

**Considered**: Adding explicit `partition_domain` and `partition_name` fields
to the session file format.

**Rejected because**:

- Requires modifying Chromium's session file format
- Backward compatibility issues with old sessions
- Doesn't solve the PageState URL problem
- Doesn't provide safe degradation when feature is disabled

**URL encoding solves all of these**:

- Works within existing session format
- Backward compatible (old browsers just see an invalid URL)
- Forces PageState to contain the encoded URL
- Safe degradation (unhandleable URL when feature disabled)

### 2. Why Manipulate PageState?

**PageState** is a Blink-maintained blob containing data for fast restoration of
a page. It also contains the URL to navigate to.

Even if we modify `virtual_url` in `NavigationEntry`, Blink would still navigate
to the URL stored in PageState. This would bypass our protection if Containers
is disabled.

### 3. Sync Considerations

**Cross-device sync**: When a container tab syncs to another device:

| Source Device  | Target Device  | Behavior                          |
| -------------- | -------------- | --------------------------------- |
| Containers ON  | Containers ON  | Tab restores to same container ✓  |
| Containers ON  | Containers OFF | URL remains encoded, page blank ✓ |
| Containers OFF | Containers ON  | Normal tab, no container          |

**Privacy**: Only the container name and icon are synced (as part of syncable
prefs). The actual **storage data** (cookies, localStorage, etc.) in the
container is **never synced**.
