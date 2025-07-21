// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/download/download_commands.h"

// Append another command that we added to the list of commands.
#define OPEN_WHEN_COMPLETE \
  OPEN_WHEN_COMPLETE, DownloadCommands::Command::DELETE_LOCAL_FILE

#include "src/chrome/browser/ui/download/download_bubble_row_view_info_unittest.cc"

#undef OPEN_WHEN_COMPLETE
