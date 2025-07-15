/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_COMMANDS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_COMMANDS_H_

// Make methods overridable for BraveDownloadCommands class.
#define IsCommandEnabled virtual IsCommandEnabled
#define ExecuteCommand                \
  ExecuteCommand_Unused() {}          \
  friend class BraveDownloadCommands; \
  virtual void ExecuteCommand

#include "src/chrome/browser/download/download_commands.h"  // IWYU pragma: export

#undef ExecuteCommand
#undef IsCommandEnabled

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_COMMANDS_H_
