// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_TOP_SITES_ADAPTER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_TOP_SITES_ADAPTER_H_

#include <map>
#include <memory>
#include <string_view>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_new_tab/new_tab_page.mojom.h"
#include "components/ntp_tiles/most_visited_sites.h"

class PrefService;

namespace brave_new_tab {

// Provides access to the top sites API for use by the new tab page.
class TopSitesAdapter : public ntp_tiles::MostVisitedSites::Observer {
 public:
  TopSitesAdapter(
      std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites,
      PrefService& pref_service);

  TopSitesAdapter(const TopSitesAdapter&) = delete;
  TopSitesAdapter& operator=(const TopSitesAdapter&) = delete;

  ~TopSitesAdapter() override;

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
      const std::map<ntp_tiles::SectionType, ntp_tiles::NTPTilesVector>&
          sections) override;
  void OnIconMadeAvailable(const GURL& site_url) override;

 private:
  std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites_;
  raw_ref<PrefService> pref_service_;
  std::vector<mojom::TopSitePtr> current_sites_;
  base::RepeatingCallback<void()> sites_updated_callback_;
};

}  // namespace brave_new_tab

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_TOP_SITES_ADAPTER_H_
