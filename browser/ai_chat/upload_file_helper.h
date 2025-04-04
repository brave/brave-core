// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_UPLOAD_FILE_HELPER_H_
#define BRAVE_BROWSER_AI_CHAT_UPLOAD_FILE_HELPER_H_

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/shell_dialogs/select_file_policy.h"

namespace content {
class WebContents;
}

class Profile;

namespace ai_chat {

class UploadFileHelper : public ui::SelectFileDialog::Listener {
 public:
  UploadFileHelper(content::WebContents* web_contents, Profile* profile);
  ~UploadFileHelper() override;
  UploadFileHelper(const UploadFileHelper&) = delete;
  UploadFileHelper& operator=(const UploadFileHelper&) = delete;

  void UploadImage(std::unique_ptr<ui::SelectFilePolicy> policy,
#if BUILDFLAG(IS_ANDROID)
                   bool use_media_capture,
#endif
                   mojom::AIChatUIHandler::UploadImageCallback callback);

 private:
  // ui::SelectFileDialog::Listener
  void FileSelected(const ui::SelectedFileInfo& file, int index) override;
  void MultiFilesSelected(
      const std::vector<ui::SelectedFileInfo>& files) override;
  void FileSelectionCanceled() override;

  void OnImageRead(
      std::tuple<std::optional<std::vector<uint8_t>>, std::string> result);
  void OnImageSanitized(std::string filename, const SkBitmap& decoded_bitmap);
  void OnImageEncoded(std::string filename,
                      std::optional<std::vector<uint8_t>> output);

  raw_ptr<content::WebContents> web_contents_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  mojom::AIChatUIHandler::UploadImageCallback upload_image_callback_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<UploadFileHelper> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_UPLOAD_FILE_HELPER_H_
