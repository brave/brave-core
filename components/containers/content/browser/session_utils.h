// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_SESSION_UTILS_H_
#define BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_SESSION_UTILS_H_

#include <optional>
#include <string>
#include <utility>

#include "base/component_export.h"
#include "url/gurl.h"

namespace containers {

// This file provides utilities for serializing and deserializing
// StoragePartitionConfig information as part of session/tab restoration.
//
// Containers use custom StoragePartitionConfigs to isolate storage (cookies,
// localStorage, etc.) per container. When a container tab is closed and later
// restored (via browser restart, "Reopen closed tab", or Sync), we need to
// restore it to the same container storage partition. However, the standard
// Chromium session serialization doesn't preserve StoragePartitionConfig data.
//
// We encode the container's storage partition key (partition_domain +
// partition_name) into a virtual URL scheme prefix during serialization:
//
//     Real URL:    https://example.com
//     Encoded URL: containers+<uuid>:https://example.com
//                  (where <uuid> is the container's unique identifier)
//
// This encoding is applied to:
// 1. The virtual_url in SerializedNavigationEntry.
// 2. The PageState struct.
//
// During deserialization, we detect this special scheme, extract the container
// information, remove the prefix, and restore the original URL while setting
// the correct StoragePartitionConfig.
//
// Why URL encoding?
// - The special scheme ensures old browsers or browsers with Containers
//   disabled cannot navigate to these URLs (they're treated as unhandleable).
// - If the feature is disabled, tabs restore to a blank page with a
//   "containers+<uuid>:" URL, preventing accidental mixing of container storage
//   with default storage.
// - PageState manipulation is necessary because even if NavigationEntry has a
//   modified URL, the browser would still open the URL stored in PageState,
//   bypassing the "unsupported scheme" protection.
//
// Serialization flow:
// 1. Tab is saved (closing, browser exit, or sync).
// 2. `GetUrlPrefixForSessionPersistence()` creates prefix (e.g.,
//    "containers+<uuid>:").
// 3. Prefix is prepended to URLs in SerializedNavigationEntry and PageState.
// 4. SerializedNavigationEntry is written to disk/sync.
//
// Deserialization flow:
// 1. Session/tab is restored (browser start, reopen tab, or sync).
// 2. RestoreStoragePartitionKeyFromUrl() detects special scheme.
// 3. Extracts container storage partition key and original URL.
// 4. Sets StoragePartitionConfig on the restored SerializedNavigationEntry.
// 5. Removes prefix from URLs in SerializedNavigationEntry and PageState.

// Generates a URL prefix for encoding container storage partition info. Given a
// storage partition key, returns a URL prefix that can be prepended to URLs in
// SerializedNavigationEntry and PageState during session serialization.
COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
std::optional<std::string> GetUrlPrefixForSessionPersistence(
    const std::pair<std::string, std::string>& storage_partition_key);

// Extracts container storage partition info from an encoded URL. This is the
// inverse of `GetUrlPrefixForSessionPersistence()`. Given a URL with an encoded
// container prefix, extracts the storage partition key (partition_domain,
// partition_name) and the original URL (https://example.com).
COMPONENT_EXPORT(CONTAINERS_CONTENT_BROWSER)
std::optional<GURL> RestoreStoragePartitionKeyFromUrl(
    const GURL& url,
    std::pair<std::string, std::string>& storage_partition_key,
    size_t& url_prefix_length);

}  // namespace containers

#endif  // BRAVE_COMPONENTS_CONTAINERS_CONTENT_BROWSER_SESSION_UTILS_H_
