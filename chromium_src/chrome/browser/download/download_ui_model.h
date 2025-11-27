// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_UI_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_UI_MODEL_H_

// Declare a method to handle DownloadCommands::kDeleteLocalFile,
// DownloadCommands::kCopyDownloadLink command.
#define OpenUsingPlatformHandler(...)           \
  OpenUsingPlatformHandler_Unused() {}          \
  virtual void DeleteLocalFile() {}             \
  virtual void CopyDownloadLinkToClipboard() {} \
  virtual void OpenUsingPlatformHandler(__VA_ARGS__)

#include <chrome/browser/download/download_ui_model.h>  // IWYU pragma: export

#undef OpenUsingPlatformHandler

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_UI_MODEL_H_
