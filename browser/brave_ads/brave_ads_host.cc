/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/brave_ads_host.h"

#include <string>
#include <utility>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_ads/search_result_ad/search_result_ad_service_factory.h"
#include "brave/browser/brave_rewards/rewards_panel/rewards_panel_coordinator.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_contents.h"

namespace brave_ads {

BraveAdsHost::BraveAdsHost(Profile* profile, content::WebContents* web_contents)
    : profile_(profile),
      tab_id_(sessions::SessionTabHelper::IdForTab(web_contents)) {
  DCHECK(profile_);
  if (web_contents) {
    browser_ = chrome::FindBrowserWithWebContents(web_contents);
    DCHECK(browser_);
  }
}

BraveAdsHost::~BraveAdsHost() = default;

void BraveAdsHost::MaybeTriggerAdViewedEvent(
    const std::string& creative_instance_id,
    MaybeTriggerAdViewedEventCallback callback) {
  DCHECK(callback);
  DCHECK(!creative_instance_id.empty());

  if (!tab_id_.is_valid()) {
    std::move(callback).Run(/* event_triggered */ false);
    return;
  }

  SearchResultAdService* search_result_ad_service =
      SearchResultAdServiceFactory::GetForProfile(profile_);

  if (!search_result_ad_service) {
    std::move(callback).Run(/* event_triggered */ false);
    return;
  }

  search_result_ad_service->MaybeTriggerSearchResultAdViewedEvent(
      creative_instance_id, tab_id_, std::move(callback));
}

void BraveAdsHost::RequestAdsEnabled(RequestAdsEnabledCallback callback) {
  DCHECK(callback);

  const AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
  brave_rewards::RewardsService* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  if (!rewards_service || !ads_service || !ads_service->IsSupportedLocale()) {
    std::move(callback).Run(false);
    return;
  }

  if (ads_service->IsEnabled()) {
    std::move(callback).Run(true);
    return;
  }

  if (!callbacks_.empty()) {
    callbacks_.push_back(std::move(callback));
    return;
  }

  callbacks_.push_back(std::move(callback));

  rewards_service_observation_.Observe(rewards_service);

  if (!ShowRewardsPopup()) {
    RunCallbacksAndReset(false);
  }
}

void BraveAdsHost::OnRequestAdsEnabledPopupClosed(bool ads_enabled) {
  // If ads were enabled then do nothing and wait for ads enabled event.
  if (!ads_enabled) {
    RunCallbacksAndReset(false);
  }
}

void BraveAdsHost::OnAdsEnabled(brave_rewards::RewardsService* rewards_service,
                                bool ads_enabled) {
  DCHECK(rewards_service);

  RunCallbacksAndReset(ads_enabled);
}

bool BraveAdsHost::ShowRewardsPopup() {
  if (!browser_) {
    return false;
  }

  auto* coordinator =
      brave_rewards::RewardsPanelCoordinator::FromBrowser(browser_);

  if (coordinator) {
    return coordinator->ShowBraveTalkOptIn();
  }

  return false;
}

void BraveAdsHost::RunCallbacksAndReset(bool result) {
  DCHECK(!callbacks_.empty());

  rewards_service_observation_.Reset();

  for (auto& callback : callbacks_) {
    std::move(callback).Run(result);
  }
  callbacks_.clear();
}

}  // namespace brave_ads
