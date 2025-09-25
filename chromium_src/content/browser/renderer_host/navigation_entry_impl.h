/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_NAVIGATION_ENTRY_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_NAVIGATION_ENTRY_IMPL_H_

#include <string>
#include <utility>

#include "content/public/browser/navigation_entry.h"

#define GetMainFrameDocumentSequenceNumber()                               \
  GetMainFrameDocumentSequenceNumber() override;                           \
                                                                           \
 private:                                                                  \
  std::optional<std::pair<std::string, std::string>>                       \
      storage_partition_key_to_restore_;                                   \
                                                                           \
 public:                                                                   \
  void SetStoragePartitionKeyToRestore(                                    \
      std::pair<std::string, std::string> storage_partition_key) override; \
  base::optional_ref<const std::pair<std::string, std::string>>            \
  GetStoragePartitionKeyToRestore()

#include <content/browser/renderer_host/navigation_entry_impl.h>  // IWYU pragma: export

#undef GetMainFrameDocumentSequenceNumber

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_NAVIGATION_ENTRY_IMPL_H_
