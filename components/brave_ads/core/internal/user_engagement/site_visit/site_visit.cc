/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit.h"

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/page_land/page_land_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"

namespace brave_ads {

SiteVisit::SiteVisit() {
  BrowserManager::GetInstance().AddObserver(this);
  TabManager::GetInstance().AddObserver(this);
}

SiteVisit::~SiteVisit() {
  BrowserManager::GetInstance().RemoveObserver(this);
  TabManager::GetInstance().RemoveObserver(this);
}

void SiteVisit::AddObserver(SiteVisitObserver* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void SiteVisit::RemoveObserver(SiteVisitObserver* observer) {
  CHECK(observer);
  observers_.RemoveObserver(observer);
}

///////////////////////////////////////////////////////////////////////////////

bool SiteVisit::IsPageLanding(const int32_t tab_id) const {
  return base::Contains(page_lands_, tab_id);
}

void SiteVisit::MaybeLandOnPage(const TabInfo& tab) {
  if (!last_clicked_ad_) {
    // No ad interactions have occurred in the current browsing session.
    return;
  }

  if (!IsPageLanding(tab.id)) {
    MaybeLandOnPageAfter(tab, *last_clicked_ad_, kPageLandAfter.Get());
  }

  // Reset the last clicked ad to prevent multiple landings on the same ad.
  last_clicked_ad_.reset();
}

void SiteVisit::MaybeLandOnPageAfter(const TabInfo& tab,
                                     const AdInfo& ad,
                                     const base::TimeDelta page_land_after) {
  CHECK(!IsPageLanding(tab.id));

  if (!DomainOrHostExists(tab.redirect_chain, ad.target_url)) {
    return BLOG(1, "Visited page does not match the ad landing page");
  }

  NotifyMaybeLandOnPage(ad, page_land_after);

  // Start the timer to check if the user has navigated to the landing page post
  // ad click.
  page_lands_[tab.id].ad = ad;
  page_lands_[tab.id].timer.Start(
      FROM_HERE, page_land_after,
      base::BindRepeating(&SiteVisit::MaybeLandOnPageAfterCallback,
                          weak_factory_.GetWeakPtr(), tab));
}

void SiteVisit::MaybeLandOnPageAfterCallback(const TabInfo& tab) {
  CHECK(IsPageLanding(tab.id));

  const AdInfo ad = page_lands_[tab.id].ad;
  DidLandOnPage(tab.id, ad.target_url) ? LandedOnPage(tab, ad)
                                       : DidNotLandOnPage(tab, ad);

  StopPageLand(tab.id);
}

void SiteVisit::LandedOnPage(const TabInfo& tab, const AdInfo& ad) {
  RecordAdEvent(ad, ConfirmationType::kLanded,
                base::BindOnce(&SiteVisit::LandedOnPageCallback,
                               weak_factory_.GetWeakPtr(), tab, ad));
}

void SiteVisit::LandedOnPageCallback(const TabInfo& tab,
                                     const AdInfo& ad,
                                     const bool success) const {
  if (!success) {
    // TODO(https://github.com/brave/brave-browser/issues/32066):
    // Detect potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Failed to record ad page land event");
    base::debug::DumpWithoutCrashing();

    BLOG(1, "Failed to record ad page land event");
    return NotifyDidNotLandOnPage(tab, ad);
  }

  NotifyDidLandOnPage(tab, ad);
}

void SiteVisit::DidNotLandOnPage(const TabInfo& tab, const AdInfo& ad) const {
  NotifyDidNotLandOnPage(tab, ad);
}

void SiteVisit::MaybeCancelPageLand(const TabInfo& tab) {
  if (!IsPageLanding(tab.id)) {
    // The tab isn't a landing page.
    return;
  }

  const AdInfo ad = page_lands_[tab.id].ad;
  if (!DidLandOnPage(tab.id, ad.target_url)) {
    // The user navigated away from the landing page post ad click.
    CancelPageLand(tab.id);
  }
}

void SiteVisit::CancelPageLand(const int32_t tab_id) {
  if (!IsPageLanding(tab_id)) {
    // The tab isn't a landing page.
    return;
  }

  // This must be called prior to stopping the page land. Otherwise, `tab_id`
  // will not be found in `page_lands_`.
  const AdInfo ad = page_lands_[tab_id].ad;

  StopPageLand(tab_id);

  NotifyCanceledPageLand(tab_id, ad);
}

void SiteVisit::StopPageLand(const int32_t tab_id) {
  page_lands_.erase(tab_id);
}

void SiteVisit::MaybeSuspendOrResumePageLandForVisibleTabId() {
  if (const std::optional<int32_t> tab_id =
          TabManager::GetInstance().MaybeGetVisibleTabId()) {
    MaybeSuspendOrResumePageLand(*tab_id);
  }
}

void SiteVisit::MaybeSuspendOrResumePageLand(const int32_t tab_id) {
  if (!IsPageLanding(tab_id)) {
    // The tab isn't a landing page.
    return;
  }

  const std::optional<TabInfo>& tab =
      TabManager::GetInstance().MaybeGetForId(tab_id);
  if (!tab) {
    return BLOG(0, "Failed to get tab for id: " << tab_id);
  }

  const bool should_resume = tab->is_visible &&
                             BrowserManager::GetInstance().IsActive() &&
                             BrowserManager::GetInstance().IsInForeground();

  should_resume ? ResumePageLand(*tab) : SuspendPageLand(*tab);
}

base::TimeDelta SiteVisit::CalculateRemainingTimeToLandOnPage(
    const int32_t tab_id) {
  CHECK(IsPageLanding(tab_id));

  const PageLandInfo& page_land = page_lands_[tab_id];

  const base::TimeDelta remaining_time =
      page_land.timer.desired_run_time() - base::TimeTicks::Now();
  CHECK(!remaining_time.is_negative());

  return remaining_time;
}

void SiteVisit::SuspendPageLand(const TabInfo& tab) {
  CHECK(IsPageLanding(tab.id));

  PageLandInfo& page_land = page_lands_[tab.id];

  if (!page_land.timer.IsRunning()) {
    // We have already checked if the user has navigated to the landing page.
    return;
  }

  // `CalculateRemainingTimeToLandOnPage` must be called prior to stopping the
  // timer, else the remaining time will be zero.
  page_land.remaining_time = CalculateRemainingTimeToLandOnPage(tab.id);

  page_land.timer.Stop();

  NotifyDidSuspendPageLand(tab, *page_land.remaining_time);
}

void SiteVisit::ResumePageLand(const TabInfo& tab) {
  CHECK(IsPageLanding(tab.id));

  PageLandInfo& page_land = page_lands_[tab.id];

  if (page_land.timer.IsRunning() || !page_land.remaining_time) {
    // This is triggered when a new tab is opened, since the page land has not
    // been previously suspended.
    return;
  }

  // Resume the timer to check if the user has navigated to the landing page.
  page_land.timer.Start(FROM_HERE, *page_land.remaining_time,
                        page_land.timer.user_task());

  NotifyDidResumePageLand(tab, *page_land.remaining_time);

  page_land.remaining_time.reset();
}

void SiteVisit::NotifyMaybeLandOnPage(const AdInfo& ad,
                                      const base::TimeDelta after) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnMaybeLandOnPage(ad, after);
  }
}

void SiteVisit::NotifyDidSuspendPageLand(
    const TabInfo& tab,
    const base::TimeDelta remaining_time) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnDidSuspendPageLand(tab, remaining_time);
  }
}

void SiteVisit::NotifyDidResumePageLand(
    const TabInfo& tab,
    const base::TimeDelta remaining_time) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnDidResumePageLand(tab, remaining_time);
  }
}

void SiteVisit::NotifyDidLandOnPage(const TabInfo& tab,
                                    const AdInfo& ad) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnDidLandOnPage(tab, ad);
  }
}

void SiteVisit::NotifyDidNotLandOnPage(const TabInfo& tab,
                                       const AdInfo& ad) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnDidNotLandOnPage(tab, ad);
  }
}

void SiteVisit::NotifyCanceledPageLand(const int32_t tab_id,
                                       const AdInfo& ad) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnCanceledPageLand(tab_id, ad);
  }
}

void SiteVisit::OnBrowserDidBecomeActive() {
  MaybeSuspendOrResumePageLandForVisibleTabId();
}

void SiteVisit::OnBrowserDidResignActive() {
  MaybeSuspendOrResumePageLandForVisibleTabId();
}

void SiteVisit::OnBrowserDidEnterForeground() {
  MaybeSuspendOrResumePageLandForVisibleTabId();
}

void SiteVisit::OnBrowserDidEnterBackground() {
  MaybeSuspendOrResumePageLandForVisibleTabId();
}

void SiteVisit::OnTabDidChangeFocus(const int32_t tab_id) {
  MaybeSuspendOrResumePageLand(tab_id);
}

void SiteVisit::OnTabDidChange(const TabInfo& tab) {
  MaybeCancelPageLand(tab);
  MaybeLandOnPage(tab);
}

void SiteVisit::OnDidOpenNewTab(const TabInfo& tab) {
  MaybeLandOnPage(tab);
}

void SiteVisit::OnDidCloseTab(const int32_t tab_id) {
  CancelPageLand(tab_id);
}

}  // namespace brave_ads
