// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_page_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/background/ntp_background_data.h"
#include "chrome/browser/search/background/ntp_custom_background_service.h"
#include "chrome/browser/search/background/ntp_custom_background_service_factory.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

BraveNewTabPageHandler::BraveNewTabPageHandler(
    mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandler>
        pending_page_handler,
    mojo::PendingRemote<brave_new_tab_page::mojom::Page> pending_page,
    Profile* profile,
    NtpCustomBackgroundService* ntp_custom_background_service,
    content::WebContents* web_contents)
    : page_handler_(this, std::move(pending_page_handler)),
      page_(std::move(pending_page)),
      profile_(profile),
      ntp_custom_background_service_(ntp_custom_background_service),
      web_contents_(web_contents) {
  DCHECK(ntp_custom_background_service_);
  ntp_custom_background_service_observation_.Observe(
      ntp_custom_background_service_);
}

BraveNewTabPageHandler::~BraveNewTabPageHandler() = default;

void BraveNewTabPageHandler::ChooseLocalCustomBackground() {
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

bool BraveNewTabPageHandler::IsCustomBackgroundEnabled() const {
  auto* service = NtpCustomBackgroundServiceFactory::GetForProfile(profile_);
  if (service->IsCustomBackgroundDisabledByPolicy())
    return false;

  return profile_->GetPrefs()->GetBoolean(
      prefs::kNtpCustomBackgroundLocalToDevice);
}

void BraveNewTabPageHandler::OnCustomBackgroundImageUpdated() {
  brave_new_tab_page::mojom::CustomBackgroundPtr value =
      brave_new_tab_page::mojom::CustomBackground::New();
  // Pass empty struct when custom background is disabled.
  if (IsCustomBackgroundEnabled()) {
    // Add a timestamp to the url to prevent the browser from using a cached
    // version when "Upload an image" is used multiple times.
    std::string time_string = std::to_string(base::Time::Now().ToTimeT());
    std::string local_string(ntp_background_images::kCustomWallpaperURL);
    value->url = GURL(local_string + "?ts=" + time_string);
  }
  page_->OnBackgroundUpdated(std::move(value));
}

void BraveNewTabPageHandler::OnNtpCustomBackgroundServiceShuttingDown() {
  ntp_custom_background_service_observation_.Reset();
  ntp_custom_background_service_ = nullptr;
}

void BraveNewTabPageHandler::FileSelected(const base::FilePath& path,
                                          int index,
                                          void* params) {
  if (ntp_custom_background_service_) {
    profile_->set_last_selected_directory(path.DirName());
    ntp_custom_background_service_->SelectLocalBackgroundImage(path);
  }

  select_file_dialog_ = nullptr;
}

void BraveNewTabPageHandler::FileSelectionCanceled(void* params) {
  select_file_dialog_ = nullptr;
}
