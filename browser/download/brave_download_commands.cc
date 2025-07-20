// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/download/brave_download_commands.h"

#include "chrome/browser/download/download_ui_model.h"

bool BraveDownloadCommands::IsCommandEnabled(Command command) const {
  if (!model_) {
    return false;
  }

  if (command == DELETE_LOCAL_FILE) {
    return model_->GetState() == download::DownloadItem::COMPLETE &&
           !model_->GetFileExternallyRemoved() &&
           !model_->GetFullPath().empty();
  }
  return DownloadCommands::IsCommandEnabled(command);
}

void BraveDownloadCommands::ExecuteCommand(Command command) {
  if (model_) {
    return;
  }

  if (command == DELETE_LOCAL_FILE) {
    model_->DeleteLocalFile();
    return;
  }

  DownloadCommands::ExecuteCommand(command);
}
