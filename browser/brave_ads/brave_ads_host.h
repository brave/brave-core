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

class Profile;

namespace brave_ads {

// The class handles chrome.braveRequestAdsEnabled() js api call for Desktop
// platforms. The js api asks the user for permission to enable ads.
class BraveAdsHost : public brave_ads::mojom::BraveAdsHost,
                     public brave_rewards::RewardsServiceObserver {
 public:
  explicit BraveAdsHost(Profile* profile);
  BraveAdsHost(const BraveAdsHost&) = delete;
  BraveAdsHost& operator=(const BraveAdsHost&) = delete;
  ~BraveAdsHost() override;

  // brave_ads::mojom::BraveAdsHost
  void RequestAdsEnabled(RequestAdsEnabledCallback callback) override;

  // brave_rewards::RewardsServiceObserver
  void OnRequestAdsEnabledPopupClosed(bool ads_enabled) override;
  void OnAdsEnabled(brave_rewards::RewardsService* rewards_service,
                    bool ads_enabled) override;

 private:
  bool ShowRewardsPopup(brave_rewards::RewardsService* rewards_service);
  void RunCallbacksAndReset(bool result);

  raw_ptr<Profile> profile_ = nullptr;
  std::vector<RequestAdsEnabledCallback> callbacks_;
  base::ScopedObservation<brave_rewards::RewardsService,
                          brave_rewards::RewardsServiceObserver>
      rewards_service_observation_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_BRAVE_ADS_HOST_H_
