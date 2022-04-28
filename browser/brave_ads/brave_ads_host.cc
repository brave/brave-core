/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/brave_ads_host.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_ads/search_result_ad/search_result_ad_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/extensions/api/brave_action_api.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"

namespace brave_ads {

namespace {

constexpr char kAdsEnableRelativeUrl[] = "request_ads_enabled_panel.html";

}  // namespace

BraveAdsHost::BraveAdsHost(Profile* profile, content::WebContents* web_contents)
    : profile_(profile), web_contents_(web_contents) {
  DCHECK(profile_);
  DCHECK(web_contents_);
}

BraveAdsHost::~BraveAdsHost() {}

void BraveAdsHost::MaybeTriggerAdViewedEvent(
    const std::string& creative_instance_id,
    MaybeTriggerAdViewedEventCallback callback) {
  DCHECK(callback);
  DCHECK(!creative_instance_id.empty());

  SearchResultAdService* search_result_ad_service =
      SearchResultAdServiceFactory::GetForProfile(profile_);

  if (!search_result_ad_service) {
    std::move(callback).Run(/* event_triggered */ false);
    return;
  }

  SessionID tab_id = sessions::SessionTabHelper::IdForTab(web_contents_);
  if (!tab_id.is_valid()) {
    std::move(callback).Run(/* event_triggered */ false);
    return;
  }

  search_result_ad_service->MaybeTriggerSearchResultAdViewedEvent(
      creative_instance_id, tab_id, std::move(callback));
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

  if (!ShowRewardsPopup(rewards_service)) {
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

bool BraveAdsHost::ShowRewardsPopup(
    brave_rewards::RewardsService* rewards_service) {
  DCHECK(rewards_service);

  Browser* browser = chrome::FindBrowserWithProfile(profile_);
  DCHECK(browser);

  auto* extension_service =
      extensions::ExtensionSystem::Get(profile_)->extension_service();
  if (!extension_service) {
    return false;
  }

  extensions::BraveComponentLoader* component_loader =
      static_cast<extensions::BraveComponentLoader*>(
          extension_service->component_loader());
  DCHECK(component_loader);

  if (!component_loader->Exists(brave_rewards_extension_id)) {
    component_loader->AddRewardsExtension();

    rewards_service->StartProcess(base::DoNothing());
  }

  std::string error;
  return extensions::BraveActionAPI::ShowActionUI(
      browser, brave_rewards_extension_id,
      std::make_unique<std::string>(kAdsEnableRelativeUrl), &error);
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
