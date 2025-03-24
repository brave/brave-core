// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/new_tab_page_handler.h"

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/background_facade.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/custom_image_chooser.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/top_sites_facade.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/vpn_facade.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/misc_metrics/new_tab_metrics.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/search_engines/search_engine_type.h"
#include "components/search_engines/template_url_service.h"
#include "components/tab_collections/public/tab_interface.h"
#include "ui/base/window_open_disposition_utils.h"
#include "url/gurl.h"

namespace brave_new_tab_page_refresh {

NewTabPageHandler::NewTabPageHandler(
    mojo::PendingReceiver<mojom::NewTabPageHandler> receiver,
    std::unique_ptr<CustomImageChooser> custom_image_chooser,
    std::unique_ptr<BackgroundFacade> background_facade,
    std::unique_ptr<TopSitesFacade> top_sites_facade,
    std::unique_ptr<VPNFacade> vpn_facade,
    tabs::TabInterface& tab,
    PrefService& pref_service,
    TemplateURLService& template_url_service,
    misc_metrics::NewTabMetrics& new_tab_metrics)
    : receiver_(this, std::move(receiver)),
      update_observer_(pref_service, top_sites_facade.get()),
      custom_image_chooser_(std::move(custom_image_chooser)),
      background_facade_(std::move(background_facade)),
      top_sites_facade_(std::move(top_sites_facade)),
      vpn_facade_(std::move(vpn_facade)),
      tab_(tab),
      pref_service_(pref_service),
      template_url_service_(template_url_service),
      new_tab_metrics_(new_tab_metrics) {
  CHECK(custom_image_chooser_);
  CHECK(background_facade_);
  CHECK(top_sites_facade_);
  CHECK(vpn_facade_);

  update_observer_.SetCallback(base::BindRepeating(&NewTabPageHandler::OnUpdate,
                                                   weak_factory_.GetWeakPtr()));
}

NewTabPageHandler::~NewTabPageHandler() = default;

void NewTabPageHandler::SetNewTabPage(
    mojo::PendingRemote<mojom::NewTabPage> page) {
  page_.reset();
  page_.Bind(std::move(page));
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
  std::move(callback).Run(background_facade_->GetBraveBackgrounds());
}

void NewTabPageHandler::GetCustomBackgrounds(
    GetCustomBackgroundsCallback callback) {
  std::move(callback).Run(background_facade_->GetCustomBackgrounds());
}

void NewTabPageHandler::GetSelectedBackground(
    GetSelectedBackgroundCallback callback) {
  std::move(callback).Run(background_facade_->GetSelectedBackground());
}

void NewTabPageHandler::GetSponsoredImageBackground(
    GetSponsoredImageBackgroundCallback callback) {
  std::move(callback).Run(background_facade_->GetSponsoredImageBackground());
}

void NewTabPageHandler::SelectBackground(
    mojom::SelectedBackgroundPtr background,
    SelectBackgroundCallback callback) {
  background_facade_->SelectBackground(std::move(background));
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
  background_facade_->RemoveCustomBackground(background_url,
                                             std::move(callback));
}

void NewTabPageHandler::NotifySponsoredImageLogoClicked(
    const std::string& creative_instance_id,
    const std::string& destination_url,
    const std::string& wallpaper_id,
    bool should_metrics_fallback_to_p3a,
    NotifySponsoredImageLogoClickedCallback callback) {
  background_facade_->NotifySponsoredImageLogoClicked(
      creative_instance_id, destination_url, wallpaper_id,
      should_metrics_fallback_to_p3a);
  std::move(callback).Run();
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

void NewTabPageHandler::ReportSearchBoxHidden(
    ReportSearchBoxHiddenCallback callback) {
  new_tab_metrics_->ReportNTPSearchDefaultEngine(std::nullopt);
  std::move(callback).Run();
}

void NewTabPageHandler::ReportSearchEngineUsage(
    int64_t engine_prepopulate_id,
    ReportSearchEngineUsageCallback callback) {
  new_tab_metrics_->ReportNTPSearchDefaultEngine(engine_prepopulate_id);
  std::move(callback).Run();
}

void NewTabPageHandler::ReportSearchResultUsage(
    int64_t engine_prepopulate_id,
    ReportSearchResultUsageCallback callback) {
  new_tab_metrics_->ReportNTPSearchUsage(engine_prepopulate_id);
  std::move(callback).Run();
}

void NewTabPageHandler::GetShowTopSites(GetShowTopSitesCallback callback) {
  std::move(callback).Run(top_sites_facade_->GetTopSitesVisible());
}

void NewTabPageHandler::SetShowTopSites(bool show_top_sites,
                                        SetShowTopSitesCallback callback) {
  top_sites_facade_->SetTopSitesVisible(show_top_sites);
  std::move(callback).Run();
}

void NewTabPageHandler::GetTopSitesListKind(
    GetTopSitesListKindCallback callback) {
  std::move(callback).Run(top_sites_facade_->GetListKind());
}

void NewTabPageHandler::SetTopSitesListKind(
    mojom::TopSitesListKind list_kind,
    SetTopSitesListKindCallback callback) {
  top_sites_facade_->SetListKind(list_kind);
  std::move(callback).Run();
}

void NewTabPageHandler::GetTopSites(GetTopSitesCallback callback) {
  top_sites_facade_->GetSites(std::move(callback));
}

void NewTabPageHandler::AddCustomTopSite(const std::string& url,
                                         const std::string& title,
                                         AddCustomTopSiteCallback callback) {
  top_sites_facade_->AddCustomSite(url, title);
  std::move(callback).Run();
}

void NewTabPageHandler::UpdateCustomTopSite(
    const std::string& url,
    const std::string& new_url,
    const std::string& title,
    UpdateCustomTopSiteCallback callback) {
  top_sites_facade_->UpdateCustomSite(url, new_url, title);
  std::move(callback).Run();
}

void NewTabPageHandler::RemoveCustomTopSite(
    const std::string& url,
    RemoveCustomTopSiteCallback callback) {
  top_sites_facade_->RemoveCustomSite(url);
  std::move(callback).Run();
}

void NewTabPageHandler::UndoCustomTopSiteAction(
    UndoCustomTopSiteActionCallback callback) {
  top_sites_facade_->UndoCustomSiteAction();
  std::move(callback).Run();
}

void NewTabPageHandler::ExcludeMostVisitedTopSite(
    const std::string& url,
    ExcludeMostVisitedTopSiteCallback callback) {
  top_sites_facade_->ExcludeMostVisitedSite(url);
  std::move(callback).Run();
}

void NewTabPageHandler::IncludeMostVisitedTopSite(
    const std::string& url,
    IncludeMostVisitedTopSiteCallback callback) {
  top_sites_facade_->IncludeMostVisitedTopSite(url);
  std::move(callback).Run();
}

void NewTabPageHandler::SetCustomTopSitePosition(
    const std::string& url,
    int32_t position,
    SetCustomTopSitePositionCallback callback) {
  top_sites_facade_->SetCustomSitePosition(url, position);
  std::move(callback).Run();
}

void NewTabPageHandler::GetShowClock(GetShowClockCallback callback) {
  std::move(callback).Run(pref_service_->GetBoolean(kNewTabPageShowClock));
}

void NewTabPageHandler::SetShowClock(bool show_clock,
                                     SetShowClockCallback callback) {
  pref_service_->SetBoolean(kNewTabPageShowClock, show_clock);
  std::move(callback).Run();
}

void NewTabPageHandler::GetClockFormat(GetClockFormatCallback callback) {
  std::move(callback).Run(pref_service_->GetString(kNewTabPageClockFormat));
}

void NewTabPageHandler::SetClockFormat(const std::string& clock_format,
                                       SetClockFormatCallback callback) {
  pref_service_->SetString(kNewTabPageClockFormat, clock_format);
  std::move(callback).Run();
}

void NewTabPageHandler::GetShowShieldsStats(
    GetShowShieldsStatsCallback callback) {
  std::move(callback).Run(pref_service_->GetBoolean(kNewTabPageShowStats));
}

void NewTabPageHandler::SetShowShieldsStats(
    bool show_shields_stats,
    SetShowShieldsStatsCallback callback) {
  pref_service_->SetBoolean(kNewTabPageShowStats, show_shields_stats);
  std::move(callback).Run();
}

void NewTabPageHandler::GetShieldsStats(GetShieldsStatsCallback callback) {
  auto stats = mojom::ShieldsStats::New();
  stats->ads_blocked = pref_service_->GetUint64(kAdsBlocked) +
                       pref_service_->GetUint64(kTrackersBlocked);
  stats->bandwidth_saved_bytes = pref_service_->GetUint64(
      brave_perf_predictor::prefs::kBandwidthSavedBytes);
  std::move(callback).Run(std::move(stats));
}

void NewTabPageHandler::GetShowTalkWidget(GetShowTalkWidgetCallback callback) {
  std::move(callback).Run(pref_service_->GetBoolean(kNewTabPageShowBraveTalk));
}

void NewTabPageHandler::SetShowTalkWidget(bool show_talk_widget,
                                          SetShowTalkWidgetCallback callback) {
  pref_service_->SetBoolean(kNewTabPageShowBraveTalk, show_talk_widget);
  std::move(callback).Run();
}

void NewTabPageHandler::GetShowRewardsWidget(
    GetShowRewardsWidgetCallback callback) {
  std::move(callback).Run(pref_service_->GetBoolean(kNewTabPageShowRewards));
}

void NewTabPageHandler::SetShowRewardsWidget(
    bool show_rewards_widget,
    SetShowRewardsWidgetCallback callback) {
  pref_service_->SetBoolean(kNewTabPageShowRewards, show_rewards_widget);
  std::move(callback).Run();
}

void NewTabPageHandler::GetShowVPNWidget(GetShowVPNWidgetCallback callback) {
  if (auto pref_name = vpn_facade_->GetWidgetPrefName()) {
    std::move(callback).Run(pref_service_->GetBoolean(*pref_name));
  } else {
    std::move(callback).Run(false);
  }
}

void NewTabPageHandler::SetShowVPNWidget(bool show_vpn_widget,
                                         SetShowVPNWidgetCallback callback) {
  if (auto pref_name = vpn_facade_->GetWidgetPrefName()) {
    pref_service_->SetBoolean(*pref_name, show_vpn_widget);
  }
  std::move(callback).Run();
}

void NewTabPageHandler::ReloadVPNPurchasedState(
    ReloadVPNPurchasedStateCallback callback) {
  vpn_facade_->ReloadPurchasedState();
  std::move(callback).Run();
}

void NewTabPageHandler::OpenVPNPanel(OpenVPNPanelCallback callback) {
  vpn_facade_->OpenPanel();
  std::move(callback).Run();
}

void NewTabPageHandler::OpenVPNAccountPage(
    brave_vpn::mojom::ManageURLType url_type,
    OpenVPNAccountPageCallback callback) {
  vpn_facade_->OpenAccountPage(url_type);
  std::move(callback).Run();
}

void NewTabPageHandler::ReportVPNWidgetUsage(
    ReportVPNWidgetUsageCallback callback) {
  vpn_facade_->RecordWidgetUsage();
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
    background_facade_->SaveCustomBackgrounds(std::move(paths),
                                              base::DoNothing());
  }
}

void NewTabPageHandler::OnUpdate(UpdateObserver::Source update_source) {
  if (!page_.is_bound()) {
    return;
  }
  switch (update_source) {
    case UpdateObserver::Source::kBackgrounds:
      page_->OnBackgroundsUpdated();
      break;
    case UpdateObserver::Source::kSearch:
      page_->OnSearchStateUpdated();
      break;
    case UpdateObserver::Source::kTopSites:
      page_->OnTopSitesUpdated();
      break;
    case UpdateObserver::Source::kClock:
      page_->OnClockStateUpdated();
      break;
    case UpdateObserver::Source::kShieldsStats:
      page_->OnShieldsStatsUpdated();
      break;
    case UpdateObserver::Source::kTalk:
      page_->OnTalkStateUpdated();
      break;
    case UpdateObserver::Source::kRewards:
      page_->OnRewardsStateUpdated();
      break;
    case UpdateObserver::Source::kVPN:
      page_->OnVPNStateUpdated();
      break;
  }
}

}  // namespace brave_new_tab_page_refresh
