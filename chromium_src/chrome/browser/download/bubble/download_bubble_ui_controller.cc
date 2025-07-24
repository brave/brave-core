// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/download/bubble/download_bubble_ui_controller.h"

#include "brave/browser/download/brave_download_commands.h"

#define ProcessDownloadButtonPress ProcessDownloadButtonPress_ChromiumImpl

#include <chrome/browser/download/bubble/download_bubble_ui_controller.cc>

#undef ProcessDownloadButtonPress

void DownloadBubbleUIController::ProcessDownloadButtonPress(
    base::WeakPtr<DownloadUIModel> model,
    DownloadCommands::Command command,
    bool is_main_view) {
  if (!model) {
    return;
  }

  if (command == BraveDownloadCommands::DELETE_LOCAL_FILE) {
    BraveDownloadCommands commands(model);
    commands.ExecuteCommand(command);
    return;
  }
  ProcessDownloadButtonPress_ChromiumImpl(model, command, is_main_view);
}
