// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_page_handler.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/ntp_background/constants.h"
#include "brave/browser/ntp_background/custom_background_file_manager.h"
#include "brave/browser/ntp_background/ntp_background_prefs.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"
#include "brave/components/brave_search_conversion/p3a.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/brave_search_conversion/types.h"
#include "brave/components/brave_search_conversion/utils.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/template_url.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "ui/base/window_open_disposition_utils.h"
#include "ui/shell_dialogs/selected_file_info.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

bool IsNTPPromotionEnabled(Profile* profile) {
  if (!brave_search_conversion::IsNTPPromotionEnabled(
          profile->GetPrefs(),
          TemplateURLServiceFactory::GetForProfile(profile))) {
    return false;
  }

  auto* service =
      ntp_background_images::ViewCounterServiceFactory::GetForProfile(profile);
  if (!service) {
    return false;
  }

  // Only show promotion if current wallpaper is not sponsored images.
  std::optional<base::Value::Dict> data =
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
      file_manager_(std::make_unique<CustomBackgroundFileManager>(profile_)),
      weak_factory_(this) {
  InitForSearchPromotion();
}

BraveNewTabPageHandler::~BraveNewTabPageHandler() = default;

void BraveNewTabPageHandler::InitForSearchPromotion() {
  // If promotion is disabled for this loading, we do nothing.
  // If some condition is changed and it can be enabled, promotion
  // will be shown at the next NTP loading.
  if (!IsNTPPromotionEnabled(profile_)) {
    return;
  }

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
  if (select_file_dialog_) {
    return;
  }

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
      ui::SelectFileDialog::SELECT_OPEN_MULTI_FILE, std::u16string(),
      profile_->last_selected_directory(), &file_types, 0,
      base::FilePath::StringType(), web_contents_->GetTopLevelNativeWindow(),
      nullptr);
}

void BraveNewTabPageHandler::UseCustomImageBackground(
    const std::string& selected_background) {
  auto decoded_background = selected_background;
  if (!decoded_background.empty()) {
    decoded_background =
        CustomBackgroundFileManager::Converter(GURL(decoded_background))
            .To<std::string>();
  }

  auto pref = NTPBackgroundPrefs(profile_->GetPrefs());
  pref.SetType(NTPBackgroundPrefs::Type::kCustomImage);
  pref.SetSelectedValue(decoded_background);
  pref.SetShouldUseRandomValue(decoded_background.empty());

  OnBackgroundUpdated();
}

void BraveNewTabPageHandler::GetCustomImageBackgrounds(
    GetCustomImageBackgroundsCallback callback) {
  std::vector<brave_new_tab_page::mojom::CustomBackgroundPtr> backgrounds;
  for (const auto& name :
       NTPBackgroundPrefs(profile_->GetPrefs()).GetCustomImageList()) {
    auto value = brave_new_tab_page::mojom::CustomBackground::New();
    value->url = CustomBackgroundFileManager::Converter(name).To<GURL>();
    backgrounds.push_back(std::move(value));
  }

  std::move(callback).Run(std::move(backgrounds));
}

void BraveNewTabPageHandler::RemoveCustomImageBackground(
    const std::string& background) {
  if (background.empty()) {
    return;
  }

  auto file_path = CustomBackgroundFileManager::Converter(GURL(background),
                                                          file_manager_.get())
                       .To<base::FilePath>();
  file_manager_->RemoveImage(
      file_path,
      base::BindOnce(&BraveNewTabPageHandler::OnRemoveCustomImageBackground,
                     weak_factory_.GetWeakPtr(), file_path));
}

void BraveNewTabPageHandler::UseBraveBackground(
    const std::string& selected_background) {
  // Call ntp custom background images service.
  auto pref = NTPBackgroundPrefs(profile_->GetPrefs());
  pref.SetType(NTPBackgroundPrefs::Type::kBrave);
  pref.SetSelectedValue(selected_background);
  pref.SetShouldUseRandomValue(selected_background.empty());

  OnBackgroundUpdated();
}

void BraveNewTabPageHandler::TryBraveSearchPromotion(const std::string& input,
                                                     bool open_new_tab) {
  const GURL promo_url = brave_search_conversion::GetPromoURL(input);
  auto window_open_disposition = WindowOpenDisposition::CURRENT_TAB;
  if (open_new_tab) {
    window_open_disposition = WindowOpenDisposition::NEW_BACKGROUND_TAB;
  }

  web_contents_->OpenURL(
      content::OpenURLParams(
          promo_url, content::Referrer(), window_open_disposition,
          ui::PageTransition::PAGE_TRANSITION_FORM_SUBMIT, false),
      /*navigation_handle_callback=*/{});

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
  if (IsNTPPromotionEnabled(profile_)) {
    return;
  }

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

  OnBackgroundUpdated();
}

void BraveNewTabPageHandler::GetSearchEngines(
    GetSearchEnginesCallback callback) {
  auto* service = TemplateURLServiceFactory::GetForProfile(profile_);
  CHECK(service);

  auto urls = service->GetTemplateURLs();
  std::vector<brave_new_tab_page::mojom::SearchEngineInfoPtr> search_engines;
  for (TemplateURL* template_url : urls) {
    if (template_url->GetBuiltinEngineType() !=
        BuiltinEngineType::KEYWORD_MODE_PREPOPULATED_ENGINE) {
      continue;
    }
    auto search_engine = brave_new_tab_page::mojom::SearchEngineInfo::New();
    search_engine->prepopulate_id = template_url->prepopulate_id();
    search_engine->host = GURL(template_url->url()).host();
    search_engine->name = base::UTF16ToUTF8(template_url->short_name());
    search_engine->keyword = base::UTF16ToUTF8(template_url->keyword());
    search_engine->favicon_url = template_url->favicon_url();
    search_engines.push_back(std::move(search_engine));
  }

  std::move(callback).Run(std::move(search_engines));
}

void BraveNewTabPageHandler::SearchWhatYouTyped(const std::string& host,
                                                const std::string& query,
                                                bool alt_key,
                                                bool ctrl_key,
                                                bool meta_key,
                                                bool shift_key) {
  auto* service = TemplateURLServiceFactory::GetForProfile(profile_);
  CHECK(service);

  auto* template_url = service->GetTemplateURLForHost(host);
  DCHECK(template_url);
  if (!template_url) {
    return;
  }

  GURL search_url = template_url->GenerateSearchURL(
      service->search_terms_data(), base::UTF8ToUTF16(query));

  const WindowOpenDisposition disposition =
      ui::DispositionFromClick(false, alt_key, ctrl_key, meta_key, shift_key);
  content::OpenURLParams params(search_url, content::Referrer(), disposition,
                                ui::PAGE_TRANSITION_FROM_ADDRESS_BAR, false);
  web_contents_->OpenURL(params, /*navigation_handle_callback=*/{});
}

bool BraveNewTabPageHandler::IsCustomBackgroundImageEnabled() const {
  if (profile_->GetPrefs()->IsManagedPreference(
          prefs::kNtpCustomBackgroundDict)) {
    return false;
  }

  return NTPBackgroundPrefs(profile_->GetPrefs()).IsCustomImageType();
}

bool BraveNewTabPageHandler::IsColorBackgroundEnabled() const {
  return NTPBackgroundPrefs(profile_->GetPrefs()).IsColorType();
}

void BraveNewTabPageHandler::OnSavedCustomImage(const base::FilePath& path) {
  if (path.empty()) {
    LOG(ERROR) << "Failed to save custom image";
    return;
  }

  if (brave_new_tab_page::mojom::kMaxCustomImageBackgrounds -
          NTPBackgroundPrefs(profile_->GetPrefs())
              .GetCustomImageList()
              .size() <=
      0) {
    // We can't save more images.
    file_manager_->RemoveImage(path, base::DoNothing());
    return;
  }

  auto file_name =
      CustomBackgroundFileManager::Converter(path).To<std::string>();
  DCHECK(!file_name.empty());

  auto background_pref = NTPBackgroundPrefs(profile_->GetPrefs());
  background_pref.SetType(NTPBackgroundPrefs::Type::kCustomImage);
  background_pref.SetSelectedValue(file_name);
  background_pref.AddCustomImageToList(file_name);
  OnBackgroundUpdated();
  OnCustomImageBackgroundsUpdated();
}

void BraveNewTabPageHandler::OnRemoveCustomImageBackground(
    const base::FilePath& path,
    bool success) {
  if (!success) {
    LOG(ERROR) << "Failed to remove custom image " << path;
    return;
  }

  auto file_name =
      CustomBackgroundFileManager::Converter(path).To<std::string>();
  DCHECK(!file_name.empty());

  auto background_pref = NTPBackgroundPrefs(profile_->GetPrefs());
  background_pref.RemoveCustomImageFromList(file_name);
  if (background_pref.GetType() == NTPBackgroundPrefs::Type::kCustomImage) {
    if (auto custom_images = background_pref.GetCustomImageList();
        !custom_images.empty() &&
        background_pref.GetSelectedValue() == file_name) {
      // Reset to the next candidate after we've removed the chosen one.
      background_pref.SetSelectedValue(custom_images.front());
    } else if (custom_images.empty()) {
      // Reset to default when there's no available custom images.
      background_pref.SetType(NTPBackgroundPrefs::Type::kBrave);
      background_pref.SetSelectedValue({});
      background_pref.SetShouldUseRandomValue(true);
    }
    OnBackgroundUpdated();
  }

  OnCustomImageBackgroundsUpdated();
}

void BraveNewTabPageHandler::OnBackgroundUpdated() {
  if (IsCustomBackgroundImageEnabled()) {
    auto value = brave_new_tab_page::mojom::CustomBackground::New();

    NTPBackgroundPrefs prefs(profile_->GetPrefs());
    auto selected_value = prefs.GetSelectedValue();
    const std::string file_name = selected_value;
    if (!file_name.empty()) {
      value->url = CustomBackgroundFileManager::Converter(file_name).To<GURL>();
    }
    value->use_random_item = prefs.ShouldUseRandomValue();
    page_->OnBackgroundUpdated(
        brave_new_tab_page::mojom::Background::NewCustom(std::move(value)));
    return;
  }

  auto ntp_background_prefs = NTPBackgroundPrefs(profile_->GetPrefs());
  if (IsColorBackgroundEnabled()) {
    auto value = brave_new_tab_page::mojom::CustomBackground::New();
    auto selected_value = ntp_background_prefs.GetSelectedValue();
    value->color = selected_value;
    value->use_random_item = ntp_background_prefs.ShouldUseRandomValue();
    page_->OnBackgroundUpdated(
        brave_new_tab_page::mojom::Background::NewCustom(std::move(value)));
    return;
  }

  DCHECK(ntp_background_prefs.IsBraveType());
  if (ntp_background_prefs.ShouldUseRandomValue()) {
    // Pass empty value for random Brave background.
    page_->OnBackgroundUpdated(nullptr);
    return;
  }

  auto* service = g_brave_browser_process->ntp_background_images_service();
  if (!service) {
    LOG(ERROR) << "No NTP background images service";
    page_->OnBackgroundUpdated(nullptr);
    return;
  }

  auto* image_data = service->GetBackgroundImagesData();
  if (!image_data || !image_data->IsValid()) {
    LOG(ERROR) << "image data is not valid";
    page_->OnBackgroundUpdated(nullptr);
    return;
  }

  auto selected_value = ntp_background_prefs.GetSelectedValue();
  auto image_url = GURL(selected_value);

  auto iter = base::ranges::find_if(
      image_data->backgrounds, [image_data, &image_url](const auto& data) {
        return image_data->url_prefix +
                   data.image_file.BaseName().AsUTF8Unsafe() ==
               image_url.spec();
      });
  if (iter == image_data->backgrounds.end()) {
    page_->OnBackgroundUpdated(nullptr);
    return;
  }

  auto value = brave_new_tab_page::mojom::BraveBackground::New();
  value->image_url = GURL(image_url);
  value->author = iter->author;
  value->link = GURL(iter->link);
  page_->OnBackgroundUpdated(
      brave_new_tab_page::mojom::Background::NewBrave(std::move(value)));
}

void BraveNewTabPageHandler::OnCustomImageBackgroundsUpdated() {
  std::vector<brave_new_tab_page::mojom::CustomBackgroundPtr> backgrounds;
  for (const auto& name :
       NTPBackgroundPrefs(profile_->GetPrefs()).GetCustomImageList()) {
    auto value = brave_new_tab_page::mojom::CustomBackground::New();
    value->url = CustomBackgroundFileManager::Converter(name).To<GURL>();
    backgrounds.push_back(std::move(value));
  }

  page_->OnCustomImageBackgroundsUpdated(std::move(backgrounds));
}

void BraveNewTabPageHandler::FileSelected(const ui::SelectedFileInfo& file,
                                          int index) {
  profile_->set_last_selected_directory(file.path().DirName());

  file_manager_->SaveImage(
      file.path(), base::BindOnce(&BraveNewTabPageHandler::OnSavedCustomImage,
                                  weak_factory_.GetWeakPtr()));

  select_file_dialog_ = nullptr;
}

void BraveNewTabPageHandler::MultiFilesSelected(
    const std::vector<ui::SelectedFileInfo>& files) {
  NTPBackgroundPrefs prefs(profile_->GetPrefs());
  auto available_image_count =
      brave_new_tab_page::mojom::kMaxCustomImageBackgrounds -
      prefs.GetCustomImageList().size();
  for (const auto& file : files) {
    if (available_image_count == 0) {
      break;
    }

    FileSelected(file, 0);
    available_image_count--;
  }
}

void BraveNewTabPageHandler::FileSelectionCanceled() {
  select_file_dialog_ = nullptr;
}

void BraveNewTabPageHandler::OnTemplateURLServiceChanged() {
  NotifySearchPromotionDisabledIfNeeded();
}

void BraveNewTabPageHandler::OnTemplateURLServiceShuttingDown() {
  template_url_service_observation_.Reset();
}

void BraveNewTabPageHandler::GetBraveBackgrounds(
    GetBraveBackgroundsCallback callback) {
  auto* service = g_brave_browser_process->ntp_background_images_service();
  if (!service) {
    LOG(ERROR) << "No NTP background images service";
    std::move(callback).Run({});
    return;
  }

  auto* image_data = service->GetBackgroundImagesData();
  if (!image_data || !image_data->IsValid()) {
    LOG(ERROR) << "image data is not valid";
    std::move(callback).Run({});
    return;
  }

  std::vector<brave_new_tab_page::mojom::BraveBackgroundPtr> backgrounds;
  base::ranges::transform(
      image_data->backgrounds, std::back_inserter(backgrounds),
      [image_data](const auto& data) {
        auto value = brave_new_tab_page::mojom::BraveBackground::New();
        value->image_url = GURL(image_data->url_prefix +
                                data.image_file.BaseName().AsUTF8Unsafe());
        value->author = data.author;
        value->link = GURL(data.link);
        return value;
      });

  std::move(callback).Run(std::move(backgrounds));
}
