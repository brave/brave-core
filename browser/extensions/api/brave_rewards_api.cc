/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_rewards_api.h"

#include <string>

#include "brave/browser/brave_rewards/donations_dialog.h"
#include "brave/common/extensions/api/brave_rewards.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "content/public/browser/web_contents.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
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

BraveRewardsDonateToSiteFunction::~BraveRewardsDonateToSiteFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsDonateToSiteFunction::Run() {
  std::unique_ptr<brave_rewards::DonateToSite::Params> params(
      brave_rewards::DonateToSite::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Sanity check: don't allow donations in private / tor contexts,
  // although the command should not have been enabled in the first place.
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (profile->IsOffTheRecord()) {
    return RespondNow(Error("Cannot donate to site in a private context"));
  }

  // Get web contents for this tab
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
        params->tab_id,
        profile,
        include_incognito_information(),
        nullptr,
        nullptr,
        &contents,
        nullptr)) {
    return RespondNow(Error(tabs_constants::kTabNotFoundError,
                            base::IntToString(params->tab_id)));
  }
  ::brave_rewards::OpenDonationDialog(contents, params->publisher_key);

  return RespondNow(NoArguments());
}

BraveRewardsGetPublisherDataFunction::~BraveRewardsGetPublisherDataFunction() {
}

BraveRewardsIncludeInAutoContributionFunction::
  ~BraveRewardsIncludeInAutoContributionFunction() {
}

ExtensionFunction::ResponseAction
  BraveRewardsIncludeInAutoContributionFunction::Run() {

  std::unique_ptr<brave_rewards::IncludeInAutoContribution::Params> params(
    brave_rewards::IncludeInAutoContribution::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ =
    RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_) {
    rewards_service_->SetContributionAutoInclude(
      params->publisher_key, params->excluded, params->window_id);
  }
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveRewardsGetPublisherDataFunction::Run() {
  std::unique_ptr<brave_rewards::GetPublisherData::Params> params(
      brave_rewards::GetPublisherData::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_) {
    rewards_service_->GetPublisherActivityFromUrl(params->window_id,
                                                  params->url,
                                                  params->favicon_url);
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetWalletPropertiesFunction::~BraveRewardsGetWalletPropertiesFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsGetWalletPropertiesFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_) {
    rewards_service_->FetchWalletProperties();
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
