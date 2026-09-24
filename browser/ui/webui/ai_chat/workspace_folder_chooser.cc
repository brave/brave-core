// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/workspace_folder_chooser.h"

#include <utility>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/select_file_policy/chrome_select_file_policy.h"
#include "content/public/browser/web_contents.h"
#include "ui/shell_dialogs/selected_file_info.h"

namespace ai_chat {

WorkspaceFolderChooser::WorkspaceFolderChooser(
    content::WebContents& web_contents,
    Profile& profile)
    : web_contents_(web_contents), profile_(profile) {}

WorkspaceFolderChooser::~WorkspaceFolderChooser() {
  if (dialog_) {
    dialog_->ListenerDestroyed();
  }
}

void WorkspaceFolderChooser::ShowDialog(ChooseFolderCallback callback) {
  // Only one dialog at a time; cancel any pending callback.
  if (callback_) {
    std::move(callback_).Run(std::nullopt);
  }
  callback_ = std::move(callback);

  dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(&web_contents_.get()));
  dialog_->SelectFile(ui::SelectFileDialog::SELECT_EXISTING_FOLDER,
                      std::u16string(), profile_->last_selected_directory(),
                      /*file_types=*/nullptr, 0, base::FilePath::StringType(),
                      web_contents_->GetTopLevelNativeWindow(), nullptr);
}

void WorkspaceFolderChooser::FileSelected(const ui::SelectedFileInfo& file,
                                          int index) {
  dialog_.reset();
  profile_->set_last_selected_directory(file.path());
  if (callback_) {
    std::move(callback_).Run(file.path());
  }
}

void WorkspaceFolderChooser::FileSelectionCanceled() {
  dialog_.reset();
  if (callback_) {
    std::move(callback_).Run(std::nullopt);
  }
}

}  // namespace ai_chat
