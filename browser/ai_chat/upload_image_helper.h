// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/shell_dialogs/select_file_policy.h"

namespace content {
class WebContents;
}

class Profile;

namespace ai_chat {

class UploadImageHelper : ui::SelectFileDialog::Listener {
 public:
  UploadImageHelper(content::WebContents* web_contents, Profile* profile);
  ~UploadImageHelper() override;

  void UploadImage(std::unique_ptr<ui::SelectFilePolicy> policy,
                   mojom::AIChatUIHandler::UploadImageCallback callback);

 private:
  // ui::SelectFileDialog::Listener
  void FileSelected(const ui::SelectedFileInfo& file, int index) override;
  void FileSelectionCanceled() override;

  raw_ptr<content::WebContents> web_contents_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  mojom::AIChatUIHandler::UploadImageCallback upload_image_callback_;
};

}  // namespace ai_chat
