// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_page_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

BraveNewTabPageHandler::BraveNewTabPageHandler(
    mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandler>
        pending_page_handler,
    mojo::PendingRemote<brave_new_tab_page::mojom::Page> pending_page,
    Profile* profile,
    NtpCustomBackgroundService* ntp_custom_background_service,
    content::WebContents* web_contents)
    : profile_(profile),
      ntp_custom_background_service_(ntp_custom_background_service),
      web_contents_(web_contents) {
  DCHECK(ntp_custom_background_service_);
  ntp_custom_background_service_observation_.Observe(
      ntp_custom_background_service_);
}

BraveNewTabPageHandler::~BraveNewTabPageHandler() = default;

void BraveNewTabPageHandler::ChooseLocalCustomBackground(
    ChooseLocalCustomBackgroundCallback callback) {
  // Early return if the select file dialog is already active.
  if (select_file_dialog_)
    return;

  select_file_dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(web_contents_));
  ui::SelectFileDialog::FileTypeInfo file_types;
  file_types.allowed_paths = ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH;
  file_types.extensions.resize(1);
  file_types.extensions[0].push_back(FILE_PATH_LITERAL("jpg"));
  file_types.extensions[0].push_back(FILE_PATH_LITERAL("jpeg"));
  file_types.extensions[0].push_back(FILE_PATH_LITERAL("png"));
  file_types.extensions[0].push_back(FILE_PATH_LITERAL("gif"));
  file_types.extension_description_overrides.push_back(
      l10n_util::GetStringUTF16(IDS_UPLOAD_IMAGE_FORMAT));
  choose_local_custom_background_callback_ = std::move(callback);
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_OPEN_FILE, std::u16string(),
      profile_->last_selected_directory(), &file_types, 0,
      base::FilePath::StringType(), web_contents_->GetTopLevelNativeWindow(),
      nullptr);
}

void BraveNewTabPageHandler::UseBraveBackground() {
  if (ntp_custom_background_service_)
    ntp_custom_background_service_->ResetCustomBackgroundInfo();
}

void BraveNewTabPageHandler::OnCustomBackgroundImageUpdated() {
  NOTIMPLEMENTED();
  // Notify page to update background image.
}

void BraveNewTabPageHandler::OnNtpCustomBackgroundServiceShuttingDown() {
  ntp_custom_background_service_observation_.Reset();
  ntp_custom_background_service_ = nullptr;
}

void BraveNewTabPageHandler::FileSelected(const base::FilePath& path,
                                          int index,
                                          void* params) {
  DCHECK(choose_local_custom_background_callback_);
  if (ntp_custom_background_service_) {
    profile_->set_last_selected_directory(path.DirName());
    ntp_custom_background_service_->SelectLocalBackgroundImage(path);
  }

  select_file_dialog_ = nullptr;

  if (choose_local_custom_background_callback_)
    std::move(choose_local_custom_background_callback_).Run(true);
}

void BraveNewTabPageHandler::FileSelectionCanceled(void* params) {
  DCHECK(choose_local_custom_background_callback_);
  select_file_dialog_ = nullptr;

  if (choose_local_custom_background_callback_)
    std::move(choose_local_custom_background_callback_).Run(false);
}
