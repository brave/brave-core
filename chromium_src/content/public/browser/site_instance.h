/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_SITE_INSTANCE_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_SITE_INSTANCE_H_

#include <base/types/optional_ref.h>

// Extends SiteInstance with CreateForURLWithOptionalFixedStoragePartition, a
// helper used by the containers feature to apply a StoragePartitionConfig. When
// a config is present it delegates to CreateForFixedStoragePartition; otherwise
// it falls back to CreateForURL.
#define CreateForURL(...)                        \
  CreateForURL(__VA_ARGS__);                     \
  static scoped_refptr<SiteInstance>             \
  CreateForURLWithOptionalFixedStoragePartition( \
      __VA_ARGS__,                               \
      base::optional_ref<StoragePartitionConfig> storage_partition_config)

#include <content/public/browser/site_instance.h>  // IWYU pragma: export

#undef CreateForURL

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_SITE_INSTANCE_H_
