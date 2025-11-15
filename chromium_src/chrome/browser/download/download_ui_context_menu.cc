// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/download/download_ui_context_menu.h"

#include "brave/browser/download/brave_download_commands_macros.h"
#include "chrome/browser/download/download_commands.h"

#define RecordCommandsEnabled RecordCommandsEnabled_Unused

// Add switch-case handling for Brave-specific commands.
// These cases are not used by the DownloadUIContextMenu, so just fall
// through.
#define EDIT_WITH_MEDIA_APP \
  EDIT_WITH_MEDIA_APP:      \
  BRAVE_DOWNLOAD_COMMANDS_SWITCH_CASES

// Replace DownloadCommands occurrences with BraveDownloadCommands
#define DownloadCommands BraveDownloadCommands

#include <chrome/browser/download/download_ui_context_menu.cc>

#undef DownloadCommands
#undef EDIT_WITH_MEDIA_APP
#undef RecordCommandsEnabled
