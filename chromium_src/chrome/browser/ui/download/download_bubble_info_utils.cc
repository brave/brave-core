// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/download/download_commands.h"
#include "components/vector_icons/vector_icons.h"

// Add an action to deleted local file in QuickActionsForDownload. This is added
// only when the download can be opened. That's why we're defining
// kLaunchChromeRefreshIcon.
#define kLaunchChromeRefreshIcon kLaunchChromeRefreshIcon);           \
  actions.emplace_back<DownloadCommands::Command>(                        \
      DownloadCommands::Command(DownloadCommands::DELETE_LOCAL_FILE), \
      l10n_util::GetStringUTF16(IDS_DOWNLOAD_BUBBLE_DELETE),   \
      &kLeoTrashIcon

#include <chrome/browser/ui/download/download_bubble_info_utils.cc>

#undef kLaunchChromeRefreshIcon
