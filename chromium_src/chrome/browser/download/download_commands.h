/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_COMMANDS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_COMMANDS_H_

#include "src/chrome/browser/download/download_commands.h"  // IWYU pragma: export

// Create brave specific commands set instead of appending to
// DownloadCommands::Command to avoid many upstream changes.
enum class BraveDownloadCommands {
  REMOVE_FROM_LIST = DownloadCommands::Command::kMaxValue + 1
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_COMMANDS_H_
