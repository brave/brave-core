// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/download/brave_download_commands_macros.h"
#include "chrome/browser/download/download_commands.h"

// Add switch-case handling for Brave-specific commands.
// These cases are not used by the DownloadUIModel, so just fall through.
#define EDIT_WITH_MEDIA_APP \
  EDIT_WITH_MEDIA_APP:      \
  BRAVE_DOWNLOAD_COMMANDS_SWITCH_CASES

#include <chrome/browser/download/download_ui_model.cc>

#undef EDIT_WITH_MEDIA_APP
