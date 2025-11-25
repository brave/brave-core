// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/download/download_commands.h"
#include "chrome/grit/generated_resources.h"

// Add switch-case handling for Brave-specific commands.
// These cases are not used by the DownloadUIModel, so just fall through.
#define EDIT_WITH_MEDIA_APP                \
  EDIT_WITH_MEDIA_APP:                     \
  case DownloadCommands::REMOVE_FROM_LIST: \
  case DownloadCommands::DELETE_LOCAL_FILE

// There's inconsistency between download bubble and brave://downloads page
// in showing "File removed" status for removed files. To keep the
// consistency, redefine the resource ID here. As a result, both download bubble
// and brave://downloads page will show "Deleted"for removed files.
#undef IDS_DOWNLOAD_STATUS_REMOVED
#define IDS_DOWNLOAD_STATUS_REMOVED IDS_DOWNLOAD_FILE_REMOVED

#include <chrome/browser/download/download_ui_model.cc>

#undef IDS_DOWNLOAD_STATUS_REMOVED
#undef EDIT_WITH_MEDIA_APP
