// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/download/download_commands.h"

// Add switch-case handling for Brave-specific commands.
// These cases are not used by the DownloadStats, so just fall through.
#define EDIT_WITH_MEDIA_APP                 \
  EDIT_WITH_MEDIA_APP:                      \
  case DownloadCommands::REMOVE_FROM_LIST:  \
  case DownloadCommands::DELETE_LOCAL_FILE: \
  case DownloadCommands::COPY_DOWNLOAD_LINK

#include <chrome/browser/download/download_stats.cc>

#undef EDIT_WITH_MEDIA_APP
