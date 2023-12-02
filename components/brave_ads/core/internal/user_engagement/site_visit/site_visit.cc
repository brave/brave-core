/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/site_visit/site_visit.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_util.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_info.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/user_engagement/site_visit/site_visit_feature.h"
#include "url/gurl.h"

namespace brave_ads {

SiteVisit::SiteVisit() {
  TabManager::GetInstance().AddObserver(this);
}

SiteVisit::~SiteVisit() {
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

void SiteVisit::MaybeLandOnPage(const int32_t tab_id,
                                const std::vector<GURL>& redirect_chain) {
  if (!last_clicked_ad_) {
    return;
  }

  if (landed_page_tab_id_ == tab_id) {
    return;
  }

  if (!DomainOrHostExists(redirect_chain, last_clicked_ad_->target_url)) {
    return BLOG(1, "Visited URL does not match the last clicked ad");
  }

  CheckIfLandedOnPage(tab_id, redirect_chain);
}

///////////////////////////////////////////////////////////////////////////////

void SiteVisit::CheckIfLandedOnPage(const int32_t tab_id,
                                    const std::vector<GURL>& redirect_chain) {
  CHECK(last_clicked_ad_);

  timer_.Stop();

  landed_page_tab_id_ = tab_id;

  const base::Time record_page_land_at = timer_.Start(
      FROM_HERE, kPageLandAfter.Get(),
      base::BindOnce(&SiteVisit::CheckIfLandedOnPageCallback,
                     base::Unretained(this), tab_id, redirect_chain));

  NotifyMaybeLandOnPage(*last_clicked_ad_, record_page_land_at);
}

void SiteVisit::CheckIfLandedOnPageCallback(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain) {
  CHECK(last_clicked_ad_);

  const AdInfo ad = *last_clicked_ad_;
  last_clicked_ad_.reset();

  landed_page_tab_id_ = 0;

  if (!TabManager::GetInstance().IsVisible(tab_id)) {
    return NotifyDidNotLandOnPage(ad);
  }

  const std::optional<TabInfo> tab =
      TabManager::GetInstance().MaybeGetForId(tab_id);
  if (!tab) {
    return NotifyDidNotLandOnPage(ad);
  }

  if (tab->redirect_chain.empty()) {
    return NotifyDidNotLandOnPage(ad);
  }

  if (!DomainOrHostExists(redirect_chain, tab->redirect_chain.back())) {
    return NotifyDidNotLandOnPage(ad);
  }

  LandOnPage(ad);
}

void SiteVisit::LandOnPage(const AdInfo& ad) {
  RecordAdEvent(ad, ConfirmationType::kLanded,
                base::BindOnce(&SiteVisit::LandOnPageCallback,
                               weak_factory_.GetWeakPtr(), ad));
}

void SiteVisit::LandOnPageCallback(const AdInfo& ad, const bool success) {
  if (!success) {
    BLOG(1, "Failed to record landed ad event");
    return NotifyDidNotLandOnPage(ad);
  }

  NotifyDidLandOnPage(ad);
}

void SiteVisit::CancelPageLand(const int32_t tab_id) {
  if (!last_clicked_ad_) {
    return;
  }

  if (landed_page_tab_id_ != tab_id) {
    return;
  }

  if (!timer_.Stop()) {
    return;
  }

  NotifyCanceledPageLand(*last_clicked_ad_, tab_id);
}

void SiteVisit::NotifyMaybeLandOnPage(const AdInfo& ad,
                                      const base::Time maybe_at) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnMaybeLandOnPage(ad, maybe_at);
  }
}

void SiteVisit::NotifyDidLandOnPage(const AdInfo& ad) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnDidLandOnPage(ad);
  }
}

void SiteVisit::NotifyDidNotLandOnPage(const AdInfo& ad) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnDidNotLandOnPage(ad);
  }
}

void SiteVisit::NotifyCanceledPageLand(const AdInfo& ad,
                                       const int32_t tab_id) const {
  for (SiteVisitObserver& observer : observers_) {
    observer.OnCanceledPageLand(ad, tab_id);
  }
}

void SiteVisit::OnTabDidChange(const TabInfo& tab) {
  MaybeLandOnPage(tab.id, tab.redirect_chain);
}

void SiteVisit::OnDidCloseTab(const int32_t tab_id) {
  CancelPageLand(tab_id);
}

}  // namespace brave_ads
