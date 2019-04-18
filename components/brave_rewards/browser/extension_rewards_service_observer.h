/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_EXTENSION_REWARDS_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_EXTENSION_REWARDS_SERVICE_OBSERVER_H_

#include <memory>
#include <string>

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
                           uint32_t result) override;
  void OnWalletProperties(RewardsService* rewards_service,
                          int error_code,
                          std::unique_ptr<brave_rewards::WalletProperties>
                              wallet_properties) override;
  void OnPublisherListNormalized(
      RewardsService* rewards_service,
      brave_rewards::ContentSiteList list) override;
  void OnExcludedSitesChanged(RewardsService* rewards_service,
                              std::string publisher_key,
                              bool excluded) override;

  void OnRecurringTipSaved(RewardsService* rewards_service,
                           bool success) override;

  void OnRecurringTipRemoved(RewardsService* rewards_service,
                             bool success) override;

  // RewardsServicePrivateObserver implementation
  void OnGetCurrentBalanceReport(RewardsService* rewards_service,
                                 const BalanceReport& balance_report) override;
  void OnPanelPublisherInfo(
      RewardsService* rewards_service,
      int error_code,
      std::unique_ptr<ledger::PublisherInfo> info,
      uint64_t windowId) override;
  void OnGrant(RewardsService* rewards_service,
               unsigned int result,
               brave_rewards::Grant grant) override;
  void OnGrantCaptcha(RewardsService* rewards_service,
                      std::string image,
                      std::string hint) override;
  void OnGrantFinish(RewardsService* rewards_service,
                     unsigned int result,
                     brave_rewards::Grant grant) override;
  void OnRewardsMainEnabled(RewardsService* rewards_service,
                            bool rewards_main_enabled) override;

  void OnPendingContributionSaved(RewardsService* rewards_service,
                                  int result) override;

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionRewardsServiceObserver);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_EXTENSION_REWARDS_SERVICE_OBSERVER_H_
