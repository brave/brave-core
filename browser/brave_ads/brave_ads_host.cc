/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/brave_ads_host.h"

#include <string>
#include <utility>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_ads/search_result_ad/search_result_ad_service_factory.h"
#include "brave/browser/brave_rewards/rewards_panel_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_service.h"
#include "chrome/browser/profiles/profile.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_contents.h"

namespace brave_ads {

BraveAdsHost::BraveAdsHost(Profile* profile, content::WebContents* web_contents)
    : profile_(profile),
      tab_id_(sessions::SessionTabHelper::IdForTab(web_contents)) {
  DCHECK(profile_);
}

BraveAdsHost::~BraveAdsHost() {}

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
  auto* panel_service =
      brave_rewards::RewardsPanelServiceFactory::GetForProfile(profile_);
  if (!panel_service || !ads_service || !ads_service->IsSupportedLocale()) {
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

  panel_observation_.Observe(panel_service);

  if (!panel_service->ShowBraveTalkOptIn()) {
    RunCallbacksAndReset(false);
  }
}

void BraveAdsHost::OnRewardsPanelClosed(Browser* browser) {
  // TODO(zenparsing): Make sure that popup is not closed until after ads has
  // been successfully enabled. We might not be able to ensure that. Also, do
  // we want to listen again for "ads enabled" so that if they are enabled in
  // some other way we can hear that? Ideally, enabling Ads should be
  // synchronous (with perhaps asynchronous side-effects).
  auto* ads_service = AdsServiceFactory::GetForProfile(profile_);
  RunCallbacksAndReset(ads_service && ads_service->IsEnabled());
}

void BraveAdsHost::RunCallbacksAndReset(bool result) {
  DCHECK(!callbacks_.empty());

  panel_observation_.Reset();

  for (auto& callback : callbacks_) {
    std::move(callback).Run(result);
  }
  callbacks_.clear();
}

}  // namespace brave_ads
