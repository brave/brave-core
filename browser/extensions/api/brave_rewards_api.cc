/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_rewards_api.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "brave/browser/brave_rewards/donations_dialog.h"
#include "brave/common/extensions/api/brave_rewards.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "content/public/browser/web_contents.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"

using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;
using brave_ads::AdsService;
using brave_ads::AdsServiceFactory;

namespace extensions {
namespace api {

BraveRewardsCreateWalletFunction::~BraveRewardsCreateWalletFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsCreateWalletFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->CreateWallet();
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
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->SetContributionAutoInclude(
      params->publisher_key, params->excluded);
  }
  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction BraveRewardsGetPublisherDataFunction::Run() {
  std::unique_ptr<brave_rewards::GetPublisherData::Params> params(
      brave_rewards::GetPublisherData::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->GetPublisherActivityFromUrl(params->window_id,
                                                  params->url,
                                                  params->favicon_url,
                                                  params->publisher_blob);
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetWalletPropertiesFunction::
~BraveRewardsGetWalletPropertiesFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetWalletPropertiesFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->FetchWalletProperties();
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetCurrentReportFunction::~BraveRewardsGetCurrentReportFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsGetCurrentReportFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->GetCurrentBalanceReport();
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetGrantsFunction::~BraveRewardsGetGrantsFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsGetGrantsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->FetchGrants(std::string(), std::string());
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetGrantCaptchaFunction::~BraveRewardsGetGrantCaptchaFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsGetGrantCaptchaFunction::Run() {
  std::unique_ptr<brave_rewards::GetGrantCaptcha::Params> params(
      brave_rewards::GetGrantCaptcha::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->GetGrantCaptcha(params->promotion_id,
                                     params->type);
  }
  return RespondNow(NoArguments());
}

BraveRewardsSolveGrantCaptchaFunction::
~BraveRewardsSolveGrantCaptchaFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsSolveGrantCaptchaFunction::Run() {
  std::unique_ptr<brave_rewards::SolveGrantCaptcha::Params> params(
      brave_rewards::SolveGrantCaptcha::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->SolveGrantCaptcha(params->solution, params->promotion_id);
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetPendingContributionsTotalFunction::
~BraveRewardsGetPendingContributionsTotalFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetPendingContributionsTotalFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(OneArgument(
          std::make_unique<base::Value>(0.0)));
  }

  rewards_service->GetPendingContributionsTotal(base::Bind(
        &BraveRewardsGetPendingContributionsTotalFunction::OnGetPendingTotal,
        this));
  return RespondLater();
}

void BraveRewardsGetPendingContributionsTotalFunction::OnGetPendingTotal(
    double amount) {
  Respond(OneArgument(std::make_unique<base::Value>(amount)));
}

BraveRewardsGetRewardsMainEnabledFunction::
~BraveRewardsGetRewardsMainEnabledFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetRewardsMainEnabledFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->GetRewardsMainEnabled(base::Bind(
        &BraveRewardsGetRewardsMainEnabledFunction::OnGetRewardsMainEnabled,
        this));
  return RespondLater();
}

void BraveRewardsGetRewardsMainEnabledFunction::OnGetRewardsMainEnabled(
    bool enabled) {
  Respond(OneArgument(std::make_unique<base::Value>(enabled)));
}

BraveRewardsSaveAdsSettingFunction::~BraveRewardsSaveAdsSettingFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsSaveAdsSettingFunction::Run() {
  std::unique_ptr<brave_rewards::SaveAdsSetting::Params> params(
      brave_rewards::SaveAdsSetting::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service_ = AdsServiceFactory::GetForProfile(profile);
  if (ads_service_) {
    if (params->key == "adsEnabled") {
      ads_service_->set_ads_enabled(params->value == "true");
    }
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetACEnabledFunction::
~BraveRewardsGetACEnabledFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetACEnabledFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->GetAutoContribute(base::BindOnce(
        &BraveRewardsGetACEnabledFunction::OnGetACEnabled,
        this));
  return RespondLater();
}

void BraveRewardsGetACEnabledFunction::OnGetACEnabled(bool enabled) {
  Respond(OneArgument(std::make_unique<base::Value>(enabled)));
}

BraveRewardsSaveSettingFunction::~BraveRewardsSaveSettingFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsSaveSettingFunction::Run() {
  std::unique_ptr<brave_rewards::SaveSetting::Params> params(
      brave_rewards::SaveSetting::Params::Create(*args_));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  if (rewards_service) {
    if (params->key == "enabledMain") {
      rewards_service->SetRewardsMainEnabled(
          std::stoi(params->value.c_str()));
    }
  }

  return RespondNow(NoArguments());
}

BraveRewardsSaveRecurringDonationFunction::
~BraveRewardsSaveRecurringDonationFunction() {
}

ExtensionFunction::ResponseAction
  BraveRewardsSaveRecurringDonationFunction::Run() {

  std::unique_ptr<brave_rewards::SaveRecurringDonation::Params> params(
    brave_rewards::SaveRecurringDonation::Params::Create(*args_));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ =
    RewardsServiceFactory::GetForProfile(profile);

  if (rewards_service_) {
    rewards_service_->SaveRecurringDonation(
      params->publisher_key, params->new_amount);
  }

  return RespondNow(NoArguments());
}

BraveRewardsRemoveRecurringDonationFunction::
~BraveRewardsRemoveRecurringDonationFunction() {
}

ExtensionFunction::ResponseAction
  BraveRewardsRemoveRecurringDonationFunction::Run() {
  std::unique_ptr<brave_rewards::RemoveRecurringDonation::Params> params(
    brave_rewards::RemoveRecurringDonation::Params::Create(*args_));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ =
    RewardsServiceFactory::GetForProfile(profile);

  if (rewards_service_) {
    rewards_service_->RemoveRecurring(params->publisher_key);
  }

  return RespondNow(NoArguments());
}

BraveRewardsGetRecurringDonationsFunction::
~BraveRewardsGetRecurringDonationsFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetRecurringDonationsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->GetRecurringDonationsList(base::Bind(
        &BraveRewardsGetRecurringDonationsFunction::OnGetRecurringDonations,
        this));
  return RespondLater();
}

void BraveRewardsGetRecurringDonationsFunction::OnGetRecurringDonations(
    std::unique_ptr<::brave_rewards::ContentSiteList> list) {
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  auto recurringDonations = std::make_unique<base::ListValue>();

  if (!list->empty()) {
    for (auto const& item: *list) {
      auto recurringDonation = std::make_unique<base::DictionaryValue>();
      recurringDonation->SetString("publisherKey", item.id);
      recurringDonation->SetInteger("amount", item.weight);
      recurringDonations->Append(std::move(recurringDonation));
    }
  }

  result->SetList("recurringDonations", std::move(recurringDonations));
  Respond(OneArgument(std::move(result)));
}

BraveRewardsGetPublisherBannerFunction::
~BraveRewardsGetPublisherBannerFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetPublisherBannerFunction::Run() {
  std::unique_ptr<brave_rewards::GetPublisherBanner::Params> params(
    brave_rewards::GetPublisherBanner::Params::Create(*args_));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->GetPublisherBanner(
      params->publisher_key,
      base::BindOnce(
        &BraveRewardsGetPublisherBannerFunction::OnPublisherBanner,
        this));
  return RespondLater();
}

void BraveRewardsGetPublisherBannerFunction::OnPublisherBanner(
    std::unique_ptr<::brave_rewards::PublisherBanner> banner) {
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());

  if (banner) {
    result->SetString("publisherKey", banner->publisher_key);
    result->SetString("title", banner->title);
    result->SetString("name", banner->name);
    result->SetString("description", banner->description);
    result->SetString("background", banner->background);
    result->SetString("logo", banner->logo);
    result->SetString("provider", banner->provider);
    result->SetBoolean("verified", banner->verified);

    auto amounts = std::make_unique<base::ListValue>();
    for (int const& value : banner->amounts) {
      amounts->AppendInteger(value);
    }
    result->SetList("amounts", std::move(amounts));

    auto social = std::make_unique<base::DictionaryValue>();
    for (auto const& item : banner->social) {
      social->SetString(item.first, item.second);
    }
    result->SetDictionary("social", std::move(social));
  }

  Respond(OneArgument(std::move(result)));
}

}  // namespace api
}  // namespace extensions
