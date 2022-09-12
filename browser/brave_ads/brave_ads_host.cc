/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/brave_ads_host.h"

#include <string>
#include <utility>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_ads/search_result_ad/search_result_ad_service_factory.h"
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

}  // namespace brave_ads
