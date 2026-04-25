// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_TOP_SITES_FACADE_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_TOP_SITES_FACADE_H_

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace brave_new_tab_page_refresh {

// Provides a simplified interface for accessing the top sites API from the new
// tab page.
class TopSitesFacade : public ntp_tiles::MostVisitedSites::Observer {
 public:
  TopSitesFacade(
      std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites,
      PrefService& pref_service);

  TopSitesFacade(const TopSitesFacade&) = delete;
  TopSitesFacade& operator=(const TopSitesFacade&) = delete;

  ~TopSitesFacade() override;

  bool GetTopSitesVisible();
  void SetTopSitesVisible(bool visible);

  mojom::TopSitesListKind GetListKind();
  void SetListKind(mojom::TopSitesListKind list_kind);

  using GetSitesCallback =
      base::OnceCallback<void(std::vector<mojom::TopSitePtr>)>;

  void GetSites(GetSitesCallback callback);

  void AddCustomSite(std::string_view url, std::string_view title);

  void UpdateCustomSite(std::string_view url,
                        std::string_view new_url,
                        std::string_view title);

  void SetCustomSitePosition(std::string_view url, int32_t position);

  void RemoveCustomSite(std::string_view url);

  void UndoCustomSiteAction();

  void ExcludeMostVisitedSite(std::string_view url);

  void IncludeMostVisitedTopSite(std::string_view url);

  void SetSitesUpdatedCallback(base::RepeatingCallback<void()> callback);

  // MostVisitedSites::Observer:
  void OnURLsAvailable(
      bool is_user_triggered,
      const std::map<ntp_tiles::SectionType, ntp_tiles::NTPTilesVector>&
          sections) override;
  void OnIconMadeAvailable(const GURL& site_url) override;

 private:
  void SyncMostVisitedSites();
  void OnPrefChanged(const std::string& path);

  std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites_;
  raw_ref<PrefService> pref_service_;
  PrefChangeRegistrar pref_change_registrar_;
  std::vector<mojom::TopSitePtr> current_sites_;
  base::RepeatingCallback<void()> sites_updated_callback_;
  base::WeakPtrFactory<TopSitesFacade> weak_factory_{this};
};

}  // namespace brave_new_tab_page_refresh

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_TOP_SITES_FACADE_H_
