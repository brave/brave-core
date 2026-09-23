/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define BRAVE_HOST_ZOOM_MAP_GET_FOR_STORAGE_PARTITION                      \
  if (GetContentClient()                                                   \
          ->browser()                                                      \
          ->ShouldUseDefaultHostZoomMapForStoragePartition(                \
              storage_partition->GetConfig())) {                           \
    auto* partition_impl =                                                 \
        static_cast<StoragePartitionImpl*>(storage_partition);             \
    return GetDefaultForBrowserContext(partition_impl->browser_context()); \
  }

#include <content/browser/host_zoom_map_impl.cc>

#undef BRAVE_HOST_ZOOM_MAP_GET_FOR_STORAGE_PARTITION
