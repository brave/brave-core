/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_H_

#include "base/types/optional_ref.h"
#include "content/public/browser/storage_partition_config.h"

#define GetMainFrameDocumentSequenceNumber()               \
  GetMainFrameDocumentSequenceNumber() = 0;                \
  virtual base::optional_ref<const StoragePartitionConfig> \
  GetStoragePartitionConfig()

#include "src/content/public/browser/navigation_entry.h"  // IWYU pragma: export

#undef GetMainFrameDocumentSequenceNumber

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_H_
