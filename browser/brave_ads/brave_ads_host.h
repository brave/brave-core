/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_BRAVE_ADS_HOST_H_
#define BRAVE_BROWSER_BRAVE_ADS_BRAVE_ADS_HOST_H_

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_ads/common/brave_ads_host.mojom.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "components/sessions/core/session_id.h"

class Browser;
class Profile;

namespace content {
class WebContents;
}

namespace brave_ads {

// The class handles ads requests from renderer side for Desktop platforms.
class BraveAdsHost : public brave_ads::mojom::BraveAdsHost,
                     public brave_rewards::RewardsServiceObserver {
 public:
  BraveAdsHost(Profile* profile, content::WebContents* web_contents);
  BraveAdsHost(const BraveAdsHost&) = delete;
  BraveAdsHost& operator=(const BraveAdsHost&) = delete;
  ~BraveAdsHost() override;

  // brave_ads::mojom::BraveAdsHost
  void MaybeTriggerAdViewedEvent(
      const std::string& creative_instance_id,
      MaybeTriggerAdViewedEventCallback callback) override;
  void RequestAdsEnabled(RequestAdsEnabledCallback callback) override;

  // brave_rewards::RewardsServiceObserver
  void OnRequestAdsEnabledPopupClosed(bool ads_enabled) override;
  void OnAdsEnabled(brave_rewards::RewardsService* rewards_service,
                    bool ads_enabled) override;

 private:
  bool ShowRewardsPopup();
  void RunCallbacksAndReset(bool result);

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<Browser> browser_ = nullptr;
  SessionID tab_id_;
  std::vector<RequestAdsEnabledCallback> callbacks_;
  base::ScopedObservation<brave_rewards::RewardsService,
                          brave_rewards::RewardsServiceObserver>
      rewards_service_observation_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_BRAVE_ADS_HOST_H_
