// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab/custom_image_chooser.h"

#include <utility>

#include "brave/components/l10n/common/localization_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_contents.h"
#include "ui/shell_dialogs/selected_file_info.h"

namespace brave_new_tab {

CustomImageChooser::CustomImageChooser(content::WebContents& web_contents)
    : web_contents_(web_contents),
      profile_(*Profile::FromBrowserContext(web_contents.GetBrowserContext())) {
}

CustomImageChooser::~CustomImageChooser() = default;

void CustomImageChooser::ShowDialog(ShowDialogCallback callback) {
  if (callback_) {
    std::move(callback_).Run({});
  }

  callback_ = std::move(callback);

  if (dialog_) {
    return;
  }

  dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(&web_contents_.get()));

  ui::SelectFileDialog::FileTypeInfo file_types;
  file_types.allowed_paths = ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH;
  file_types.extensions.push_back(
      {{FILE_PATH_LITERAL("jpg"), FILE_PATH_LITERAL("jpeg"),
        FILE_PATH_LITERAL("png"), FILE_PATH_LITERAL("gif")}});
  file_types.extension_description_overrides.push_back(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_UPLOAD_IMAGE_FORMAT));

  dialog_->SelectFile(ui::SelectFileDialog::SELECT_OPEN_MULTI_FILE,
                      std::u16string(), profile_->last_selected_directory(),
                      &file_types, 0, base::FilePath::StringType(),
                      web_contents_->GetTopLevelNativeWindow(), nullptr);
}

void CustomImageChooser::FileSelected(const ui::SelectedFileInfo& file,
                                      int index) {
  dialog_ = nullptr;
  profile_->set_last_selected_directory(file.path().DirName());
  if (callback_) {
    std::move(callback_).Run({file.path()});
  }
}

void CustomImageChooser::MultiFilesSelected(
    const std::vector<ui::SelectedFileInfo>& files) {
  dialog_ = nullptr;
  if (!files.empty()) {
    profile_->set_last_selected_directory(files.back().path().DirName());
  }
  std::vector<base::FilePath> paths;
  paths.reserve(files.size());
  for (auto& file : files) {
    paths.push_back(file.path());
  }
  if (callback_) {
    std::move(callback_).Run(std::move(paths));
  }
}

void CustomImageChooser::FileSelectionCanceled() {
  dialog_ = nullptr;
  if (callback_) {
    std::move(callback_).Run({});
  }
}

}  // namespace brave_new_tab
