/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_FPS_ENTRY_WRAPPER_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_FPS_ENTRY_WRAPPER_H_

#include "base/time/time.h"
#include "base/values.h"
#include "content/public/browser/storage_partition_config.h"
#include "url/gurl.h"

namespace ephemeral_storage {

  // A single first party storage area queued for cleanup: the storage area
  // identity (url + storage partition) plus the time the last tab using it was
  // closed. The close time lets a pending cleanup survive a browser restart
  // while still honoring the keep-alive window.
  class FirstPartyStorageAreaEntry {
   public:
    FirstPartyStorageAreaEntry(const FirstPartyStorageAreaEntry&) = default;
    FirstPartyStorageAreaEntry& operator=(const FirstPartyStorageAreaEntry&) =
        default;
    FirstPartyStorageAreaEntry(FirstPartyStorageAreaEntry&&) = default;
    FirstPartyStorageAreaEntry& operator=(FirstPartyStorageAreaEntry&&) =
        default;
    ~FirstPartyStorageAreaEntry();

    // Parses an entry from a stored pref value. Supports the legacy formats:
    //   - a bare url spec string (default storage partition, no close time),
    //   and
    //   - a dict without the close-time key (older versions).
    // Returns nullopt if the value can't be interpreted as an area.
    static std::optional<FirstPartyStorageAreaEntry> FromValue(
        const base::Value& value,
        content::BrowserContext* browser_context);

    static FirstPartyStorageAreaEntry Create(
        const GURL& url,
        const content::StoragePartitionConfig& storage_partition_config);

    // Serializes to the current dict format (always includes the close time).
    base::Value ToValue() const;

    // Matches by storage area identity only, ignoring the close time.
    bool Matches(
        const GURL& url,
        const content::StoragePartitionConfig& storage_partition_config) const;

    // True if the keepalive pending when the area was stored has already
    // elapsed, i.e. re-using the area should no longer cancel the queued
    // cleanup. A missing close time (legacy entries) or a backwards clock jump
    // is treated as expired.
    bool IsKeepAliveExpired(base::TimeDelta keep_alive) const;

    const GURL& url() const { return url_; }
    const content::StoragePartitionConfig& storage_partition_config() const {
      return storage_partition_config_;
    }
    base::Time closed_at() const { return closed_at_; }

   private:
    FirstPartyStorageAreaEntry(
        const GURL& url,
        const content::StoragePartitionConfig& storage_partition_config,
        base::Time closed_at);

    GURL url_;
    content::StoragePartitionConfig storage_partition_config_;
    base::Time closed_at_;
  };

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_FPS_ENTRY_WRAPPER_H_
