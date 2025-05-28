/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_TAB_CONTENTS_TAB_UTIL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_TAB_CONTENTS_TAB_UTIL_H_

#include "content/public/browser/storage_partition_config.h"

#define GetSiteInstanceForNewTab(...) \
  GetSiteInstanceForNewTab(           \
      __VA_ARGS__,                    \
      std::optional<content::StoragePartitionConfig> storage_partition_config)

#include "src/chrome/browser/tab_contents/tab_util.h"  // IWYU pragma: export

#undef GetSiteInstanceForNewTab

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_TAB_CONTENTS_TAB_UTIL_H_
