/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_EXTENSION_REWARDS_SERVICE_OBSERVER_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_EXTENSION_REWARDS_SERVICE_OBSERVER_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_private_observer.h"

class Profile;

namespace brave_rewards {

class RewardsService;

class ExtensionRewardsServiceObserver : public RewardsServiceObserver,
                                        public RewardsServicePrivateObserver {
 public:
  explicit ExtensionRewardsServiceObserver(Profile* profile);
  ExtensionRewardsServiceObserver(const ExtensionRewardsServiceObserver&) =
      delete;
  ExtensionRewardsServiceObserver& operator=(
      const ExtensionRewardsServiceObserver&) = delete;
  ~ExtensionRewardsServiceObserver() override;

  // RewardsServiceObserver implementation
  void OnRewardsInitialized(RewardsService* rewards_service) override;

  void OnPublisherListNormalized(
      RewardsService* rewards_service,
      ledger::type::PublisherInfoList list) override;
  void OnExcludedSitesChanged(RewardsService* rewards_service,
                              std::string publisher_key,
                              bool excluded) override;

  void OnRecurringTipSaved(RewardsService* rewards_service,
                           bool success) override;

  void OnRecurringTipRemoved(RewardsService* rewards_service,
                             bool success) override;

  void OnPendingContributionRemoved(
      RewardsService* rewards_service,
      const ledger::type::Result result) override;

  void OnReconcileComplete(
      RewardsService* rewards_service,
      const ledger::type::Result result,
      const std::string& contribution_id,
      const double amount,
      const ledger::type::RewardsType type,
      const ledger::type::ContributionProcessor processor) override;

  void OnDisconnectWallet(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result,
      const std::string& wallet_type) override;

  void OnUnblindedTokensReady(
      brave_rewards::RewardsService* rewards_service) override;

  // RewardsServicePrivateObserver implementation
  void OnPanelPublisherInfo(
      RewardsService* rewards_service,
      const ledger::type::Result result,
      const ledger::type::PublisherInfo* info,
      uint64_t windowId) override;
  void OnFetchPromotions(
      RewardsService* rewards_service,
      const ledger::type::Result result,
      const ledger::type::PromotionList& list) override;

  void OnPromotionFinished(
      RewardsService* rewards_service,
      const ledger::type::Result result,
      ledger::type::PromotionPtr promotion) override;

  void OnPendingContributionSaved(
      RewardsService* rewards_service,
      const ledger::type::Result result) override;

  void OnAdsEnabled(RewardsService* rewards_service,
                            bool ads_enabled) override;

  void OnCompleteReset(const bool success) override;

 private:
  Profile* profile_;
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_EXTENSION_REWARDS_SERVICE_OBSERVER_H_
