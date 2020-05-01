/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_EXTENSION_REWARDS_SERVICE_OBSERVER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_EXTENSION_REWARDS_SERVICE_OBSERVER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_private_observer.h"

class Profile;

namespace brave_rewards {

class RewardsService;

class ExtensionRewardsServiceObserver : public RewardsServiceObserver,
                                        public RewardsServicePrivateObserver {
 public:
  explicit ExtensionRewardsServiceObserver(Profile* profile);
  ~ExtensionRewardsServiceObserver() override;

  // RewardsServiceObserver implementation
  void OnWalletInitialized(RewardsService* rewards_service,
                           int32_t result) override;
  void OnWalletProperties(RewardsService* rewards_service,
                          int error_code,
                          std::unique_ptr<brave_rewards::WalletProperties>
                              wallet_properties) override;
  void OnPublisherListNormalized(
      RewardsService* rewards_service,
      const brave_rewards::ContentSiteList& list) override;
  void OnExcludedSitesChanged(RewardsService* rewards_service,
                              std::string publisher_key,
                              bool excluded) override;

  void OnRecurringTipSaved(RewardsService* rewards_service,
                           bool success) override;

  void OnRecurringTipRemoved(RewardsService* rewards_service,
                             bool success) override;

  void OnPendingContributionRemoved(RewardsService* rewards_service,
                                    int32_t result) override;

  void OnReconcileComplete(
      RewardsService* rewards_service,
      unsigned int result,
      const std::string& contribution_id,
      const double amount,
      const int32_t type) override;

  void OnDisconnectWallet(
      brave_rewards::RewardsService* rewards_service,
      int32_t result,
      const std::string& wallet_type) override;

  void OnUnblindedTokensReady(
      brave_rewards::RewardsService* rewards_service) override;

  // RewardsServicePrivateObserver implementation
  void OnPanelPublisherInfo(
      RewardsService* rewards_service,
      int error_code,
      const ledger::PublisherInfo* info,
      uint64_t windowId) override;
  void OnFetchPromotions(
      RewardsService* rewards_service,
      const uint32_t result,
      const std::vector<brave_rewards::Promotion>& list) override;

  void OnPromotionFinished(
      RewardsService* rewards_service,
      const uint32_t result,
      brave_rewards::Promotion promotion) override;

  void OnRewardsMainEnabled(RewardsService* rewards_service,
                            bool rewards_main_enabled) override;

  void OnPendingContributionSaved(RewardsService* rewards_service,
                                  int result) override;

  void OnAdsEnabled(RewardsService* rewards_service,
                            bool ads_enabled) override;

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionRewardsServiceObserver);
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_EXTENSION_REWARDS_SERVICE_OBSERVER_H_
