// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/download/bubble/download_bubble_ui_controller.h"

#include "base/metrics/histogram_functions.h"
#include "brave/browser/download/brave_download_commands.h"
#include "chrome/browser/download/download_commands.h"

// Scrubs out the histogramming overload for UmaHistogramEnumeration to
// avoid crash from Brave-specific commands.
#define UmaHistogramEnumeration(...) DoNothing()

// Add switch cases for Brave-specific commands. This will end up calling
// DownloadCommands::ExecuteCommand() for those commands.
#define OPEN_SAFE_BROWSING_SETTING \
  OPEN_SAFE_BROWSING_SETTING:      \
  case DownloadCommands::DELETE_LOCAL_FILE

#include <chrome/browser/download/bubble/download_bubble_ui_controller.cc>

#undef OPEN_SAFE_BROWSING_SETTING
#undef UmaHistogramEnumeration
