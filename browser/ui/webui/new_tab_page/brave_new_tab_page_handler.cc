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
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/browser/ntp_background/constants.h"
#include "brave/browser/ntp_background/ntp_background_prefs.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/components/brave_search_conversion/p3a.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/image_fetcher/image_decoder_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

namespace {

data_decoder::DataDecoder* GetDataDecoder() {
  static base::NoDestructor<data_decoder::DataDecoder> data_decoder;
  return data_decoder.get();
}

bool IsNTPPromotionEnabled(Profile* profile) {
  if (!brave_search_conversion::IsNTPPromotionEnabled(
          profile->GetPrefs(),
          TemplateURLServiceFactory::GetForProfile(profile))) {
    return false;
  }

  auto* service =
      ntp_background_images::ViewCounterServiceFactory::GetForProfile(profile);
  if (!service)
    return false;

  // Only show promotion if current wallpaper is not sponsored images.
  absl::optional<base::Value::Dict> data =
      service->GetCurrentWallpaperForDisplay();
  if (data) {
    if (const auto is_background =
            data->FindBool(ntp_background_images::kIsBackgroundKey)) {
      return is_background.value();
    }
  }
  return false;
}

}  // namespace

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
      weak_factory_(this) {
  InitForSearchPromotion();
}

BraveNewTabPageHandler::~BraveNewTabPageHandler() = default;

void BraveNewTabPageHandler::InitForSearchPromotion() {
  // If promotion is disabled for this loading, we do nothing.
  // If some condition is changed and it can be enabled, promotion
  // will be shown at the next NTP loading.
  if (!IsNTPPromotionEnabled(profile_))
    return;

  // Observing user's dismiss or default search provider change to hide
  // promotion from NTP while NTP is loaded.
  pref_change_registrar_.Init(profile_->GetPrefs());
  pref_change_registrar_.Add(
      brave_search_conversion::prefs::kDismissed,
      base::BindRepeating(&BraveNewTabPageHandler::OnSearchPromotionDismissed,
                          base::Unretained(this)));
  template_url_service_observation_.Observe(
      TemplateURLServiceFactory::GetForProfile(profile_));

  brave_search_conversion::p3a::RecordPromoShown(
      g_browser_process->local_state(),
      brave_search_conversion::ConversionType::kNTP);
}

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
  NTPBackgroundPrefs(profile_->GetPrefs())
      .SetType(NTPBackgroundPrefs::Type::kBrave);
  OnCustomBackgroundUpdated();
  DeleteSanitizedImageFile();
}

void BraveNewTabPageHandler::TryBraveSearchPromotion(const std::string& input,
                                                     bool open_new_tab) {
  const GURL promo_url = brave_search_conversion::GetPromoURL(input);
  auto window_open_disposition = WindowOpenDisposition::CURRENT_TAB;
  if (open_new_tab) {
    window_open_disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
  }

  web_contents_->OpenURL(content::OpenURLParams(
      promo_url, content::Referrer(), window_open_disposition,
      ui::PageTransition::PAGE_TRANSITION_FORM_SUBMIT, false));

  brave_search_conversion::p3a::RecordPromoTrigger(
      g_browser_process->local_state(),
      brave_search_conversion::ConversionType::kNTP);
}

void BraveNewTabPageHandler::DismissBraveSearchPromotion() {
  brave_search_conversion::SetDismissed(profile_->GetPrefs());
}

void BraveNewTabPageHandler::IsSearchPromotionEnabled(
    IsSearchPromotionEnabledCallback callback) {
  std::move(callback).Run(IsNTPPromotionEnabled(profile_));
}

void BraveNewTabPageHandler::NotifySearchPromotionDisabledIfNeeded() const {
  // If enabled, we don't do anything. When NTP is reloaded or opened,
  // user will see promotion.
  if (IsNTPPromotionEnabled(profile_))
    return;

  // Hide promotion when it's disabled.
  page_->OnSearchPromotionDisabled();
}

void BraveNewTabPageHandler::OnSearchPromotionDismissed() {
  NotifySearchPromotionDisabledIfNeeded();
}

void BraveNewTabPageHandler::UseColorBackground(const std::string& color,
                                                bool use_random_color) {
  if (use_random_color) {
    DCHECK(color == brave_new_tab_page::mojom::kRandomSolidColorValue ||
           color == brave_new_tab_page::mojom::kRandomGradientColorValue)
        << "When |use_random_color| is true, |color| should be "
           "kRandomSolidColorValue or kRandomGradientColorValue";
  }

  auto background_pref = NTPBackgroundPrefs(profile_->GetPrefs());
  background_pref.SetType(NTPBackgroundPrefs::Type::kColor);
  background_pref.SetSelectedValue(color);
  background_pref.SetShouldUseRandomValue(use_random_color);

  OnCustomBackgroundUpdated();
  DeleteSanitizedImageFile();
}

bool BraveNewTabPageHandler::IsCustomBackgroundImageEnabled() const {
  auto* prefs = profile_->GetPrefs();
  if (prefs->IsManagedPreference(prefs::kNtpCustomBackgroundDict))
    return false;

  return NTPBackgroundPrefs(prefs).IsCustomImageType();
}

bool BraveNewTabPageHandler::IsColorBackgroundEnabled() const {
  return NTPBackgroundPrefs(profile_->GetPrefs()).IsColorType();
}

void BraveNewTabPageHandler::OnCustomBackgroundUpdated() {
  brave_new_tab_page::mojom::CustomBackgroundPtr value =
      brave_new_tab_page::mojom::CustomBackground::New();
  // Pass empty struct when custom background is disabled.
  if (IsCustomBackgroundImageEnabled()) {
    // Add a timestamp to the url to prevent the browser from using a cached
    // version when "Upload an image" is used multiple times.
    std::string time_string = std::to_string(base::Time::Now().ToTimeT());
    std::string local_string(ntp_background_images::kCustomWallpaperURL);
    value->url = GURL(local_string + "?ts=" + time_string);
  } else if (IsColorBackgroundEnabled()) {
    auto ntp_background_prefs = NTPBackgroundPrefs(profile_->GetPrefs());
    auto selected_value = ntp_background_prefs.GetSelectedValue();
    DCHECK(absl::holds_alternative<std::string>(selected_value));
    value->color = absl::get<std::string>(selected_value);
    value->use_random_item = ntp_background_prefs.ShouldUseRandomValue();
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

void BraveNewTabPageHandler::OnTemplateURLServiceChanged() {
  NotifySearchPromotionDisabledIfNeeded();
}

void BraveNewTabPageHandler::OnTemplateURLServiceShuttingDown() {
  template_url_service_observation_.Reset();
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
      *input, gfx::Size() /* No particular size desired. */, GetDataDecoder(),
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

  NTPBackgroundPrefs(profile_->GetPrefs())
      .SetType(NTPBackgroundPrefs::Type::kCustomImage);
  OnCustomBackgroundUpdated();
}

void BraveNewTabPageHandler::DeleteSanitizedImageFile() {
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock()},
      base::GetDeleteFileCallback(GetSanitizedImageFilePath()));
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
