/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_SITE_INSTANCE_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_SITE_INSTANCE_H_

#include <optional>

#define CreateForURL(...)                        \
  CreateForURL(__VA_ARGS__);                     \
  static scoped_refptr<SiteInstance>             \
  CreateForURLWithOptionalFixedStoragePartition( \
      __VA_ARGS__,                               \
      const std::optional<StoragePartitionConfig>& storage_partition_config)

#include <content/public/browser/site_instance.h>  // IWYU pragma: export

#undef CreateForURL

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_SITE_INSTANCE_H_
