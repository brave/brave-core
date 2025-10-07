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

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "ui/shell_dialogs/select_file_dialog.h"
#include "ui/shell_dialogs/select_file_policy.h"

namespace content {
class WebContents;
}

class Profile;

namespace ai_chat {

class UploadFileHelper : public ui::SelectFileDialog::Listener {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnFilesSelected() {}

   protected:
    ~Observer() override = default;
  };

  UploadFileHelper(content::WebContents* web_contents, Profile* profile);
  ~UploadFileHelper() override;
  UploadFileHelper(const UploadFileHelper&) = delete;
  UploadFileHelper& operator=(const UploadFileHelper&) = delete;

  void UploadFile(std::unique_ptr<ui::SelectFilePolicy> policy,
#if BUILDFLAG(IS_ANDROID)
                  bool use_media_capture,
#endif
                  mojom::AIChatUIHandler::UploadFileCallback callback);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Process image data (sanitization and resizing)
  static void ProcessImageData(
      data_decoder::DataDecoder* data_decoder,
      const std::vector<uint8_t>& image_data,
      base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> callback);

 private:
  // ui::SelectFileDialog::Listener
  void FileSelected(const ui::SelectedFileInfo& file, int index) override;
  void MultiFilesSelected(
      const std::vector<ui::SelectedFileInfo>& files) override;
  void FileSelectionCanceled() override;

  void OnFileRead(
      std::tuple<std::optional<std::vector<uint8_t>>, base::FilePath> result);
  void OnImageEncoded(std::string filename,
                      std::optional<std::vector<uint8_t>> output);

  base::ObserverList<Observer> observers_;
  raw_ptr<content::WebContents> web_contents_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
  scoped_refptr<ui::SelectFileDialog> select_file_dialog_;
  mojom::AIChatUIHandler::UploadFileCallback upload_file_callback_;

  // DataDecoder instance for processing image data
  data_decoder::DataDecoder data_decoder_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<UploadFileHelper> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_UPLOAD_FILE_HELPER_H_
