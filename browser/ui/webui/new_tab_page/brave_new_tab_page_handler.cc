// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_page_handler.h"

#include <utility>

#include "base/bind.h"
#include "base/containers/span.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ntp_background_images/constants.h"
#include "brave/common/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "chrome/browser/image_fetcher/image_decoder_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

BraveNewTabPageHandler::BraveNewTabPageHandler(
    mojo::PendingReceiver<brave_new_tab_page::mojom::PageHandler>
        pending_page_handler,
    mojo::PendingRemote<brave_new_tab_page::mojom::Page> pending_page,
    Profile* profile,
    content::WebContents* web_contents)
    : page_handler_(this, std::move(pending_page_handler)),
      page_(std::move(pending_page)),
      profile_(profile),
      web_contents_(web_contents),
      weak_factory_(this) {}

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
      brave_l10n::GetLocalizedResourceUTF16String(IDS_UPLOAD_IMAGE_FORMAT));
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_OPEN_FILE, std::u16string(),
      profile_->last_selected_directory(), &file_types, 0,
      base::FilePath::StringType(), web_contents_->GetTopLevelNativeWindow(),
      nullptr);
}

void BraveNewTabPageHandler::UseBraveBackground() {
  // Call ntp custom background images service.
  profile_->GetPrefs()->SetBoolean(kNewTabPageCustomBackgroundEnabled, false);
  OnCustomBackgroundImageUpdated();
  DeleteSanitizedImageFile();
}

bool BraveNewTabPageHandler::IsCustomBackgroundEnabled() const {
  auto* prefs = profile_->GetPrefs();
  if (prefs->IsManagedPreference(prefs::kNtpCustomBackgroundDict))
    return false;

  return prefs->GetBoolean(kNewTabPageCustomBackgroundEnabled);
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

void BraveNewTabPageHandler::FileSelected(const base::FilePath& path,
                                          int index,
                                          void* params) {
  profile_->set_last_selected_directory(path.DirName());
  // By saving sanitized image, we don't need to do it whenever
  // NTP opens.
  ConvertSelectedImageFileAndSave(path);
  select_file_dialog_ = nullptr;
}

void BraveNewTabPageHandler::FileSelectionCanceled(void* params) {
  select_file_dialog_ = nullptr;
}

void BraveNewTabPageHandler::ConvertSelectedImageFileAndSave(
    const base::FilePath& image_file) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(
          [](const base::FilePath& image_file_path) {
            std::string contents;
            if (!base::ReadFileToString(image_file_path, &contents))
              return absl::optional<std::string>();
            return absl::optional<std::string>(contents);
          },
          image_file),
      base::BindOnce(&BraveNewTabPageHandler::OnGotImageFile,
                     weak_factory_.GetWeakPtr()));
}

void BraveNewTabPageHandler::OnGotImageFile(absl::optional<std::string> input) {
  if (!input)
    return;

  // Send image body to image decoder in isolated process.
  GetImageDecoder()->DecodeImage(
      *input, gfx::Size() /* No particular size desired. */,
      base::BindOnce(&BraveNewTabPageHandler::OnImageDecoded,
                     weak_factory_.GetWeakPtr()));
}

void BraveNewTabPageHandler::OnImageDecoded(const gfx::Image& image) {
  // Re-encode vetted image as PNG and save.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(
          [](const SkBitmap& bitmap, const base::FilePath& target_file_path) {
            auto encoded = base::MakeRefCounted<base::RefCountedBytes>();
            if (!gfx::PNGCodec::EncodeBGRASkBitmap(
                    bitmap, /*discard_transparency=*/false, &encoded->data())) {
              return false;
            }
            return base::WriteFile(
                target_file_path,
                base::span<const uint8_t>(encoded->front(), encoded->size()));
          },
          image.AsBitmap(), GetSanitizedImageFilePath()),
      base::BindOnce(&BraveNewTabPageHandler::OnSavedEncodedImage,
                     weak_factory_.GetWeakPtr()));
}

void BraveNewTabPageHandler::OnSavedEncodedImage(bool success) {
  if (!success)
    return;

  profile_->GetPrefs()->SetBoolean(kNewTabPageCustomBackgroundEnabled, true);
  OnCustomBackgroundImageUpdated();
}

void BraveNewTabPageHandler::DeleteSanitizedImageFile() {
  base::ThreadPool::PostTask(FROM_HERE, {base::MayBlock()},
                             base::BindOnce(base::GetDeleteFileCallback(),
                                            GetSanitizedImageFilePath()));
}

base::FilePath BraveNewTabPageHandler::GetSanitizedImageFilePath() const {
  return profile_->GetPath().AppendASCII(
      ntp_background_images::kSanitizedImageFileName);
}

image_fetcher::ImageDecoder* BraveNewTabPageHandler::GetImageDecoder() {
  if (!image_decoder_)
    image_decoder_ = std::make_unique<ImageDecoderImpl>();
  return image_decoder_.get();
}
