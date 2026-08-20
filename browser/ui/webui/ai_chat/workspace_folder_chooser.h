// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_WORKSPACE_FOLDER_CHOOSER_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_WORKSPACE_FOLDER_CHOOSER_H_

#include <optional>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "ui/shell_dialogs/select_file_dialog.h"

class Profile;

namespace content {
class WebContents;
}

namespace ai_chat {

// Shows a native "choose folder" dialog for selecting a local workspace
// directory for the AI Chat workspace tools. Single-use per ShowDialog() call.
class WorkspaceFolderChooser : public ui::SelectFileDialog::Listener {
 public:
  using ChooseFolderCallback =
      base::OnceCallback<void(std::optional<base::FilePath>)>;

  WorkspaceFolderChooser(content::WebContents& web_contents, Profile& profile);
  ~WorkspaceFolderChooser() override;

  WorkspaceFolderChooser(const WorkspaceFolderChooser&) = delete;
  WorkspaceFolderChooser& operator=(const WorkspaceFolderChooser&) = delete;

  void ShowDialog(ChooseFolderCallback callback);

  // ui::SelectFileDialog::Listener:
  void FileSelected(const ui::SelectedFileInfo& file, int index) override;
  void FileSelectionCanceled() override;

 private:
  raw_ref<content::WebContents> web_contents_;
  raw_ref<Profile> profile_;
  scoped_refptr<ui::SelectFileDialog> dialog_;
  ChooseFolderCallback callback_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_WORKSPACE_FOLDER_CHOOSER_H_
