/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_H_

#include <string>
#include <utility>

#include "base/types/optional_ref.h"
#include "content/public/browser/storage_partition_config.h"

#define GetSSL()                                                        \
  GetSSL() = 0;                                                         \
  virtual void SetStoragePartitionKeyToRestore(                         \
      std::pair<std::string, std::string> storage_partition_key) = 0;   \
  virtual base::optional_ref<const std::pair<std::string, std::string>> \
  GetStoragePartitionKeyToRestore()

#include <content/public/browser/navigation_entry.h>  // IWYU pragma: export

#undef GetSSL

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_NAVIGATION_ENTRY_H_
