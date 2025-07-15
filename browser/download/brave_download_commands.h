// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_COMMANDS_H_
#define BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_COMMANDS_H_

#include "chrome/browser/download/download_commands.h"

// This class overrides the DownloadCommands class to provide Brave-specific
// command handling. This class is used in DownloadBubbleRowView and
// DownloadShelfView.
class BraveDownloadCommands : public DownloadCommands {
 public:
  // Create brave specific commands set instead of appending to
  // DownloadCommands::Command to avoid many upstream changes.
  // Removes the download item from the list. The actual file is not deleted.
  // Used by download shelf view.
  static constexpr Command kRemoveFromList = Command(Command::kMaxValue + 1);

  // Remove downloaded file from disk and and remove the download item from the
  // list. Used by download bubble view.
  static constexpr Command kDeleteLocalFile = Command(Command::kMaxValue + 2);

  using DownloadCommands::DownloadCommands;
  ~BraveDownloadCommands() override = default;

  // DownloadCommands:
  bool IsCommandEnabled(Command command) const override;
  void ExecuteCommand(Command command) override;
};

#endif  // BRAVE_BROWSER_DOWNLOAD_BRAVE_DOWNLOAD_COMMANDS_H_
