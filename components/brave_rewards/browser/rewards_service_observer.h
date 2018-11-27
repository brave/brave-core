/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_OBSERVER_H_

#include "base/observer_list_types.h"
#include "brave/components/brave_rewards/browser/content_site.h"
#include "brave/components/brave_rewards/browser/grant.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"

namespace ledger {
struct PublisherInfo;
}

namespace brave_rewards {

class RewardsService;
struct WalletProperties;

class RewardsServiceObserver : public base::CheckedObserver {
 public:
  ~RewardsServiceObserver() override {}

  virtual void OnWalletInitialized(RewardsService* rewards_service,
                               int error_code) {};
  virtual void OnWalletProperties(
      RewardsService* rewards_service,
      int error_code,
      brave_rewards::WalletProperties* properties) {};
  virtual void OnGrant(RewardsService* rewards_service,
                           unsigned int error_code,
                           brave_rewards::Grant properties) {};
  virtual void OnGrantCaptcha(RewardsService* rewards_service,
                              std::string image,
                              std::string hint) {};
  virtual void OnRecoverWallet(RewardsService* rewards_service,
                               unsigned int result,
                               double balance,
                               std::vector<brave_rewards::Grant> grants) {};
  virtual void OnGrantFinish(RewardsService* rewards_service,
                                 unsigned int result,
                                 brave_rewards::Grant grant) {};
  virtual void OnContentSiteUpdated(RewardsService* rewards_service) {};
  virtual void OnExcludedSitesChanged(RewardsService* rewards_service) {};
  virtual void OnReconcileComplete(RewardsService* rewards_service,
                                   unsigned int result,
                                   const std::string& viewing_id,
                                   const std::string& probi) {};
  virtual void OnRecurringDonationUpdated(RewardsService* rewards_service,
                                          brave_rewards::ContentSiteList) {};
  virtual void OnCurrentTips(RewardsService* rewards_service,
                             brave_rewards::ContentSiteList) {};
  virtual void OnPublisherBanner(brave_rewards::RewardsService* rewards_service,
                                 const brave_rewards::PublisherBanner banner) {};
  virtual void OnGetPublisherActivityFromUrl(
      brave_rewards::RewardsService* rewards_service,
      int error_code,
      ledger::PublisherInfo* info,
      uint64_t windowId) {};
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_OBSERVER_H_
