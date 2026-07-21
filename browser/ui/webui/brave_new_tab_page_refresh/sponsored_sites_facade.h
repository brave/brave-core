// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_SPONSORED_SITES_FACADE_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_SPONSORED_SITES_FACADE_H_

#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/brave_new_tab_page.mojom.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"

class PrefService;

namespace history {
class HistoryService;
}  // namespace history

namespace brave_new_tab_page_refresh {

// Provides a simplified interface for accessing sponsored sites eligible for
// display on the new tab page.
class SponsoredSitesFacade
    : public ntp_background_images::NTPBackgroundImagesService::Observer {
 public:
  SponsoredSitesFacade(PrefService& pref_service,
                       ntp_background_images::NTPBackgroundImagesService*
                           background_images_service,
                       history::HistoryService* history_service);

  SponsoredSitesFacade(const SponsoredSitesFacade&) = delete;
  SponsoredSitesFacade& operator=(const SponsoredSitesFacade&) = delete;

  ~SponsoredSitesFacade() override;

  using GetSitesCallback =
      base::OnceCallback<void(std::vector<mojom::SponsoredSitePtr>)>;

  void GetSites(GetSitesCallback callback);

  void SetSitesUpdatedCallback(base::RepeatingClosure callback);

 private:
  // NTPBackgroundImagesService::Observer:
  void OnSponsoredSitesDataDidUpdate() override;

  bool IsEligible() const;
  bool IsEnabled() const;
  bool IsNewTabPageAdsEnabled() const;
  bool IsRewardsWalletConnected() const;

  raw_ref<PrefService> pref_service_;
  raw_ptr<ntp_background_images::NTPBackgroundImagesService>
      background_images_service_;
  raw_ptr<history::HistoryService> history_service_;
  base::CancelableTaskTracker history_task_tracker_;
  base::RepeatingClosure updated_callback_;
  base::ScopedObservation<
      ntp_background_images::NTPBackgroundImagesService,
      ntp_background_images::NTPBackgroundImagesService::Observer>
      ntp_background_images_service_observation_{this};
};

}  // namespace brave_new_tab_page_refresh

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_NEW_TAB_PAGE_REFRESH_SPONSORED_SITES_FACADE_H_
