// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_

#include "chrome/browser/download/download_ui_model.h"

// Implement DeleteLocalFile method to remove the download item from disk.
#define SetOpenWhenComplete(...)             \
  SetOpenWhenComplete(__VA_ARGS__) override; \
  void DeleteLocalFile()

#include "src/chrome/browser/download/download_item_model.h"  // IWYU pragma: export

#undef SetOpenWhenComplete

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_MODEL_H_
