/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_EXTENSION_REWARDS_SERVICE_OBSERVER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_EXTENSION_REWARDS_SERVICE_OBSERVER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

class Profile;

namespace brave_rewards {

class RewardsService;

class ExtensionRewardsServiceObserver : public RewardsServiceObserver {
 public:
  explicit ExtensionRewardsServiceObserver(Profile* profile);

  ExtensionRewardsServiceObserver(const ExtensionRewardsServiceObserver&) =
      delete;

  ExtensionRewardsServiceObserver& operator=(
      const ExtensionRewardsServiceObserver&) = delete;

  ~ExtensionRewardsServiceObserver() override;

  // RewardsServiceObserver:
  void OnRewardsInitialized(RewardsService* rewards_service) override;

  void OnRewardsWalletCreated() override;

  void OnTermsOfServiceUpdateAccepted() override;

  void OnPublisherListNormalized(
      RewardsService* rewards_service,
      std::vector<brave_rewards::mojom::PublisherInfoPtr> list) override;

  void OnExcludedSitesChanged(RewardsService* rewards_service,
                              std::string publisher_key,
                              bool excluded) override;

  void OnRecurringTipSaved(RewardsService* rewards_service,
                           bool success) override;

  void OnRecurringTipRemoved(RewardsService* rewards_service,
                             bool success) override;

  void OnReconcileComplete(
      RewardsService* rewards_service,
      const brave_rewards::mojom::Result result,
      const std::string& contribution_id,
      const double amount,
      const brave_rewards::mojom::RewardsType type,
      const brave_rewards::mojom::ContributionProcessor processor) override;

  void OnExternalWalletConnected() override;

  void OnExternalWalletLoggedOut() override;

  void OnExternalWalletDisconnected() override;

  void OnCompleteReset(const bool success) override;

  void OnPanelPublisherInfo(RewardsService* rewards_service,
                            const brave_rewards::mojom::Result result,
                            const brave_rewards::mojom::PublisherInfo* info,
                            uint64_t windowId) override;

 private:
  raw_ptr<Profile> profile_ = nullptr;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_EXTENSION_REWARDS_SERVICE_OBSERVER_H_
