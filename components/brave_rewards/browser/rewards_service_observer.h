/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_

#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/brave_rewards/browser/grant.h"

namespace brave_rewards {

class RewardsService;

class RewardsServiceObserver : public base::CheckedObserver {
 public:
  ~RewardsServiceObserver() override {}

  virtual void OnWalletInitialized(RewardsService* rewards_service,
                               int error_code) {};
  virtual void OnWalletProperties(
      RewardsService* rewards_service,
      int error_code,
      std::unique_ptr<brave_rewards::WalletProperties> properties) {};
  virtual void OnGrant(RewardsService* rewards_service,
                           unsigned int error_code,
                           brave_rewards::Grant properties) {};
  virtual void OnGrantCaptcha(RewardsService* rewards_service,
                                  std::string image) {};
  virtual void OnRecoverWallet(RewardsService* rewards_service,
                               unsigned int result,
                               double balance,
                               std::vector<brave_rewards::Grant> grants) {};
  virtual void OnGrantFinish(RewardsService* rewards_service,
                                 unsigned int result,
                                 brave_rewards::Grant grant) {};
  virtual void OnContentSiteUpdated(RewardsService* rewards_service) {};
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_
