// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_NEW_TAB_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_NEW_TAB_PAGE_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/update_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class GURL;
class PrefService;
class TemplateURLService;
enum class WindowOpenDisposition;

namespace misc_metrics {
class NewTabMetrics;
}

namespace content {
class WebContents;
}

namespace brave_new_tab_page_refresh {

class BackgroundFacade;
class CustomImageChooser;
class TopSitesFacade;
class VPNFacade;

// Handler for messages from the NTP front end application. Interface method
// implementations should be fairly trivial. Any non-trivial operations should
// be delegated to a helper class.
class NewTabPageHandler : public mojom::NewTabPageHandler {
 public:
  NewTabPageHandler(mojo::PendingReceiver<mojom::NewTabPageHandler> receiver,
                    std::unique_ptr<CustomImageChooser> custom_image_chooser,
                    std::unique_ptr<BackgroundFacade> background_facade,
                    std::unique_ptr<TopSitesFacade> top_sites_facade,
                    std::unique_ptr<VPNFacade> vpn_facade,
                    content::WebContents& web_contents,
                    PrefService& pref_service,
                    TemplateURLService& template_url_service,
                    misc_metrics::NewTabMetrics& new_tab_metrics);

  ~NewTabPageHandler() override;

  // mojom::NewTabPageHandler:
  void SetNewTabPage(mojo::PendingRemote<mojom::NewTabPage> page) override;
  void GetBackgroundsEnabled(GetBackgroundsEnabledCallback callback) override;
  void SetBackgroundsEnabled(bool enabled,
                             SetBackgroundsEnabledCallback callback) override;
  void GetSponsoredImagesEnabled(
      GetSponsoredImagesEnabledCallback callback) override;
  void SetSponsoredImagesEnabled(
      bool enabled,
      SetSponsoredImagesEnabledCallback callback) override;
  void GetBraveBackgrounds(GetBraveBackgroundsCallback callback) override;
  void GetCustomBackgrounds(GetCustomBackgroundsCallback callback) override;
  void GetSelectedBackground(GetSelectedBackgroundCallback callback) override;
  void GetSponsoredImageBackground(
      GetSponsoredImageBackgroundCallback callback) override;
  void SelectBackground(mojom::SelectedBackgroundPtr background,
                        SelectBackgroundCallback callback) override;
  void ShowCustomBackgroundChooser(
      ShowCustomBackgroundChooserCallback callback) override;
  void RemoveCustomBackground(const std::string& background_url,
                              RemoveCustomBackgroundCallback callback) override;
  void NotifySponsoredImageLogoClicked(
      const std::string& wallpaper_id,
      const std::string& creative_instance_id,
      const std::string& destination_url,
      brave_ads::mojom::NewTabPageAdMetricType mojom_ad_metric_type,
      NotifySponsoredImageLogoClickedCallback callback) override;

  void GetShowSearchBox(GetShowSearchBoxCallback callback) override;
  void SetShowSearchBox(bool show_search_box,
                        SetShowSearchBoxCallback callback) override;
  void GetSearchSuggestionsEnabled(
      GetSearchSuggestionsEnabledCallback callback) override;
  void SetSearchSuggestionsEnabled(
      bool enabled,
      SetSearchSuggestionsEnabledCallback callback) override;
  void GetSearchSuggestionsPromptDismissed(
      GetSearchSuggestionsPromptDismissedCallback callback) override;
  void SetSearchSuggestionsPromptDismissed(
      bool dismissed,
      SetSearchSuggestionsPromptDismissedCallback callback) override;
  void GetLastUsedSearchEngine(
      GetLastUsedSearchEngineCallback callback) override;
  void SetLastUsedSearchEngine(
      const std::string& engine_host,
      SetLastUsedSearchEngineCallback callback) override;
  void GetAvailableSearchEngines(
      GetAvailableSearchEnginesCallback callback) override;
  void OpenSearch(const std::string& query,
                  const std::string& engine,
                  mojom::EventDetailsPtr details,
                  OpenSearchCallback callback) override;
  void OpenURLFromSearch(const std::string& url,
                         mojom::EventDetailsPtr details,
                         OpenURLFromSearchCallback callback) override;
  void ReportSearchBoxHidden(ReportSearchBoxHiddenCallback callback) override;
  void ReportSearchEngineUsage(
      int64_t engine_prepopulate_id,
      ReportSearchEngineUsageCallback callback) override;
  void ReportSearchResultUsage(
      int64_t engine_prepopulate_id,
      ReportSearchResultUsageCallback callback) override;
  void GetShowTopSites(GetShowTopSitesCallback callback) override;
  void SetShowTopSites(bool show_top_sites,
                       SetShowTopSitesCallback callback) override;
  void GetTopSitesListKind(GetTopSitesListKindCallback callback) override;
  void SetTopSitesListKind(mojom::TopSitesListKind list_kind,
                           SetTopSitesListKindCallback callback) override;
  void GetTopSites(GetTopSitesCallback callback) override;
  void AddCustomTopSite(const std::string& url,
                        const std::string& title,
                        AddCustomTopSiteCallback callback) override;
  void UpdateCustomTopSite(const std::string& url,
                           const std::string& new_url,
                           const std::string& title,
                           UpdateCustomTopSiteCallback callback) override;
  void SetCustomTopSitePosition(
      const std::string& url,
      int32_t position,
      SetCustomTopSitePositionCallback callback) override;
  void RemoveCustomTopSite(const std::string& url,
                           RemoveCustomTopSiteCallback callback) override;
  void UndoCustomTopSiteAction(
      UndoCustomTopSiteActionCallback callback) override;
  void ExcludeMostVisitedTopSite(
      const std::string& url,
      ExcludeMostVisitedTopSiteCallback callback) override;
  void IncludeMostVisitedTopSite(
      const std::string& url,
      IncludeMostVisitedTopSiteCallback callback) override;
  void GetShowClock(GetShowClockCallback callback) override;
  void SetShowClock(bool show_clock, SetShowClockCallback callback) override;
  void GetClockFormat(GetClockFormatCallback callback) override;
  void SetClockFormat(mojom::ClockFormat clock_format,
                      SetClockFormatCallback callback) override;
  void GetShowShieldsStats(GetShowShieldsStatsCallback callback) override;
  void SetShowShieldsStats(bool show_shields_stats,
                           SetShowShieldsStatsCallback callback) override;
  void GetShieldsStats(GetShieldsStatsCallback callback) override;
  void GetShowTalkWidget(GetShowTalkWidgetCallback callback) override;
  void SetShowTalkWidget(bool show_talk_widget,
                         SetShowTalkWidgetCallback callback) override;
  void GetShowRewardsWidget(GetShowRewardsWidgetCallback callback) override;
  void SetShowRewardsWidget(bool show_rewards_widget,
                            SetShowRewardsWidgetCallback callback) override;
  void GetShowVPNWidget(GetShowVPNWidgetCallback callback) override;
  void SetShowVPNWidget(bool show_vpn_widget,
                        SetShowVPNWidgetCallback callback) override;
  void ReloadVPNPurchasedState(
      ReloadVPNPurchasedStateCallback callback) override;
  void OpenVPNPanel(OpenVPNPanelCallback callback) override;
  void OpenVPNAccountPage(brave_vpn::mojom::ManageURLType url_type,
                          OpenVPNAccountPageCallback callback) override;
  void ReportVPNWidgetUsage(ReportVPNWidgetUsageCallback callback) override;

 private:
  void OnCustomBackgroundsSelected(ShowCustomBackgroundChooserCallback callback,
                                   std::vector<base::FilePath> paths);

  void OnUpdate(UpdateObserver::Source update_source);
  void OpenGURL(const GURL& gurl, WindowOpenDisposition disposition);

  mojo::Receiver<mojom::NewTabPageHandler> receiver_;
  mojo::Remote<mojom::NewTabPage> page_;
  UpdateObserver update_observer_;
  std::unique_ptr<CustomImageChooser> custom_image_chooser_;
  std::unique_ptr<BackgroundFacade> background_facade_;
  std::unique_ptr<TopSitesFacade> top_sites_facade_;
  std::unique_ptr<VPNFacade> vpn_facade_;
  raw_ref<content::WebContents> web_contents_;
  raw_ref<PrefService> pref_service_;
  raw_ref<TemplateURLService> template_url_service_;
  raw_ref<misc_metrics::NewTabMetrics> new_tab_metrics_;
  base::WeakPtrFactory<NewTabPageHandler> weak_factory_{this};
};

}  // namespace brave_new_tab_page_refresh

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_NEW_TAB_PAGE_HANDLER_H_
