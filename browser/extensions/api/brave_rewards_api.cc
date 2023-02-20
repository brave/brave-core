/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_rewards_api.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/version.h"
#include "bat/ads/supported_subdivisions.h"
#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_panel/rewards_panel_coordinator.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_rewards/rewards_tab_helper.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/brave_rewards/tip_dialog.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/extensions/api/brave_rewards.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "brave/components/l10n/common/locale_util.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"

using brave_ads::AdsService;
using brave_ads::AdsServiceFactory;
using brave_rewards::RewardsPanelCoordinator;
using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;
using brave_rewards::RewardsTabHelper;

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
  auto* browser = chrome::FindBrowserWithWebContents(web_contents);
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

std::string StringifyResult(ledger::mojom::CreateRewardsWalletResult result) {
  switch (result) {
    case ledger::mojom::CreateRewardsWalletResult::kSuccess:
      return "success";
    case ledger::mojom::CreateRewardsWalletResult::kWalletGenerationDisabled:
      return "wallet-generation-disabled";
    case ledger::mojom::CreateRewardsWalletResult::kGeoCountryAlreadyDeclared:
      return "country-already-declared";
    case ledger::mojom::CreateRewardsWalletResult::kUnexpected:
      return "unexpected-error";
  }
}

}  // namespace

namespace extensions {
namespace api {

BraveRewardsIsSupportedFunction::~BraveRewardsIsSupportedFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsIsSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  bool is_supported = ::brave_rewards::IsSupportedForProfile(
      profile, ::brave_rewards::IsSupportedOptions::kSkipRegionCheck);
  return RespondNow(WithArguments(is_supported));
}

BraveRewardsIsUnsupportedRegionFunction::
    ~BraveRewardsIsUnsupportedRegionFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsIsUnsupportedRegionFunction::Run() {
  bool is_unsupported_region = ::brave_rewards::IsUnsupportedRegion();
  return RespondNow(WithArguments(is_unsupported_region));
}

BraveRewardsGetLocaleFunction::~BraveRewardsGetLocaleFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsGetLocaleFunction::Run() {
  return RespondNow(WithArguments(brave_l10n::GetDefaultLocaleString()));
}

BraveRewardsOpenRewardsPanelFunction::~BraveRewardsOpenRewardsPanelFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsOpenRewardsPanelFunction::Run() {
  if (auto* coordinator = GetPanelCoordinator(this)) {
    coordinator->OpenRewardsPanel();
  }
  return RespondNow(NoArguments());
}

BraveRewardsShowGrantCaptchaFunction::~BraveRewardsShowGrantCaptchaFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsShowGrantCaptchaFunction::Run() {
  auto params = brave_rewards::ShowGrantCaptcha::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params.get());

  if (auto* coordinator = GetPanelCoordinator(this)) {
    coordinator->ShowGrantCaptcha(params->grant_id);
  }

  return RespondNow(NoArguments());
}

BraveRewardsUpdateMediaDurationFunction::
    ~BraveRewardsUpdateMediaDurationFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsUpdateMediaDurationFunction::Run() {
  std::unique_ptr<brave_rewards::UpdateMediaDuration::Params> params(
      brave_rewards::UpdateMediaDuration::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

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
  std::unique_ptr<brave_rewards::GetPublisherInfo::Params> params(
      brave_rewards::GetPublisherInfo::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

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
    const ledger::mojom::Result result,
    ledger::mojom::PublisherInfoPtr info) {
  if (!info) {
    Respond(WithArguments(static_cast<int>(result)));
    return;
  }

  base::Value::Dict dict;
  dict.Set("publisherKey", info->id);
  dict.Set("name", info->name);
  dict.Set("percentage", static_cast<int>(info->percent));
  dict.Set("status", static_cast<int>(info->status));
  dict.Set("excluded",
           info->excluded == ledger::mojom::PublisherExclude::EXCLUDED);
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
  EXTENSION_FUNCTION_VALIDATE(params.get());

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
  EXTENSION_FUNCTION_VALIDATE(params.get());

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
    ledger::mojom::Result result,
    ledger::mojom::PublisherInfoPtr info) {
  if (!info) {
    Respond(NoArguments());
    return;
  }

  base::Value::Dict dict;
  dict.Set("publisherKey", info->id);
  dict.Set("name", info->name);
  dict.Set("percentage", static_cast<int>(info->percent));
  dict.Set("status", static_cast<int>(info->status));
  dict.Set("excluded",
           info->excluded == ledger::mojom::PublisherExclude::EXCLUDED);
  dict.Set("url", info->url);
  dict.Set("provider", info->provider);
  dict.Set("favIconUrl", info->favicon_url);

  Respond(WithArguments(std::move(dict)));
}

BraveRewardsGetPublisherPanelInfoFunction::
    ~BraveRewardsGetPublisherPanelInfoFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetPublisherPanelInfoFunction::Run() {
  std::unique_ptr<brave_rewards::GetPublisherPanelInfo::Params> params(
      brave_rewards::GetPublisherPanelInfo::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

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
    const ledger::mojom::Result result,
    ledger::mojom::PublisherInfoPtr info) {
  if (!info) {
    Respond(WithArguments(static_cast<int>(result)));
    return;
  }

  base::Value::Dict dict;
  dict.Set("publisherKey", info->id);
  dict.Set("name", info->name);
  dict.Set("percentage", static_cast<int>(info->percent));
  dict.Set("status", static_cast<int>(info->status));
  dict.Set("excluded",
           info->excluded == ledger::mojom::PublisherExclude::EXCLUDED);
  dict.Set("url", info->url);
  dict.Set("provider", info->provider);
  dict.Set("favIconUrl", info->favicon_url);

  Respond(WithArguments(static_cast<int>(result), std::move(dict)));
}

BraveRewardsSavePublisherInfoFunction::
    ~BraveRewardsSavePublisherInfoFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsSavePublisherInfoFunction::Run() {
  std::unique_ptr<brave_rewards::SavePublisherInfo::Params> params(
      brave_rewards::SavePublisherInfo::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not available"));
  }

  auto publisher_info = ledger::mojom::PublisherInfo::New();
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
    const ledger::mojom::Result result) {
  Respond(WithArguments(static_cast<int>(result)));
}

BraveRewardsTipSiteFunction::~BraveRewardsTipSiteFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsTipSiteFunction::Run() {
  std::unique_ptr<brave_rewards::TipSite::Params> params(
      brave_rewards::TipSite::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Sanity check: don't allow tips in private / tor contexts,
  // although the command should not have been enabled in the first place.
  if (!brave::IsRegularProfile(browser_context())) {
    return RespondNow(Error("Cannot tip to site in a private context"));
  }

  // Get web contents for this tab
  content::WebContents* contents =
      WebContentsFromBrowserContext(params->tab_id, browser_context());
  if (!contents) {
    return RespondNow(Error(tabs_constants::kTabNotFoundError,
                            base::NumberToString(params->tab_id)));
  }

  base::Value::Dict params_dict;
  params_dict.Set("publisherKey", params->publisher_key);
  params_dict.Set("entryPoint", params->entry_point);
  params_dict.Set(
      "url", contents ? contents->GetLastCommittedURL().spec() : std::string());
  ::brave_rewards::OpenTipDialog(contents, std::move(params_dict));

  return RespondNow(NoArguments());
}

BraveRewardsTipUserFunction::~BraveRewardsTipUserFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsTipUserFunction::Run() {
  std::unique_ptr<brave_rewards::TipUser::Params> params(
      brave_rewards::TipUser::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Sanity check: don't allow tips in private / tor contexts,
  // although the command should not have been enabled in the first place.
  if (!brave::IsRegularProfile(browser_context())) {
    return RespondNow(Error("Cannot tip user in a private context"));
  }

  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  // If the user clicks the tipping button before having opted into the Rewards,
  // then the Rewards service would not have started the ledger process yet. We
  // need to open the Rewards panel for the user to offer opting in.
  if (!profile->GetPrefs()->GetBoolean(::brave_rewards::prefs::kEnabled)) {
    // Get web contents for this tab
    content::WebContents* contents =
        WebContentsFromBrowserContext(params->tab_id, browser_context());
    if (!contents) {
      return RespondNow(Error(tabs_constants::kTabNotFoundError,
                              base::NumberToString(params->tab_id)));
    }
    auto* coordinator = GetPanelCoordinator(contents);
    if (!coordinator) {
      return RespondNow(Error("Unable to open Rewards panel"));
    }
    coordinator->OpenRewardsPanel();
    return RespondNow(NoArguments());
  }

  AddRef();

  rewards_service->GetPublisherInfo(
      params->publisher_key,
      base::BindOnce(&BraveRewardsTipUserFunction::OnTipUserGetPublisherInfo,
                     this));

  return RespondNow(NoArguments());
}

void BraveRewardsTipUserFunction::OnTipUserGetPublisherInfo(
    const ledger::mojom::Result result,
    ledger::mojom::PublisherInfoPtr info) {
  if (result != ledger::mojom::Result::LEDGER_OK &&
      result != ledger::mojom::Result::NOT_FOUND) {
    Release();
    return;
  }

  if (result == ledger::mojom::Result::LEDGER_OK) {
    ShowTipDialog();
    Release();
    return;
  }

  std::unique_ptr<brave_rewards::TipUser::Params> params(
      brave_rewards::TipUser::Params::Create(args()));

  auto publisher_info = ledger::mojom::PublisherInfo::New();
  publisher_info->id = params->publisher_key;
  publisher_info->name = params->publisher_name;
  publisher_info->url = params->url;
  publisher_info->provider = params->media_type;
  publisher_info->favicon_url = params->fav_icon_url;

  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    Release();
    return;
  }

  rewards_service->SavePublisherInfo(
      0, std::move(publisher_info),
      base::BindOnce(&BraveRewardsTipUserFunction::OnTipUserSavePublisherInfo,
                     this));
}

void BraveRewardsTipUserFunction::OnTipUserSavePublisherInfo(
    const ledger::mojom::Result result) {
  if (result != ledger::mojom::Result::LEDGER_OK) {
    Release();
    return;
  }

  ShowTipDialog();
  Release();
}

void BraveRewardsTipUserFunction::ShowTipDialog() {
  std::unique_ptr<brave_rewards::TipUser::Params> params(
      brave_rewards::TipUser::Params::Create(args()));
  if (!params) {
    Release();
    return;
  }

  // Get web contents for this tab
  content::WebContents* contents =
      WebContentsFromBrowserContext(params->tab_id, browser_context());
  if (!contents) {
    Release();
    return;
  }

  base::Value::Dict media_meta_data_dict;
  media_meta_data_dict.Set("mediaType", params->media_type);
  media_meta_data_dict.Set("publisherKey", params->publisher_key);
  media_meta_data_dict.Set("publisherName", params->publisher_name);
  media_meta_data_dict.Set("publisherScreenName",
                           params->publisher_screen_name);
  media_meta_data_dict.Set("postId", params->post_id);
  media_meta_data_dict.Set("postTimestamp", params->post_timestamp);
  media_meta_data_dict.Set("postText", params->post_text);

  base::Value::Dict params_dict;
  params_dict.Set("publisherKey", params->publisher_key);
  params_dict.Set("url", params->url);
  params_dict.Set("mediaMetaData", std::move(media_meta_data_dict));

  ::brave_rewards::OpenTipDialog(contents, std::move(params_dict));
}

BraveRewardsIncludeInAutoContributionFunction::
    ~BraveRewardsIncludeInAutoContributionFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsIncludeInAutoContributionFunction::Run() {
  std::unique_ptr<brave_rewards::IncludeInAutoContribution::Params> params(
      brave_rewards::IncludeInAutoContribution::Params::Create(args()));
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
  std::unique_ptr<brave_rewards::GetPublisherData::Params> params(
      brave_rewards::GetPublisherData::Params::Create(args()));
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
    ledger::mojom::RewardsParametersPtr parameters) {
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
    data.Set("vbatDeadline", floor(parameters->vbat_deadline.ToDoubleT() *
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
    ledger::mojom::CreateRewardsWalletResult result) {
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
    ledger::mojom::UserType user_type) {
  auto map_user_type = [](ledger::mojom::UserType user_type) -> std::string {
    switch (user_type) {
      case ledger::mojom::UserType::kLegacyUnconnected:
        return "legacy-unconnected";
      case ledger::mojom::UserType::kConnected:
        return "connected";
      case ledger::mojom::UserType::kUnconnected:
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

  std::unique_ptr<brave_rewards::GetBalanceReport::Params> params(
      brave_rewards::GetBalanceReport::Params::Create(args()));

  rewards_service->GetBalanceReport(
      params->month, params->year,
      base::BindOnce(&BraveRewardsGetBalanceReportFunction::OnBalanceReport,
                     this));
  return RespondLater();
}

void BraveRewardsGetBalanceReportFunction::OnBalanceReport(
    const ledger::mojom::Result result,
    ledger::mojom::BalanceReportInfoPtr report) {
  base::Value::Dict data;
  data.Set("ads", report ? report->earning_from_ads : 0.0);
  data.Set("contribute", report ? report->auto_contribute : 0.0);
  data.Set("grant", report ? report->grants : 0.0);
  data.Set("tips", report ? report->one_time_donation : 0.0);
  data.Set("monthly", report ? report->recurring_donation : 0.0);
  Respond(WithArguments(std::move(data)));
}

BraveRewardsFetchPromotionsFunction::~BraveRewardsFetchPromotionsFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsFetchPromotionsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not available"));
  }

  rewards_service->FetchPromotions(base::BindOnce(
      &BraveRewardsFetchPromotionsFunction::OnPromotionsFetched, this));

  return RespondLater();
}

void BraveRewardsFetchPromotionsFunction::OnPromotionsFetched(
    std::vector<ledger::mojom::PromotionPtr> promotions) {
  base::Value::List list;
  for (auto& item : promotions) {
    base::Value::Dict dict;
    dict.Set("promotionId", item->id);
    dict.Set("type", static_cast<int>(item->type));
    dict.Set("status", static_cast<int>(item->status));
    dict.Set("createdAt", static_cast<double>(item->created_at));
    dict.Set("claimableUntil", static_cast<double>(item->claimable_until));
    dict.Set("expiresAt", static_cast<double>(item->expires_at));
    dict.Set("amount", item->approximate_value);
    list.Append(std::move(dict));
  }
  Respond(WithArguments(std::move(list)));
}

BraveRewardsClaimPromotionFunction::~BraveRewardsClaimPromotionFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsClaimPromotionFunction::Run() {
  std::unique_ptr<brave_rewards::ClaimPromotion::Params> params(
      brave_rewards::ClaimPromotion::Params::Create(args()));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not available"));
  }

  rewards_service->ClaimPromotion(
      params->promotion_id,
      base::BindOnce(&BraveRewardsClaimPromotionFunction::OnClaimPromotion,
                     this, params->promotion_id));
  return RespondLater();
}

void BraveRewardsClaimPromotionFunction::OnClaimPromotion(
    const std::string& promotion_id,
    const ledger::mojom::Result result,
    const std::string& captcha_image,
    const std::string& hint,
    const std::string& captcha_id) {
  base::Value::Dict data;
  data.Set("result", static_cast<int>(result));
  data.Set("promotionId", promotion_id);
  data.Set("captchaImage", captcha_image);
  data.Set("captchaId", captcha_id);
  data.Set("hint", hint);
  Respond(WithArguments(std::move(data)));
}

BraveRewardsAttestPromotionFunction::~BraveRewardsAttestPromotionFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsAttestPromotionFunction::Run() {
  std::unique_ptr<brave_rewards::AttestPromotion::Params> params(
      brave_rewards::AttestPromotion::Params::Create(args()));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not available"));
  }

  rewards_service->AttestPromotion(
      params->promotion_id, params->solution,
      base::BindOnce(&BraveRewardsAttestPromotionFunction::OnAttestPromotion,
                     this, params->promotion_id));
  return RespondLater();
}

void BraveRewardsAttestPromotionFunction::OnAttestPromotion(
    const std::string& promotion_id,
    const ledger::mojom::Result result,
    ledger::mojom::PromotionPtr promotion) {
  base::Value::Dict data;
  data.Set("promotionId", promotion_id);

  if (!promotion) {
    Respond(WithArguments(static_cast<int>(result), std::move(data)));
    return;
  }

  data.Set("expiresAt", static_cast<double>(promotion->expires_at));
  data.Set("amount", static_cast<double>(promotion->approximate_value));
  data.Set("type", static_cast<int>(promotion->type));
  Respond(WithArguments(static_cast<int>(result), std::move(data)));
}

BraveRewardsGetPendingContributionsTotalFunction::
    ~BraveRewardsGetPendingContributionsTotalFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetPendingContributionsTotalFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(WithArguments(0.0));
  }

  rewards_service->GetPendingContributionsTotal(base::BindOnce(
      &BraveRewardsGetPendingContributionsTotalFunction::OnGetPendingTotal,
      this));
  return RespondLater();
}

void BraveRewardsGetPendingContributionsTotalFunction::OnGetPendingTotal(
    double amount) {
  Respond(WithArguments(amount));
}

BraveRewardsSaveAdsSettingFunction::~BraveRewardsSaveAdsSettingFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsSaveAdsSettingFunction::Run() {
  std::unique_ptr<brave_rewards::SaveAdsSetting::Params> params(
      brave_rewards::SaveAdsSetting::Params::Create(args()));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);

  if (!rewards_service || !ads_service) {
    return RespondNow(Error("Service is not initialized"));
  }

  if (params->key == "adsEnabled") {
    ads_service->SetEnabled(params->value == "true" &&
                            ads_service->IsSupportedLocale());
  }

  return RespondNow(NoArguments());
}

BraveRewardsSetAutoContributeEnabledFunction::
    ~BraveRewardsSetAutoContributeEnabledFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsSetAutoContributeEnabledFunction::Run() {
  std::unique_ptr<brave_rewards::SetAutoContributeEnabled::Params> params(
      brave_rewards::SetAutoContributeEnabled::Params::Create(args()));
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
  std::unique_ptr<brave_rewards::SaveRecurringTip::Params> params(
      brave_rewards::SaveRecurringTip::Params::Create(args()));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service_) {
    return RespondNow(NoArguments());
  }

  rewards_service_->SaveRecurringTip(
      params->publisher_key, params->new_amount,
      base::BindOnce(&BraveRewardsSaveRecurringTipFunction::OnSaveRecurringTip,
                     this));

  return RespondLater();
}

void BraveRewardsSaveRecurringTipFunction::OnSaveRecurringTip(
    ledger::mojom::Result result) {
  Respond(result == ledger::mojom::Result::LEDGER_OK ? NoArguments()
                                                     : Error("Failed to save"));
}

BraveRewardsRemoveRecurringTipFunction::
    ~BraveRewardsRemoveRecurringTipFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsRemoveRecurringTipFunction::Run() {
  std::unique_ptr<brave_rewards::RemoveRecurringTip::Params> params(
      brave_rewards::RemoveRecurringTip::Params::Create(args()));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ =
      RewardsServiceFactory::GetForProfile(profile);

  if (rewards_service_) {
    rewards_service_->RemoveRecurringTip(params->publisher_key);
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
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  base::Value::Dict result;
  base::Value::List recurringTips;

  if (!list.empty()) {
    for (const auto& item : list) {
      base::Value::Dict tip;
      tip.Set("publisherKey", item->id);
      tip.Set("amount", item->weight);
      recurringTips.Append(std::move(tip));
    }
  }

  result.Set("recurringTips", std::move(recurringTips));
  Respond(WithArguments(std::move(result)));
}

BraveRewardsRefreshPublisherFunction::~BraveRewardsRefreshPublisherFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsRefreshPublisherFunction::Run() {
  std::unique_ptr<brave_rewards::RefreshPublisher::Params> params(
      brave_rewards::RefreshPublisher::Params::Create(args()));

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
    const ledger::mojom::PublisherStatus status,
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

BraveRewardsGetInlineTippingPlatformEnabledFunction::
    ~BraveRewardsGetInlineTippingPlatformEnabledFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetInlineTippingPlatformEnabledFunction::Run() {
  std::unique_ptr<brave_rewards::GetInlineTippingPlatformEnabled::Params>
      params(brave_rewards::GetInlineTippingPlatformEnabled::Params::Create(
          args()));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(WithArguments(false));
  }

  rewards_service->GetInlineTippingPlatformEnabled(
      params->key,
      base::BindOnce(&BraveRewardsGetInlineTippingPlatformEnabledFunction::
                         OnInlineTipSetting,
                     this));
  return RespondLater();
}

void BraveRewardsGetInlineTippingPlatformEnabledFunction::OnInlineTipSetting(
    bool value) {
  Respond(WithArguments(value));
}

BraveRewardsIsAutoContributeSupportedFunction::
    ~BraveRewardsIsAutoContributeSupportedFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsIsAutoContributeSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  return RespondNow(
      WithArguments(rewards_service->IsAutoContributeSupported()));
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
      base::BindOnce(&BraveRewardsFetchBalanceFunction::OnBalance, this));
  return RespondLater();
}

void BraveRewardsFetchBalanceFunction::OnBalance(
    const ledger::mojom::Result result,
    ledger::mojom::BalancePtr balance) {
  base::Value::Dict balance_value;
  if (result == ledger::mojom::Result::LEDGER_OK && balance) {
    balance_value.Set("total", balance->total);

    base::Value::Dict wallets;
    for (auto const& rate : balance->wallets) {
      wallets.Set(rate.first, rate.second);
    }
    balance_value.Set("wallets", std::move(wallets));
  } else {
    balance_value.Set("total", 0.0);
    base::Value::Dict wallets;
    balance_value.Set("wallets", std::move(wallets));
  }

  Respond(WithArguments(std::move(balance_value)));
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
    base::expected<ledger::mojom::ExternalWalletPtr,
                   ledger::mojom::GetExternalWalletError> result) {
  auto wallet = std::move(result).value_or(nullptr);
  if (!wallet) {
    return Respond(NoArguments());
  }

  base::Value::Dict data;
  data.Set("type", wallet->type);
  data.Set("address", wallet->address);
  data.Set("status", static_cast<int>(wallet->status));
  data.Set("userName", wallet->user_name);
  data.Set("accountUrl", wallet->account_url);
  data.Set("loginUrl", wallet->login_url);
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

BraveRewardsGetAdsEnabledFunction::~BraveRewardsGetAdsEnabledFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsGetAdsEnabledFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);

  if (!ads_service) {
    return RespondNow(WithArguments(false));
  }

  const bool enabled = ads_service->IsEnabled();
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
    ads::mojom::StatementInfoPtr statement) {
  if (!statement) {
    Respond(WithArguments(false));
  } else {
    base::Value::Dict dict;
    dict.Set("nextPaymentDate",
             statement->next_payment_date.ToDoubleT() * 1000);
    dict.Set("adsReceivedThisMonth", statement->ads_received_this_month);
    dict.Set("earningsThisMonth", statement->earnings_this_month);
    dict.Set("earningsLastMonth", statement->earnings_last_month);

    Respond(WithArguments(true, std::move(dict)));
  }

  Release();  // Balanced in Run()
}

BraveRewardsGetAdsSupportedFunction::~BraveRewardsGetAdsSupportedFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsGetAdsSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);

  if (!ads_service) {
    return RespondNow(WithArguments(false));
  }

  const bool supported = ads_service->IsSupportedLocale();
  return RespondNow(WithArguments(supported));
}

BraveRewardsGetAdsDataFunction::~BraveRewardsGetAdsDataFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsGetAdsDataFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);

  base::Value::Dict ads_data;
  if (!ads_service) {
    ads_data.Set("adsIsSupported", false);
    ads_data.Set("adsEnabled", false);
    ads_data.Set("adsPerHour", 0);
    ads_data.Set("adsSubdivisionTargeting", std::string());
    ads_data.Set("automaticallyDetectedAdsSubdivisionTargeting", std::string());
    ads_data.Set("shouldAllowAdsSubdivisionTargeting", false);
    ads_data.Set("adsUIEnabled", false);
  } else {
    ads_data.Set("adsIsSupported", ads_service->IsSupportedLocale());
    ads_data.Set("adsEnabled", ads_service->IsEnabled());
    ads_data.Set(
        "adsPerHour",
        static_cast<int>(ads_service->GetMaximumNotificationAdsPerHour()));
    ads_data.Set("adsSubdivisionTargeting",
                 ads_service->GetSubdivisionTargetingCode());
    ads_data.Set("automaticallyDetectedAdsSubdivisionTargeting",
                 ads_service->GetAutoDetectedSubdivisionTargetingCode());
    ads_data.Set("shouldAllowAdsSubdivisionTargeting",
                 ads_service->ShouldAllowSubdivisionTargeting());
    ads_data.Set("adsUIEnabled", true);
  }

  base::Value::List subdivisions;
  const auto subdivision_infos = ads::GetSupportedSubdivisions();
  for (const auto& info : subdivision_infos) {
    base::Value::Dict subdivision;
    subdivision.Set("value", info.first);
    subdivision.Set("name", info.second);
    subdivisions.Append(std::move(subdivision));
  }
  ads_data.Set("subdivisions", std::move(subdivisions));

  return RespondNow(WithArguments(std::move(ads_data)));
}

BraveRewardsIsInitializedFunction::~BraveRewardsIsInitializedFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsIsInitializedFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  return RespondNow(
      WithArguments(rewards_service && rewards_service->IsInitialized()));
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
  auto params(
      brave_rewards::UpdateScheduledCaptchaResult::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

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

  return RespondNow(NoArguments());
}

BraveRewardsEnableAdsFunction::~BraveRewardsEnableAdsFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsEnableAdsFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  if (auto* ads_service = AdsServiceFactory::GetForProfile(profile)) {
    ads_service->SetEnabled(true);
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
    ledger::mojom::AutoContributePropertiesPtr properties) {
  base::Value::Dict prefs;
  prefs.Set("autoContributeEnabled",
            properties ? properties->enabled_contribute : false);
  prefs.Set("autoContributeAmount", properties ? properties->amount : 0.0);

  auto* ads_service = AdsServiceFactory::GetForProfile(
      Profile::FromBrowserContext(browser_context()));

  if (ads_service) {
    prefs.Set("adsEnabled", ads_service->IsEnabled());
    prefs.Set(
        "adsPerHour",
        static_cast<double>(ads_service->GetMaximumNotificationAdsPerHour()));
  } else {
    prefs.Set("adsEnabled", false);
    prefs.Set("adsPerHour", 0.0);
  }

  Respond(WithArguments(std::move(prefs)));
}

BraveRewardsUpdatePrefsFunction::~BraveRewardsUpdatePrefsFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsUpdatePrefsFunction::Run() {
  auto params = brave_rewards::UpdatePrefs::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  auto* ads_service = AdsServiceFactory::GetForProfile(profile);

  if (rewards_service) {
    auto& ac_enabled = params->prefs.auto_contribute_enabled;
    if (ac_enabled)
      rewards_service->SetAutoContributeEnabled(*ac_enabled);

    auto& ac_amount = params->prefs.auto_contribute_amount;
    if (ac_amount)
      rewards_service->SetAutoContributionAmount(*ac_amount);
  }

  if (ads_service) {
    auto& ads_enabled = params->prefs.ads_enabled;
    if (ads_enabled)
      ads_service->SetEnabled(*ads_enabled);

    auto& ads_per_hour = params->prefs.ads_per_hour;
    if (ads_per_hour)
      ads_service->SetMaximumNotificationAdsPerHour(*ads_per_hour);
  }

  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
