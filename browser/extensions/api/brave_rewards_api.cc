/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_rewards_api.h"

#include "brave/common/extensions/api/brave_rewards.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "chrome/browser/profiles/profile.h"

using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;

namespace extensions {
namespace api {

BraveRewardsCreateWalletFunction::~BraveRewardsCreateWalletFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsCreateWalletFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_) {
    rewards_service_->CreateWallet();
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetPublisherDataFunction::~BraveRewardsGetPublisherDataFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsGetPublisherDataFunction::Run() {
  std::unique_ptr<brave_rewards::GetPublisherData::Params> params(
      brave_rewards::GetPublisherData::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_) {
    rewards_service_->GetPublisherActivityFromUrl(params->window_id, params->url);
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetWalletPropertiesFunction::~BraveRewardsGetWalletPropertiesFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsGetWalletPropertiesFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_) {
    rewards_service_->GetWalletProperties();
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetCurrentReportFunction::~BraveRewardsGetCurrentReportFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsGetCurrentReportFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_) {
    rewards_service_->GetCurrentBalanceReport();
  }
  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
