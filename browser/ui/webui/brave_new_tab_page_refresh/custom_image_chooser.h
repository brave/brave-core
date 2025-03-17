// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_CUSTOM_IMAGE_CHOOSER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_CUSTOM_IMAGE_CHOOSER_H_

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "ui/shell_dialogs/select_file_dialog.h"

class Profile;

namespace content {
class WebContents;
}

namespace brave_new_tab_page_refresh {

// Displays a file chooser dialog for use on the New Tab Page, allowing the user
// to select background images from their device.
class CustomImageChooser : public ui::SelectFileDialog::Listener {
 public:
  CustomImageChooser(content::WebContents& web_contents, Profile& profile);
  ~CustomImageChooser() override;

  CustomImageChooser(const CustomImageChooser&) = delete;
  CustomImageChooser& operator=(const CustomImageChooser&) = delete;

  using ShowDialogCallback =
      base::OnceCallback<void(std::vector<base::FilePath>)>;

  void ShowDialog(ShowDialogCallback callback);

  // ui::SelectFileDialog::Listener:
  void FileSelected(const ui::SelectedFileInfo& file, int index) override;
  void MultiFilesSelected(
      const std::vector<ui::SelectedFileInfo>& files) override;
  void FileSelectionCanceled() override;

 private:
  raw_ref<content::WebContents> web_contents_;
  raw_ref<Profile> profile_;
  scoped_refptr<ui::SelectFileDialog> dialog_;
  ShowDialogCallback callback_;
};

}  // namespace brave_new_tab_page_refresh

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_CUSTOM_IMAGE_CHOOSER_H_
