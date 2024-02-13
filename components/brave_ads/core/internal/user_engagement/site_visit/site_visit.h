/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_H_

#include <cstdint>
#include <optional>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_observer.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

namespace base {
class Time;
}  // namespace base

class GURL;

namespace brave_ads {

struct TabInfo;

class SiteVisit final : public TabManagerObserver {
 public:
  SiteVisit();

  SiteVisit(const SiteVisit&) = delete;
  SiteVisit& operator=(const SiteVisit&) = delete;

  SiteVisit(SiteVisit&&) noexcept = delete;
  SiteVisit& operator=(SiteVisit&&) noexcept = delete;

  ~SiteVisit() override;

  void AddObserver(SiteVisitObserver* observer);
  void RemoveObserver(SiteVisitObserver* observer);

  void SetLastClickedAd(const AdInfo& ad) { last_clicked_ad_ = ad; }

  void MaybeLandOnPage(int32_t tab_id, const std::vector<GURL>& redirect_chain);

 private:
  void CheckIfLandedOnPage(int32_t tab_id,
                           const std::vector<GURL>& redirect_chain);
  void CheckIfLandedOnPageCallback(int32_t tab_id,
                                   const std::vector<GURL>& redirect_chain);

  void LandOnPage(const AdInfo& ad);
  void LandOnPageCallback(const AdInfo& ad, bool success);

  void CancelPageLand(int32_t tab_id);

  void NotifyMaybeLandOnPage(const AdInfo& ad, base::Time maybe_at) const;
  void NotifyDidLandOnPage(const AdInfo& ad) const;
  void NotifyDidNotLandOnPage(const AdInfo& ad) const;
  void NotifyCanceledPageLand(const AdInfo& ad, int32_t tab_id) const;

  // TabManagerObserver:
  void OnTabDidChange(const TabInfo& tab) override;
  void OnDidCloseTab(int32_t tab_id) override;

  base::ObserverList<SiteVisitObserver> observers_;

  int32_t landed_page_tab_id_ = 0;

  Timer timer_;

  std::optional<AdInfo> last_clicked_ad_;

  base::WeakPtrFactory<SiteVisit> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_H_
