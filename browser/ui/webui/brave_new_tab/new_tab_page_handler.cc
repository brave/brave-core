// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab/new_tab_page_handler.h"

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/webui/brave_new_tab/background_adapter.h"
#include "brave/browser/ui/webui/brave_new_tab/custom_image_chooser.h"
#include "brave/components/brave_private_cdn/private_cdn_helper.h"
#include "brave/components/brave_private_cdn/private_cdn_request_helper.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/browser/themes/theme_syncable_service.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/public/tab_interface.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/template_url_service.h"
#include "services/network/public/cpp/header_util.h"
#include "ui/base/window_open_disposition_utils.h"
#include "url/gurl.h"

namespace brave_new_tab {

NewTabPageHandler::NewTabPageHandler(
    mojo::PendingReceiver<mojom::NewTabPageHandler> receiver,
    std::unique_ptr<CustomImageChooser> custom_image_chooser,
    std::unique_ptr<BackgroundAdapter> background_adapter,
    std::unique_ptr<brave_private_cdn::PrivateCDNRequestHelper> pcdn_helper,
    tabs::TabInterface& tab,
    PrefService& pref_service,
    TemplateURLService& template_url_service)
    : receiver_(this, std::move(receiver)),
      update_observer_(pref_service),
      custom_image_chooser_(std::move(custom_image_chooser)),
      background_adapter_(std::move(background_adapter)),
      pcdn_helper_(std::move(pcdn_helper)),
      tab_(tab),
      pref_service_(pref_service),
      template_url_service_(template_url_service) {
  CHECK(custom_image_chooser_);
  CHECK(background_adapter_);
  CHECK(pcdn_helper_);

  update_observer_.SetCallback(base::BindRepeating(&NewTabPageHandler::OnUpdate,
                                                   weak_factory_.GetWeakPtr()));
}

NewTabPageHandler::~NewTabPageHandler() = default;

void NewTabPageHandler::SetNewTabPage(
    mojo::PendingRemote<mojom::NewTabPage> page) {
  page_.reset();
  page_.Bind(std::move(page));
}

void NewTabPageHandler::LoadResourceFromPcdn(
    const std::string& url,
    LoadResourceFromPcdnCallback callback) {
  GURL resource_url(url);
  if (!resource_url.is_valid()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  auto on_resource_downloaded = [](decltype(callback) callback, bool is_padded,
                                   int response_code, const std::string& body) {
    if (!network::IsSuccessfulStatus(response_code)) {
      std::move(callback).Run(std::nullopt);
      return;
    }
    std::string_view body_view(body);
    if (is_padded) {
      if (!brave::PrivateCdnHelper::GetInstance()->RemovePadding(&body_view)) {
        std::move(callback).Run(std::nullopt);
        return;
      }
    }
    std::move(callback).Run(
        std::vector<uint8_t>(body_view.begin(), body_view.end()));
  };

  pcdn_helper_->DownloadToString(
      resource_url,
      base::BindOnce(on_resource_downloaded, std::move(callback),
                     base::EndsWith(resource_url.path(), ".pad")));
}

void NewTabPageHandler::GetBackgroundsEnabled(
    GetBackgroundsEnabledCallback callback) {
  bool backgrounds_enabled = pref_service_->GetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage);
  std::move(callback).Run(backgrounds_enabled);
}

void NewTabPageHandler::SetBackgroundsEnabled(
    bool enabled,
    SetBackgroundsEnabledCallback callback) {
  pref_service_->SetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, enabled);
  std::move(callback).Run();
}

void NewTabPageHandler::GetBackgroundsCustomizable(
    GetBackgroundsCustomizableCallback callback) {
  std::move(callback).Run(
      !pref_service_->IsManagedPreference(GetThemePrefNameInMigration(
          ThemePrefInMigration::kNtpCustomBackgroundDict)));
}

void NewTabPageHandler::GetSponsoredImagesEnabled(
    GetSponsoredImagesEnabledCallback callback) {
  bool sponsored_images_enabled = pref_service_->GetBoolean(
      ntp_background_images::prefs::
          kNewTabPageShowSponsoredImagesBackgroundImage);
  std::move(callback).Run(sponsored_images_enabled);
}

void NewTabPageHandler::SetSponsoredImagesEnabled(
    bool enabled,
    SetSponsoredImagesEnabledCallback callback) {
  pref_service_->SetBoolean(ntp_background_images::prefs::
                                kNewTabPageShowSponsoredImagesBackgroundImage,
                            enabled);
  std::move(callback).Run();
}

void NewTabPageHandler::GetBraveBackgrounds(
    GetBraveBackgroundsCallback callback) {
  std::move(callback).Run(background_adapter_->GetBraveBackgrounds());
}

void NewTabPageHandler::GetCustomBackgrounds(
    GetCustomBackgroundsCallback callback) {
  std::move(callback).Run(background_adapter_->GetCustomBackgrounds());
}

void NewTabPageHandler::GetSelectedBackground(
    GetSelectedBackgroundCallback callback) {
  std::move(callback).Run(background_adapter_->GetSelectedBackground());
}

void NewTabPageHandler::GetSponsoredImageBackground(
    GetSponsoredImageBackgroundCallback callback) {
  std::move(callback).Run(background_adapter_->GetSponsoredImageBackground());
}

void NewTabPageHandler::SelectBackground(
    mojom::SelectedBackgroundPtr background,
    SelectBackgroundCallback callback) {
  background_adapter_->SelectBackground(std::move(background));
  std::move(callback).Run();
}

void NewTabPageHandler::ShowCustomBackgroundChooser(
    ShowCustomBackgroundChooserCallback callback) {
  custom_image_chooser_->ShowDialog(
      base::BindOnce(&NewTabPageHandler::OnCustomBackgroundsSelected,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NewTabPageHandler::RemoveCustomBackground(
    const std::string& background_url,
    RemoveCustomBackgroundCallback callback) {
  background_adapter_->RemoveCustomBackground(background_url,
                                              std::move(callback));
}

void NewTabPageHandler::GetShowSearchBox(GetShowSearchBoxCallback callback) {
  std::move(callback).Run(pref_service_->GetBoolean(
      brave_search_conversion::prefs::kShowNTPSearchBox));
}

void NewTabPageHandler::SetShowSearchBox(bool show_search_box,
                                         SetShowSearchBoxCallback callback) {
  pref_service_->SetBoolean(brave_search_conversion::prefs::kShowNTPSearchBox,
                            show_search_box);
  std::move(callback).Run();
}

void NewTabPageHandler::GetSearchSuggestionsEnabled(
    GetSearchSuggestionsEnabledCallback callback) {
  std::move(callback).Run(
      pref_service_->GetBoolean(prefs::kSearchSuggestEnabled));
}

void NewTabPageHandler::SetSearchSuggestionsEnabled(
    bool enabled,
    SetSearchSuggestionsEnabledCallback callback) {
  pref_service_->SetBoolean(prefs::kSearchSuggestEnabled, enabled);
  std::move(callback).Run();
}

void NewTabPageHandler::GetSearchSuggestionsPromptDismissed(
    GetSearchSuggestionsPromptDismissedCallback callback) {
  std::move(callback).Run(
      pref_service_->GetBoolean(brave_search_conversion::prefs::kDismissed));
}

void NewTabPageHandler::SetSearchSuggestionsPromptDismissed(
    bool dismissed,
    SetSearchSuggestionsPromptDismissedCallback callback) {
  pref_service_->SetBoolean(brave_search_conversion::prefs::kDismissed,
                            dismissed);
  std::move(callback).Run();
}

void NewTabPageHandler::GetLastUsedSearchEngine(
    GetLastUsedSearchEngineCallback callback) {
  std::move(callback).Run(pref_service_->GetString(
      brave_search_conversion::prefs::kLastUsedNTPSearchEngine));
}

void NewTabPageHandler::SetLastUsedSearchEngine(
    const std::string& engine_host,
    SetLastUsedSearchEngineCallback callback) {
  pref_service_->SetString(
      brave_search_conversion::prefs::kLastUsedNTPSearchEngine, engine_host);
  std::move(callback).Run();
}

void NewTabPageHandler::GetAvailableSearchEngines(
    GetAvailableSearchEnginesCallback callback) {
  std::vector<mojom::SearchEngineInfoPtr> search_engines;
  for (auto template_url : template_url_service_->GetTemplateURLs()) {
    if (template_url->GetBuiltinEngineType() !=
        BuiltinEngineType::KEYWORD_MODE_PREPOPULATED_ENGINE) {
      continue;
    }
    auto search_engine = mojom::SearchEngineInfo::New();
    search_engine->prepopulate_id = template_url->prepopulate_id();
    search_engine->host = GURL(template_url->url()).host();
    if (search_engine->host.empty()) {
      search_engine->host = "google.com";
    }
    search_engine->name = base::UTF16ToUTF8(template_url->short_name());
    search_engine->keyword = base::UTF16ToUTF8(template_url->keyword());
    search_engine->favicon_url = template_url->favicon_url().spec();
    search_engines.push_back(std::move(search_engine));
  }
  std::move(callback).Run(std::move(search_engines));
}

void NewTabPageHandler::OpenSearch(const std::string& query,
                                   const std::string& engine,
                                   mojom::EventDetailsPtr details,
                                   OpenSearchCallback callback) {
  auto* template_url = template_url_service_->GetTemplateURLForHost(engine);
  if (!template_url) {
    std::move(callback).Run();
    return;
  }

  GURL search_url = template_url->GenerateSearchURL(
      template_url_service_->search_terms_data(), base::UTF8ToUTF16(query));

  tab_->GetBrowserWindowInterface()->OpenGURL(
      search_url,
      ui::DispositionFromClick(false, details->alt_key, details->ctrl_key,
                               details->meta_key, details->shift_key));

  std::move(callback).Run();
}

void NewTabPageHandler::OpenURLFromSearch(const std::string& url,
                                          mojom::EventDetailsPtr details,
                                          OpenURLFromSearchCallback callback) {
  tab_->GetBrowserWindowInterface()->OpenGURL(
      GURL(url),
      ui::DispositionFromClick(false, details->alt_key, details->ctrl_key,
                               details->meta_key, details->shift_key));
  std::move(callback).Run();
}

void NewTabPageHandler::OnCustomBackgroundsSelected(
    ShowCustomBackgroundChooserCallback callback,
    std::vector<base::FilePath> paths) {
  // Before continuing, notify the caller of whether backgrounds were selected.
  // This allows the front-end to display a loading indicator while the save
  // operation is in progress.
  std::move(callback).Run(!paths.empty());

  if (!paths.empty()) {
    background_adapter_->SaveCustomBackgrounds(std::move(paths),
                                               base::DoNothing());
  }
}

void NewTabPageHandler::OnUpdate(UpdateObserver::Source update_source) {
  if (!page_.is_bound()) {
    return;
  }
  switch (update_source) {
    case UpdateObserver::Source::kBackgroundPrefs:
      page_->OnBackgroundPrefsUpdated();
      break;
    case UpdateObserver::Source::kSearchPrefs:
      page_->OnSearchPrefsUpdated();
      break;
  }
}

}  // namespace brave_new_tab
