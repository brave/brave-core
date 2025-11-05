// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_COMMANDS_MACROS_H_
#define BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_COMMANDS_MACROS_H_

// A macro to list all Brave-specific DownloadCommands switch cases. This is
// used in multiple chromium_src files, so we define it here to avoid code
// duplication.
#define BRAVE_DOWNLOAD_COMMANDS_SWITCH_CASES \
  case DownloadCommands::REMOVE_FROM_LIST:   \
  case DownloadCommands::DELETE_LOCAL_FILE:  \
  case DownloadCommands::COPY_DOWNLOAD_LINK

#endif  // BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_COMMANDS_MACROS_H_
