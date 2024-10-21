/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit.h"

#include "base/containers/contains.h"
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

void SiteVisit::AddObserver(SiteVisitObserver* const observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void SiteVisit::RemoveObserver(SiteVisitObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

///////////////////////////////////////////////////////////////////////////////

bool SiteVisit::IsLandingOnPage(const int32_t tab_id) const {
  return base::Contains(page_lands_, tab_id);
}

void SiteVisit::MaybeLandOnPage(const TabInfo& tab,
                                const int http_status_code) {
  if (!last_clicked_ad_) {
    // No ad interactions have occurred in the current browsing session.
    return;
  }

  if (!IsAllowedToLandOnPage(last_clicked_ad_->type)) {
    return;
  }

  if (!IsLandingOnPage(tab.id)) {
    MaybeLandOnPageAfter(tab, http_status_code, *last_clicked_ad_,
                         kPageLandAfter.Get());
  }

  // Reset the last clicked ad to prevent multiple landings on the same ad.
  last_clicked_ad_.reset();
}

void SiteVisit::MaybeLandOnPageAfter(const TabInfo& tab,
                                     const int http_status_code,
                                     const AdInfo& ad,
                                     const base::TimeDelta page_land_after) {
  CHECK(!IsLandingOnPage(tab.id));

  if (!DomainOrHostExists(tab.redirect_chain, ad.target_url)) {
    return BLOG(1, "Visited page does not match the ad landing page");
  }

  NotifyMaybeLandOnPage(ad, page_land_after);

  page_lands_[tab.id].ad = ad;

  // Start the timer to check if the user has navigated to the landing page post
  // ad click.
  page_lands_[tab.id].timer.Start(
      FROM_HERE, page_land_after,
      base::BindRepeating(&SiteVisit::MaybeLandOnPageAfterCallback,
                          weak_factory_.GetWeakPtr(), tab.id,
                          http_status_code));

  if (!tab.is_visible) {
    // Suspend the page landing if the tab is not visible, i.e., the user opened
    // the tab in the background.
    SuspendPageLand(tab.id);
  }
}

void SiteVisit::MaybeLandOnPageAfterCallback(const int32_t tab_id,
                                             const int http_status_code) {
  CHECK(IsLandingOnPage(tab_id));

  const AdInfo ad = page_lands_[tab_id].ad;

  if (DidLandOnPage(tab_id, ad.target_url)) {
    LandedOnPage(tab_id, http_status_code, ad);
  } else {
    DidNotLandOnPage(tab_id, ad);
  }

  StopPageLand(tab_id);
}

void SiteVisit::LandedOnPage(const int32_t tab_id,
                             const int http_status_code,
                             const AdInfo& ad) {
  RecordAdEvent(
      ad, mojom::ConfirmationType::kLanded,
      base::BindOnce(&SiteVisit::LandedOnPageCallback,
                     weak_factory_.GetWeakPtr(), tab_id, http_status_code, ad));
}

void SiteVisit::LandedOnPageCallback(const int32_t tab_id,
                                     const int http_status_code,
                                     const AdInfo& ad,
                                     const bool success) const {
  if (!success) {
    BLOG(0, "Failed to record ad page land event");

    return NotifyDidNotLandOnPage(tab_id, ad);
  }

  NotifyDidLandOnPage(tab_id, http_status_code, ad);
}

void SiteVisit::DidNotLandOnPage(const int32_t tab_id, const AdInfo& ad) const {
  NotifyDidNotLandOnPage(tab_id, ad);
}

void SiteVisit::MaybeCancelPageLand(const int32_t tab_id) {
  if (!IsLandingOnPage(tab_id)) {
    return;
  }

  const AdInfo ad = page_lands_[tab_id].ad;

  if (!DidLandOnPage(tab_id, ad.target_url)) {
    // The user navigated away from the landing page post ad click.
    CancelPageLand(tab_id);
  }
}

void SiteVisit::CancelPageLand(const int32_t tab_id) {
  if (!IsLandingOnPage(tab_id)) {
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

void SiteVisit::MaybeSuspendOrResumePageLandForVisibleTab() {
  if (const std::optional<TabInfo> tab =
          TabManager::GetInstance().MaybeGetVisible()) {
    MaybeSuspendOrResumePageLand(tab->id);
  }
}

void SiteVisit::MaybeSuspendOrResumePageLand(const int32_t tab_id) {
  if (!kShouldSuspendAndResumePageLand.Get()) {
    return;
  }

  if (!IsLandingOnPage(tab_id)) {
    return;
  }

  const bool should_resume = TabManager::GetInstance().IsVisible(tab_id) &&
                             BrowserManager::GetInstance().IsActive() &&
                             BrowserManager::GetInstance().IsInForeground();
  if (should_resume) {
    return ResumePageLand(tab_id);
  }

  SuspendPageLand(tab_id);
}

base::TimeDelta SiteVisit::CalculateRemainingTimeToLandOnPage(
    const int32_t tab_id) {
  CHECK(IsLandingOnPage(tab_id));

  const PageLandInfo& page_land = page_lands_[tab_id];
  return page_land.timer.desired_run_time() - base::TimeTicks::Now();
}

void SiteVisit::SuspendPageLand(const int32_t tab_id) {
  CHECK(IsLandingOnPage(tab_id));

  PageLandInfo& page_land = page_lands_[tab_id];

  if (!page_land.timer.IsRunning()) {
    // We have already checked if the user has navigated to the landing page.
    return;
  }

  // `CalculateRemainingTimeToLandOnPage` must be called prior to stopping the
  // timer, else the remaining time will be zero.
  page_land.remaining_time = CalculateRemainingTimeToLandOnPage(tab_id);

  page_land.timer.Stop();

  NotifyDidSuspendPageLand(tab_id, *page_land.remaining_time);
}

void SiteVisit::ResumePageLand(const int32_t tab_id) {
  CHECK(IsLandingOnPage(tab_id));

  PageLandInfo& page_land = page_lands_[tab_id];

  if (page_land.timer.IsRunning() || !page_land.remaining_time) {
    // This is triggered when a new tab is opened, since the page land has not
    // been previously suspended.
    return;
  }

  // Resume the timer to check if the user has navigated to the landing page.
  page_land.timer.Start(FROM_HERE, *page_land.remaining_time,
                        page_land.timer.user_task());

  NotifyDidResumePageLand(tab_id, *page_land.remaining_time);

  page_land.remaining_time.reset();
}

void SiteVisit::NotifyMaybeLandOnPage(const AdInfo& ad,
                                      const base::TimeDelta after) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnMaybeLandOnPage(ad, after);
  }
}

void SiteVisit::NotifyDidSuspendPageLand(
    const int32_t tab_id,
    const base::TimeDelta remaining_time) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnDidSuspendPageLand(tab_id, remaining_time);
  }
}

void SiteVisit::NotifyDidResumePageLand(
    const int32_t tab_id,
    const base::TimeDelta remaining_time) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnDidResumePageLand(tab_id, remaining_time);
  }
}

void SiteVisit::NotifyDidLandOnPage(const int32_t tab_id,
                                    const int http_status_code,
                                    const AdInfo& ad) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnDidLandOnPage(tab_id, http_status_code, ad);
  }
}

void SiteVisit::NotifyDidNotLandOnPage(const int32_t tab_id,
                                       const AdInfo& ad) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnDidNotLandOnPage(tab_id, ad);
  }
}

void SiteVisit::NotifyCanceledPageLand(const int32_t tab_id,
                                       const AdInfo& ad) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnCanceledPageLand(tab_id, ad);
  }
}

void SiteVisit::OnBrowserDidBecomeActive() {
  // Required to suspend or resume the page land because `OnTabDidChangeFocus`
  // is not called when the browser becomes active on mobile.
  MaybeSuspendOrResumePageLandForVisibleTab();
}

void SiteVisit::OnBrowserDidResignActive() {
  // Required to suspend or resume the page land because `OnTabDidChangeFocus`
  // is not called when the browser resigns active on mobile.
  MaybeSuspendOrResumePageLandForVisibleTab();
}

void SiteVisit::OnBrowserDidEnterForeground() {
  // Required to suspend or resume the page land because `OnTabDidChangeFocus`
  // is not called when the browser enters the foreground on mobile.
  MaybeSuspendOrResumePageLandForVisibleTab();
}

void SiteVisit::OnBrowserDidEnterBackground() {
  // Required to suspend or resume the page land because `OnTabDidChangeFocus`
  // is not called when the browser enters the background on mobile.
  MaybeSuspendOrResumePageLandForVisibleTab();
}

void SiteVisit::OnTabDidChangeFocus(const int32_t tab_id) {
  MaybeSuspendOrResumePageLand(tab_id);
}

void SiteVisit::OnTabDidChange(const TabInfo& tab) {
  MaybeCancelPageLand(tab.id);
}

void SiteVisit::OnTabDidLoad(const TabInfo& tab, const int http_status_code) {
  MaybeLandOnPage(tab, http_status_code);
}

void SiteVisit::OnDidCloseTab(const int32_t tab_id) {
  CancelPageLand(tab_id);
}

}  // namespace brave_ads
