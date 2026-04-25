/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_COMMANDS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_COMMANDS_H_

// Extend the Command enum to include Brave-specific commands.
// Note that we keep Command::kMaxValue as the last value of Chromium's max
// value as it's used for histogramming purposes. We don't want our commands to
// be counted in.
#define OPEN_WITH_MEDIA_APP                                             \
  /* Removes the download item from the list. The actual file is not */ \
  /* deleted. Used by download shelf view. */                           \
  REMOVE_FROM_LIST = 23,                                                       \
  /* Remove downloaded file from disk and and remove the download item from */ \
  /* the list. Used by download bubble view. */                                \
  DELETE_LOCAL_FILE = 24,                                                      \
  /* Copy download link to clipboard from DownloadUIContextMenuView */         \
  COPY_DOWNLOAD_LINK = 25,                                                  \
  OPEN_WITH_MEDIA_APP

#include <chrome/browser/download/download_commands.h>  // IWYU pragma: export

#undef OPEN_WITH_MEDIA_APP

static_assert(DownloadCommands::Command::kMaxValue ==
                  DownloadCommands::Command::EDIT_WITH_MEDIA_APP,
              "We should update kRemoveFromList and kDeleteLocalFile values if "
              "we change the max value of DownloadCommands::Command");

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_DOWNLOAD_DOWNLOAD_COMMANDS_H_
