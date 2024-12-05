/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_H_

#include <cstdint>
#include <map>
#include <optional>

#include "base/check.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager_observer.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_observer.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

// This class is responsible for tracking and managing user engagements with
// advertisements and their associated landing pages. Occluded tabs suspend the
// landing, while visible tabs start or resume the landing.

struct PageLandInfo;
struct TabInfo;

class SiteVisit final : public BrowserManagerObserver,
                        public TabManagerObserver {
 public:
  SiteVisit();

  SiteVisit(const SiteVisit&) = delete;
  SiteVisit& operator=(const SiteVisit&) = delete;

  ~SiteVisit() override;

  void AddObserver(SiteVisitObserver* observer);
  void RemoveObserver(SiteVisitObserver* observer);

  void set_last_clicked_ad(const AdInfo& ad) {
    CHECK(ad.IsValid());

    last_clicked_ad_ = ad;
  }

 private:
  // Returns true if the tab is currently landing on a page.
  bool IsLandingOnPage(int32_t tab_id) const;

  void MaybeLandOnPage(const TabInfo& tab, int http_status_code);
  void MaybeLandOnPageAfter(const TabInfo& tab,
                            int http_status_code,
                            const AdInfo& ad,
                            base::TimeDelta page_land_after);
  void MaybeLandOnPageAfterCallback(int32_t tab_id, int http_status_code);
  void LandedOnPage(int32_t tab_id, int http_status_code, const AdInfo& ad);
  void LandedOnPageCallback(int32_t tab_id,
                            int http_status_code,
                            const AdInfo& ad,
                            bool success) const;
  void DidNotLandOnPage(int32_t tab_id, const AdInfo& ad) const;
  void MaybeCancelPageLand(int32_t tab_id);
  void CancelPageLand(int32_t tab_id);
  void StopPageLand(int32_t tab_id);

  void MaybeSuspendOrResumePageLandForVisibleTab();
  void MaybeSuspendOrResumePageLand(int32_t tab_id);
  base::TimeDelta CalculateRemainingTimeToLandOnPage(int32_t tab_id);
  void SuspendPageLand(int32_t tab_id);
  void ResumePageLand(int32_t tab_id);

  void NotifyMaybeLandOnPage(const AdInfo& ad, base::TimeDelta after) const;
  void NotifyDidSuspendPageLand(int32_t tab_id,
                                base::TimeDelta remaining_time) const;
  void NotifyDidResumePageLand(int32_t tab_id,
                               base::TimeDelta remaining_time) const;
  void NotifyDidLandOnPage(int32_t tab_id,
                           int http_status_code,
                           const AdInfo& ad) const;
  void NotifyDidNotLandOnPage(int32_t tab_id, const AdInfo& ad) const;
  void NotifyCanceledPageLand(int32_t tab_id, const AdInfo& ad) const;

  // BrowserManagerObserver:
  void OnBrowserDidBecomeActive() override;
  void OnBrowserDidResignActive() override;
  void OnBrowserDidEnterForeground() override;
  void OnBrowserDidEnterBackground() override;

  // TabManagerObserver:
  void OnTabDidChangeFocus(int32_t tab_id) override;
  void OnTabDidChange(const TabInfo& tab) override;
  void OnTabDidLoad(const TabInfo& tab, int http_status_code) override;
  void OnDidCloseTab(int32_t tab_id) override;

  base::ObserverList<SiteVisitObserver> observers_;

  std::optional<AdInfo> last_clicked_ad_;

  std::map</*tab_id*/ int32_t, PageLandInfo> page_lands_;

  base::WeakPtrFactory<SiteVisit> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_SITE_VISIT_SITE_VISIT_H_
