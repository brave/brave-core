/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_rewards_api.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_rewards/rewards_tab_helper.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/brave_rewards/tip_panel_coordinator.h"
#include "brave/common/extensions/api/brave_rewards.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/event_router.h"

using brave_ads::AdsService;
using brave_ads::AdsServiceFactory;
using brave_rewards::RewardsPanelCoordinator;
using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;
using brave_rewards::RewardsTabHelper;
using brave_rewards::TipPanelCoordinator;

namespace {

RewardsTabHelper* GetRewardsTabHelperForTabId(
    int tab_id,
    content::BrowserContext* browser_context) {
  DCHECK(browser_context);
  content::WebContents* web_contents = nullptr;
  bool found = extensions::ExtensionTabUtil::GetTabById(
      tab_id, browser_context, false, nullptr, nullptr, &web_contents, nullptr);
  if (!found || !web_contents) {
    return nullptr;
  }
  return RewardsTabHelper::FromWebContents(web_contents);
}

content::WebContents* WebContentsFromBrowserContext(
    int tab_id,
    content::BrowserContext* browser_context) {
  content::WebContents* contents = nullptr;
  extensions::ExtensionTabUtil::GetTabById(
      tab_id, Profile::FromBrowserContext(browser_context), false, nullptr,
      nullptr, &contents, nullptr);
  return contents;
}

RewardsPanelCoordinator* GetPanelCoordinator(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  auto* browser = chrome::FindBrowserWithTab(web_contents);
  return browser ? RewardsPanelCoordinator::FromBrowser(browser) : nullptr;
}

RewardsPanelCoordinator* GetPanelCoordinator(ExtensionFunction* function) {
  DCHECK(function);
  auto* web_contents = function->GetSenderWebContents();
  if (!web_contents) {
    return nullptr;
  }
  return GetPanelCoordinator(web_contents);
}

TipPanelCoordinator* GetTipPanelCoordinator(
    int tab_id,
    content::BrowserContext* browser_context) {
  auto* contents = WebContentsFromBrowserContext(tab_id, browser_context);
  if (!contents) {
    return nullptr;
  }

  auto* browser = chrome::FindBrowserWithTab(contents);
  if (!browser) {
    return nullptr;
  }

  return brave_rewards::TipPanelCoordinator::FromBrowser(browser);
}

std::string StringifyResult(
    brave_rewards::mojom::CreateRewardsWalletResult result) {
  switch (result) {
    case brave_rewards::mojom::CreateRewardsWalletResult::kSuccess:
      return "success";
    case brave_rewards::mojom::CreateRewardsWalletResult::
        kWalletGenerationDisabled:
      return "wallet-generation-disabled";
    case brave_rewards::mojom::CreateRewardsWalletResult::
        kGeoCountryAlreadyDeclared:
      return "country-already-declared";
    case brave_rewards::mojom::CreateRewardsWalletResult::kUnexpected:
      return "unexpected-error";
  }
}

}  // namespace

namespace extensions::api {

BraveRewardsIsSupportedFunction::~BraveRewardsIsSupportedFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsIsSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  bool is_supported = ::brave_rewards::IsSupportedForProfile(profile);
  return RespondNow(WithArguments(is_supported));
}

BraveRewardsRecordNTPPanelTriggerFunction::
    ~BraveRewardsRecordNTPPanelTriggerFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsRecordNTPPanelTriggerFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(NoArguments());
  }

  if (!profile->GetPrefs()->GetBoolean(::brave_rewards::prefs::kEnabled)) {
    rewards_service->GetP3AConversionMonitor()->RecordPanelTrigger(
        ::brave_rewards::p3a::PanelTrigger::kNTP);
  }

  return RespondNow(NoArguments());
}

BraveRewardsOpenRewardsPanelFunction::~BraveRewardsOpenRewardsPanelFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsOpenRewardsPanelFunction::Run() {
  if (auto* coordinator = GetPanelCoordinator(this)) {
    coordinator->OpenRewardsPanel();
  }
  return RespondNow(NoArguments());
}

BraveRewardsUpdateMediaDurationFunction::
    ~BraveRewardsUpdateMediaDurationFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsUpdateMediaDurationFunction::Run() {
  std::optional<brave_rewards::UpdateMediaDuration::Params> params =
      brave_rewards::UpdateMediaDuration::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(NoArguments());
  }

  rewards_service->UpdateMediaDuration(params->window_id, params->publisher_key,
                                       params->duration, params->first_visit);

  return RespondNow(NoArguments());
}

BraveRewardsGetPublisherInfoFunction::~BraveRewardsGetPublisherInfoFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsGetPublisherInfoFunction::Run() {
  std::optional<brave_rewards::GetPublisherInfo::Params> params =
      brave_rewards::GetPublisherInfo::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->GetPublisherInfo(
      params->publisher_key,
      base::BindOnce(&BraveRewardsGetPublisherInfoFunction::OnGetPublisherInfo,
                     this));

  return RespondLater();
}

void BraveRewardsGetPublisherInfoFunction::OnGetPublisherInfo(
    const ::brave_rewards::mojom::Result result,
    ::brave_rewards::mojom::PublisherInfoPtr info) {
  if (!info) {
    Respond(WithArguments(static_cast<int>(result)));
    return;
  }

  base::Value::Dict dict;
  dict.Set("publisherKey", info->id);
  dict.Set("name", info->name);
  dict.Set("percentage", static_cast<int>(info->percent));
  dict.Set("status", static_cast<int>(info->status));
  dict.Set("excluded", info->excluded ==
                           ::brave_rewards::mojom::PublisherExclude::EXCLUDED);
  dict.Set("url", info->url);
  dict.Set("provider", info->provider);
  dict.Set("favIconUrl", info->favicon_url);

  Respond(WithArguments(static_cast<int>(result), std::move(dict)));
}

BraveRewardsSetPublisherIdForTabFunction::
    ~BraveRewardsSetPublisherIdForTabFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsSetPublisherIdForTabFunction::Run() {
  auto params = brave_rewards::SetPublisherIdForTab::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* tab_helper =
      GetRewardsTabHelperForTabId(params->tab_id, browser_context());

  if (tab_helper) {
    tab_helper->SetPublisherIdForTab(params->publisher_id);
  }

  return RespondNow(NoArguments());
}

BraveRewardsGetPublisherInfoForTabFunction::
    ~BraveRewardsGetPublisherInfoForTabFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetPublisherInfoForTabFunction::Run() {
  auto params = brave_rewards::GetPublisherInfoForTab::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* profile = Profile::FromBrowserContext(browser_context());

  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(NoArguments());
  }

  auto* tab_helper = GetRewardsTabHelperForTabId(params->tab_id, profile);
  if (!tab_helper) {
    return RespondNow(NoArguments());
  }

  std::string publisher_id = tab_helper->GetPublisherIdForTab();
  if (publisher_id.empty()) {
    return RespondNow(NoArguments());
  }

  rewards_service->GetPublisherPanelInfo(
      publisher_id,
      base::BindOnce(
          &BraveRewardsGetPublisherInfoForTabFunction::OnGetPublisherPanelInfo,
          this));

  return RespondLater();
}

void BraveRewardsGetPublisherInfoForTabFunction::OnGetPublisherPanelInfo(
    ::brave_rewards::mojom::Result result,
    ::brave_rewards::mojom::PublisherInfoPtr info) {
  if (!info) {
    Respond(NoArguments());
    return;
  }

  base::Value::Dict dict;
  dict.Set("publisherKey", info->id);
  dict.Set("name", info->name);
  dict.Set("percentage", static_cast<int>(info->percent));
  dict.Set("status", static_cast<int>(info->status));
  dict.Set("excluded", info->excluded ==
                           ::brave_rewards::mojom::PublisherExclude::EXCLUDED);
  dict.Set("url", info->url);
  dict.Set("provider", info->provider);
  dict.Set("favIconUrl", info->favicon_url);

  Respond(WithArguments(std::move(dict)));
}

BraveRewardsGetPublisherPanelInfoFunction::
    ~BraveRewardsGetPublisherPanelInfoFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetPublisherPanelInfoFunction::Run() {
  std::optional<brave_rewards::GetPublisherPanelInfo::Params> params =
      brave_rewards::GetPublisherPanelInfo::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not available"));
  }

  rewards_service->GetPublisherPanelInfo(
      params->publisher_key,
      base::BindOnce(
          &BraveRewardsGetPublisherPanelInfoFunction::OnGetPublisherPanelInfo,
          this));

  return RespondLater();
}

void BraveRewardsGetPublisherPanelInfoFunction::OnGetPublisherPanelInfo(
    const ::brave_rewards::mojom::Result result,
    ::brave_rewards::mojom::PublisherInfoPtr info) {
  if (!info) {
    Respond(WithArguments(static_cast<int>(result)));
    return;
  }

  base::Value::Dict dict;
  dict.Set("publisherKey", info->id);
  dict.Set("name", info->name);
  dict.Set("percentage", static_cast<int>(info->percent));
  dict.Set("status", static_cast<int>(info->status));
  dict.Set("excluded", info->excluded ==
                           ::brave_rewards::mojom::PublisherExclude::EXCLUDED);
  dict.Set("url", info->url);
  dict.Set("provider", info->provider);
  dict.Set("favIconUrl", info->favicon_url);

  Respond(WithArguments(static_cast<int>(result), std::move(dict)));
}

BraveRewardsSavePublisherInfoFunction::
    ~BraveRewardsSavePublisherInfoFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsSavePublisherInfoFunction::Run() {
  std::optional<brave_rewards::SavePublisherInfo::Params> params =
      brave_rewards::SavePublisherInfo::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not available"));
  }

  auto publisher_info = ::brave_rewards::mojom::PublisherInfo::New();
  publisher_info->id = params->publisher_key;
  publisher_info->name = params->publisher_name;
  publisher_info->url = params->url;
  publisher_info->provider = params->media_type;
  publisher_info->favicon_url = params->fav_icon_url;

  rewards_service->SavePublisherInfo(
      params->window_id, std::move(publisher_info),
      base::BindOnce(
          &BraveRewardsSavePublisherInfoFunction::OnSavePublisherInfo, this));

  return RespondLater();
}

void BraveRewardsSavePublisherInfoFunction::OnSavePublisherInfo(
    const ::brave_rewards::mojom::Result result) {
  Respond(WithArguments(static_cast<int>(result)));
}

BraveRewardsTipSiteFunction::~BraveRewardsTipSiteFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsTipSiteFunction::Run() {
  std::optional<brave_rewards::TipSite::Params> params =
      brave_rewards::TipSite::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  // Sanity check: don't allow tips in private / tor contexts,
  // although the command should not have been enabled in the first place.
  if (!Profile::FromBrowserContext(browser_context())->IsRegularProfile()) {
    return RespondNow(Error("Cannot tip to site in a private context"));
  }

  auto* coordinator = GetTipPanelCoordinator(params->tab_id, browser_context());
  if (!coordinator) {
    return RespondNow(Error(tabs_constants::kTabNotFoundError,
                            base::NumberToString(params->tab_id)));
  }

  coordinator->ShowPanelForPublisher(params->publisher_key);
  return RespondNow(NoArguments());
}

BraveRewardsIncludeInAutoContributionFunction::
    ~BraveRewardsIncludeInAutoContributionFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsIncludeInAutoContributionFunction::Run() {
  std::optional<brave_rewards::IncludeInAutoContribution::Params> params =
      brave_rewards::IncludeInAutoContribution::Params::Create(args());
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->SetPublisherExclude(params->publisher_key,
                                         params->exclude);
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetPublisherDataFunction::~BraveRewardsGetPublisherDataFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsGetPublisherDataFunction::Run() {
  std::optional<brave_rewards::GetPublisherData::Params> params =
      brave_rewards::GetPublisherData::Params::Create(args());
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->GetPublisherActivityFromUrl(params->window_id, params->url,
                                                 params->favicon_url,
                                                 params->publisher_blob);
  }
  return RespondNow(NoArguments());
}

BraveRewardsGetRewardsParametersFunction::
    ~BraveRewardsGetRewardsParametersFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetRewardsParametersFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not available"));
  }

  rewards_service->GetRewardsParameters(base::BindOnce(
      &BraveRewardsGetRewardsParametersFunction::OnGetRewardsParameters, this));
  return RespondLater();
}

void BraveRewardsGetRewardsParametersFunction::OnGetRewardsParameters(
    ::brave_rewards::mojom::RewardsParametersPtr parameters) {
  base::Value::Dict data;

  if (!parameters) {
    data.Set("rate", 0.0);
    data.Set("monthlyTipChoices", base::Value::List());
    data.Set("autoContributeChoices", base::Value::List());
    data.Set("payoutStatus", base::Value::Dict());
    data.Set("walletProviderRegions", base::Value::Dict());
    data.Set("vbatExpired", false);
    return Respond(WithArguments(std::move(data)));
  }

  data.Set("rate", parameters->rate);
  base::Value::List monthly_choices;
  for (auto const& item : parameters->monthly_tip_choices) {
    monthly_choices.Append(item);
  }
  data.Set("monthlyTipChoices", std::move(monthly_choices));

  base::Value::List ac_choices;
  for (double const& choice : parameters->auto_contribute_choices) {
    ac_choices.Append(choice);
  }
  data.Set("autoContributeChoices", std::move(ac_choices));

  base::Value::Dict payout_status;
  for (const auto& [key, value] : parameters->payout_status) {
    payout_status.Set(key, value);
  }
  data.Set("payoutStatus", std::move(payout_status));

  base::Value::Dict provider_regions;
  for (const auto& [provider, regions] : parameters->wallet_provider_regions) {
    base::Value::List allow;
    for (const auto& country : regions->allow) {
      allow.Append(country);
    }
    base::Value::List block;
    for (const auto& country : regions->block) {
      block.Append(country);
    }
    base::Value::Dict regions_dict;
    regions_dict.Set("allow", std::move(allow));
    regions_dict.Set("block", std::move(block));
    provider_regions.Set(provider, std::move(regions_dict));
  }
  data.Set("walletProviderRegions", std::move(provider_regions));

  if (!parameters->vbat_deadline.is_null()) {
    data.Set("vbatDeadline",
             floor(parameters->vbat_deadline.InSecondsFSinceUnixEpoch() *
                   base::Time::kMillisecondsPerSecond));
  }
  data.Set("vbatExpired", parameters->vbat_expired);

  Respond(WithArguments(std::move(data)));
}

BraveRewardsCreateRewardsWalletFunction::
    ~BraveRewardsCreateRewardsWalletFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsCreateRewardsWalletFunction::Run() {
  auto params = brave_rewards::CreateRewardsWallet::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("RewardsService not available"));
  }

  rewards_service->CreateRewardsWallet(
      params->country,
      base::BindOnce(
          &BraveRewardsCreateRewardsWalletFunction::CreateRewardsWalletCallback,
          this));

  return RespondLater();
}

void BraveRewardsCreateRewardsWalletFunction::CreateRewardsWalletCallback(
    ::brave_rewards::mojom::CreateRewardsWalletResult result) {
  Respond(WithArguments(StringifyResult(result)));
}

BraveRewardsGetAvailableCountriesFunction::
    ~BraveRewardsGetAvailableCountriesFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetAvailableCountriesFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->GetAvailableCountries(base::BindOnce(
      &BraveRewardsGetAvailableCountriesFunction::GetAvailableCountriesCallback,
      this));

  return RespondLater();
}

void BraveRewardsGetAvailableCountriesFunction::GetAvailableCountriesCallback(
    std::vector<std::string> countries) {
  base::Value::List country_list;
  for (auto& country : countries) {
    country_list.Append(std::move(country));
  }
  Respond(WithArguments(std::move(country_list)));
}

BraveRewardsGetDefaultCountryFunction::
    ~BraveRewardsGetDefaultCountryFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsGetDefaultCountryFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  return RespondNow(WithArguments(rewards_service->GetCountryCode()));
}

BraveRewardsGetDeclaredCountryFunction::
    ~BraveRewardsGetDeclaredCountryFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetDeclaredCountryFunction::Run() {
  auto* prefs = Profile::FromBrowserContext(browser_context())->GetPrefs();
  std::string country = prefs->GetString(::brave_rewards::prefs::kDeclaredGeo);
  return RespondNow(WithArguments(std::move(country)));
}

BraveRewardsGetUserTypeFunction::~BraveRewardsGetUserTypeFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsGetUserTypeFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(WithArguments(std::string()));
  }

  rewards_service->GetUserType(
      base::BindOnce(&BraveRewardsGetUserTypeFunction::Callback, this));

  return RespondLater();
}

void BraveRewardsGetUserTypeFunction::Callback(
    ::brave_rewards::mojom::UserType user_type) {
  auto map_user_type =
      [](::brave_rewards::mojom::UserType user_type) -> std::string {
    switch (user_type) {
      case ::brave_rewards::mojom::UserType::kConnected:
        return "connected";
      case ::brave_rewards::mojom::UserType::kUnconnected:
        return "unconnected";
    }
  };
  Respond(WithArguments(map_user_type(user_type)));
}

BraveRewardsGetPublishersVisitedCountFunction::
    ~BraveRewardsGetPublishersVisitedCountFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetPublishersVisitedCountFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(WithArguments(0));
  }

  rewards_service->GetPublishersVisitedCount(base::BindOnce(
      &BraveRewardsGetPublishersVisitedCountFunction::Callback, this));

  return RespondLater();
}

void BraveRewardsGetPublishersVisitedCountFunction::Callback(int count) {
  Respond(WithArguments(count));
}

BraveRewardsGetBalanceReportFunction::~BraveRewardsGetBalanceReportFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsGetBalanceReportFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not available"));
  }

  std::optional<brave_rewards::GetBalanceReport::Params> params =
      brave_rewards::GetBalanceReport::Params::Create(args());

  rewards_service->GetBalanceReport(
      params->month, params->year,
      base::BindOnce(&BraveRewardsGetBalanceReportFunction::OnBalanceReport,
                     this));
  return RespondLater();
}

void BraveRewardsGetBalanceReportFunction::OnBalanceReport(
    const ::brave_rewards::mojom::Result result,
    ::brave_rewards::mojom::BalanceReportInfoPtr report) {
  base::Value::Dict data;
  data.Set("ads", report ? report->earning_from_ads : 0.0);
  data.Set("contribute", report ? report->auto_contribute : 0.0);
  data.Set("tips", report ? report->one_time_donation : 0.0);
  data.Set("monthly", report ? report->recurring_donation : 0.0);
  Respond(WithArguments(std::move(data)));
}

BraveRewardsSetAutoContributeEnabledFunction::
    ~BraveRewardsSetAutoContributeEnabledFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsSetAutoContributeEnabledFunction::Run() {
  std::optional<brave_rewards::SetAutoContributeEnabled::Params> params =
      brave_rewards::SetAutoContributeEnabled::Params::Create(args());
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->SetAutoContributeEnabled(params->enabled);
  return RespondNow(NoArguments());
}

BraveRewardsGetACEnabledFunction::~BraveRewardsGetACEnabledFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsGetACEnabledFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->GetAutoContributeEnabled(
      base::BindOnce(&BraveRewardsGetACEnabledFunction::OnGetACEnabled, this));
  return RespondLater();
}

void BraveRewardsGetACEnabledFunction::OnGetACEnabled(bool enabled) {
  Respond(WithArguments(enabled));
}

BraveRewardsSaveRecurringTipFunction::~BraveRewardsSaveRecurringTipFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsSaveRecurringTipFunction::Run() {
  std::optional<brave_rewards::SaveRecurringTip::Params> params =
      brave_rewards::SaveRecurringTip::Params::Create(args());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(NoArguments());
  }

  rewards_service->SaveRecurringTip(
      params->publisher_key, params->new_amount,
      base::BindOnce(&BraveRewardsSaveRecurringTipFunction::OnSaveRecurringTip,
                     this));

  return RespondLater();
}

void BraveRewardsSaveRecurringTipFunction::OnSaveRecurringTip(
    ::brave_rewards::mojom::Result result) {
  Respond(result == ::brave_rewards::mojom::Result::OK
              ? NoArguments()
              : Error("Failed to save"));
}

BraveRewardsRemoveRecurringTipFunction::
    ~BraveRewardsRemoveRecurringTipFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsRemoveRecurringTipFunction::Run() {
  std::optional<brave_rewards::RemoveRecurringTip::Params> params =
      brave_rewards::RemoveRecurringTip::Params::Create(args());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (rewards_service) {
    rewards_service->RemoveRecurringTip(params->publisher_key);
  }

  return RespondNow(NoArguments());
}

BraveRewardsGetRecurringTipsFunction::~BraveRewardsGetRecurringTipsFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsGetRecurringTipsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->GetRecurringTips(base::BindOnce(
      &BraveRewardsGetRecurringTipsFunction::OnGetRecurringTips, this));
  return RespondLater();
}

void BraveRewardsGetRecurringTipsFunction::OnGetRecurringTips(
    std::vector<::brave_rewards::mojom::PublisherInfoPtr> list) {
  base::Value::Dict result;
  base::Value::List recurring_tips;

  if (!list.empty()) {
    for (const auto& item : list) {
      base::Value::Dict tip;
      tip.Set("publisherKey", item->id);
      tip.Set("amount", item->weight);
      recurring_tips.Append(std::move(tip));
    }
  }

  result.Set("recurringTips", std::move(recurring_tips));
  Respond(WithArguments(std::move(result)));
}

BraveRewardsRefreshPublisherFunction::~BraveRewardsRefreshPublisherFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsRefreshPublisherFunction::Run() {
  std::optional<brave_rewards::RefreshPublisher::Params> params =
      brave_rewards::RefreshPublisher::Params::Create(args());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(WithArguments(false, std::string()));
  }
  rewards_service->RefreshPublisher(
      params->publisher_key,
      base::BindOnce(&BraveRewardsRefreshPublisherFunction::OnRefreshPublisher,
                     this));
  return RespondLater();
}

void BraveRewardsRefreshPublisherFunction::OnRefreshPublisher(
    const ::brave_rewards::mojom::PublisherStatus status,
    const std::string& publisher_key) {
  Respond(WithArguments(static_cast<int>(status), publisher_key));
}

BraveRewardsGetAllNotificationsFunction::
    ~BraveRewardsGetAllNotificationsFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetAllNotificationsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  base::Value::List list;

  if (!rewards_service) {
    return RespondNow(WithArguments(std::move(list)));
  }

  auto notifications = rewards_service->GetAllNotifications();

  for (auto const& notification : notifications) {
    base::Value::Dict item;
    item.Set("id", notification.second.id_);
    item.Set("type", notification.second.type_);
    item.Set("timestamp", static_cast<double>(notification.second.timestamp_));

    base::Value::List args;
    for (auto const& arg : notification.second.args_) {
      args.Append(arg);
    }

    item.Set("args", std::move(args));
    list.Append(std::move(item));
  }

  return RespondNow(WithArguments(std::move(list)));
}

BraveRewardsFetchBalanceFunction::~BraveRewardsFetchBalanceFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsFetchBalanceFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not available"));
  }

  rewards_service->FetchBalance(
      base::BindOnce(&BraveRewardsFetchBalanceFunction::OnFetchBalance, this));

  return RespondLater();
}

void BraveRewardsFetchBalanceFunction::OnFetchBalance(
    ::brave_rewards::mojom::BalancePtr balance) {
  Respond(balance ? WithArguments(balance->total) : NoArguments());
}

BraveRewardsGetExternalWalletProvidersFunction::
    ~BraveRewardsGetExternalWalletProvidersFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetExternalWalletProvidersFunction::Run() {
  base::Value::List providers;

  auto* profile = Profile::FromBrowserContext(browser_context());
  if (auto* rewards_service = RewardsServiceFactory::GetForProfile(profile)) {
    for (auto& provider : rewards_service->GetExternalWalletProviders()) {
      providers.Append(provider);
    }
  }
  return RespondNow(WithArguments(std::move(providers)));
}

BraveRewardsGetExternalWalletFunction::
    ~BraveRewardsGetExternalWalletFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsGetExternalWalletFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not available"));
  }

  rewards_service->GetExternalWallet(base::BindOnce(
      &BraveRewardsGetExternalWalletFunction::OnGetExternalWallet, this));
  return RespondLater();
}

void BraveRewardsGetExternalWalletFunction::OnGetExternalWallet(
    ::brave_rewards::mojom::ExternalWalletPtr wallet) {
  if (!wallet) {
    return Respond(NoArguments());
  }

  base::Value::Dict data;
  data.Set("type", wallet->type);
  data.Set("address", wallet->address);
  data.Set("status", static_cast<int>(wallet->status));
  data.Set("userName", wallet->user_name);
  data.Set("accountUrl", wallet->account_url);
  data.Set("activityUrl", wallet->activity_url);

  Respond(WithArguments(std::move(data)));
}

BraveRewardsGetRewardsEnabledFunction::
    ~BraveRewardsGetRewardsEnabledFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsGetRewardsEnabledFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  bool enabled =
      profile->GetPrefs()->GetBoolean(::brave_rewards::prefs::kEnabled);
  return RespondNow(WithArguments(enabled));
}

BraveRewardsGetAdsAccountStatementFunction::
    ~BraveRewardsGetAdsAccountStatementFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetAdsAccountStatementFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);

  if (!ads_service) {
    return RespondNow(Error("Ads service is not initialized"));
  }

  AddRef();  // Balanced in OnGetAdsAccountStatement().

  ads_service->GetStatementOfAccounts(base::BindOnce(
      &BraveRewardsGetAdsAccountStatementFunction::OnGetAdsAccountStatement,
      this));
  return RespondLater();
}

void BraveRewardsGetAdsAccountStatementFunction::OnGetAdsAccountStatement(
    brave_ads::mojom::StatementInfoPtr statement) {
  if (!statement) {
    Respond(WithArguments(false));
  } else {
    base::Value::Dict dict;
    dict.Set("nextPaymentDate",
             statement->next_payment_date.InSecondsFSinceUnixEpoch() * 1000);
    dict.Set("adsReceivedThisMonth", statement->ads_received_this_month);
    dict.Set("minEarningsThisMonth", statement->min_earnings_this_month);
    dict.Set("maxEarningsThisMonth", statement->max_earnings_this_month);
    dict.Set("minEarningsLastMonth", statement->min_earnings_previous_month);
    dict.Set("maxEarningsLastMonth", statement->max_earnings_previous_month);

    Respond(WithArguments(true, std::move(dict)));
  }

  Release();  // Balanced in Run()
}

BraveRewardsIsInitializedFunction::~BraveRewardsIsInitializedFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsIsInitializedFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  return RespondNow(
      WithArguments(rewards_service && rewards_service->IsInitialized()));
}

BraveRewardsSelfCustodyInviteDismissedFunction::
    ~BraveRewardsSelfCustodyInviteDismissedFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsSelfCustodyInviteDismissedFunction::Run() {
  auto* prefs = Profile::FromBrowserContext(browser_context())->GetPrefs();
  return RespondNow(WithArguments(
      prefs->GetBoolean(::brave_rewards::prefs::kSelfCustodyInviteDismissed)));
}

BraveRewardsDismissSelfCustodyInviteFunction::
    ~BraveRewardsDismissSelfCustodyInviteFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsDismissSelfCustodyInviteFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  profile->GetPrefs()->SetBoolean(
      ::brave_rewards::prefs::kSelfCustodyInviteDismissed, true);
  if (auto* event_router = extensions::EventRouter::Get(profile)) {
    event_router->BroadcastEvent(std::make_unique<Event>(
        events::BRAVE_START,
        brave_rewards::OnSelfCustodyInviteDismissed::kEventName,
        base::Value::List()));
  }
  return RespondNow(NoArguments());
}

BraveRewardsIsTermsOfServiceUpdateRequiredFunction::
    ~BraveRewardsIsTermsOfServiceUpdateRequiredFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsIsTermsOfServiceUpdateRequiredFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  return RespondNow(
      WithArguments(rewards_service->IsTermsOfServiceUpdateRequired()));
}

BraveRewardsAcceptTermsOfServiceUpdateFunction::
    ~BraveRewardsAcceptTermsOfServiceUpdateFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsAcceptTermsOfServiceUpdateFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->AcceptTermsOfServiceUpdate();
  return RespondNow(NoArguments());
}

BraveRewardsGetScheduledCaptchaInfoFunction::
    ~BraveRewardsGetScheduledCaptchaInfoFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetScheduledCaptchaInfoFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* brave_adaptive_captcha_service =
      brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::GetForProfile(
          profile);
  if (!brave_adaptive_captcha_service) {
    return RespondNow(
        Error("Adaptive captcha service called from incognito or unsupported "
              "profile"));
  }

  std::string url;
  bool max_attempts_exceeded = false;
  brave_adaptive_captcha_service->GetScheduledCaptchaInfo(
      &url, &max_attempts_exceeded);

  base::Value::Dict dict;
  dict.Set("url", url);
  dict.Set("maxAttemptsExceeded", max_attempts_exceeded);

  return RespondNow(WithArguments(std::move(dict)));
}

BraveRewardsUpdateScheduledCaptchaResultFunction::
    ~BraveRewardsUpdateScheduledCaptchaResultFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsUpdateScheduledCaptchaResultFunction::Run() {
  auto params =
      brave_rewards::UpdateScheduledCaptchaResult::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* brave_adaptive_captcha_service =
      brave_adaptive_captcha::BraveAdaptiveCaptchaServiceFactory::GetForProfile(
          profile);
  if (!brave_adaptive_captcha_service) {
    return RespondNow(
        Error("Adaptive captcha service called from incognito or unsupported "
              "profile"));
  }

  brave_adaptive_captcha_service->UpdateScheduledCaptchaResult(params->result);

  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);
  if (ads_service && params->result) {
    ads_service->NotifyDidSolveAdaptiveCaptcha();
  }

  return RespondNow(NoArguments());
}

BraveRewardsGetPrefsFunction::~BraveRewardsGetPrefsFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsGetPrefsFunction::Run() {
  auto* rewards_service = RewardsServiceFactory::GetForProfile(
      Profile::FromBrowserContext(browser_context()));

  if (!rewards_service)
    return RespondNow(Error("Rewards service is not initialized"));

  rewards_service->GetAutoContributeProperties(base::BindRepeating(
      &BraveRewardsGetPrefsFunction::GetAutoContributePropertiesCallback,
      this));

  return RespondLater();
}

void BraveRewardsGetPrefsFunction::GetAutoContributePropertiesCallback(
    ::brave_rewards::mojom::AutoContributePropertiesPtr properties) {
  base::Value::Dict prefs;
  prefs.Set("autoContributeEnabled",
            properties ? properties->enabled_contribute : false);
  prefs.Set("autoContributeAmount", properties ? properties->amount : 0.0);
  Respond(WithArguments(std::move(prefs)));
}

}  // namespace extensions::api
