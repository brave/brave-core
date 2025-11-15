// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_

#include "chrome/browser/download/download_ui_model.h"

// Implements a few methods
// * DeleteLocalFile() methods to remove the download item from disk.
// * CopyDownloadLinkToClipboard() to copy the download item's URL to clipboard.
#define SetOpenWhenComplete(...)             \
  SetOpenWhenComplete(__VA_ARGS__) override; \
  void DeleteLocalFile() override;           \
  void CopyDownloadLinkToClipboard()

#include <chrome/browser/download/download_item_model.h>  // IWYU pragma: export

#undef SetOpenWhenComplete

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_
