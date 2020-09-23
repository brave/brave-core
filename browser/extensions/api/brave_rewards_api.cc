/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_rewards_api.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/brave_rewards/tip_dialog.h"
#include "brave/browser/extensions/api/brave_action_api.h"
#include "brave/common/extensions/api/brave_rewards.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "extensions/common/constants.h"

using brave_ads::AdsService;
using brave_ads::AdsServiceFactory;
using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;

namespace extensions {
namespace api {

BraveRewardsCreateWalletFunction::BraveRewardsCreateWalletFunction()
    : weak_factory_(this) {
}

BraveRewardsCreateWalletFunction::~BraveRewardsCreateWalletFunction() {
}

void BraveRewardsCreateWalletFunction::OnCreateWallet(
    const ledger::type::Result result) {
}

ExtensionFunction::ResponseAction BraveRewardsCreateWalletFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->CreateWallet(
        base::Bind(
            &BraveRewardsCreateWalletFunction::OnCreateWallet,
            weak_factory_.GetWeakPtr()));
  }
  return RespondNow(NoArguments());
}

BraveRewardsOpenBrowserActionUIFunction::
~BraveRewardsOpenBrowserActionUIFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsOpenBrowserActionUIFunction::Run() {
  std::unique_ptr<brave_rewards::OpenBrowserActionUI::Params> params(
      brave_rewards::OpenBrowserActionUI::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  std::string error;
  if (!BraveActionAPI::ShowActionUI(this,
      brave_rewards_extension_id,
      std::move(params->window_id),
      std::move(params->relative_path), &error)) {
    return RespondNow(Error(error));
  }
  return RespondNow(NoArguments());
}

BraveRewardsTipSiteFunction::~BraveRewardsTipSiteFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsTipSiteFunction::Run() {
  std::unique_ptr<brave_rewards::TipSite::Params> params(
      brave_rewards::TipSite::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Sanity check: don't allow tips in private / tor contexts,
  // although the command should not have been enabled in the first place.
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (profile->IsOffTheRecord()) {
    return RespondNow(Error("Cannot tip to site in a private context"));
  }

  // Get web contents for this tab
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
        params->tab_id,
        profile,
        false,
        nullptr,
        nullptr,
        &contents,
        nullptr)) {
    return RespondNow(Error(tabs_constants::kTabNotFoundError,
                            base::NumberToString(params->tab_id)));
  }

  auto params_dict = std::make_unique<base::DictionaryValue>();
  params_dict->SetString("publisherKey", params->publisher_key);
  params_dict->SetBoolean("monthly", params->monthly);
  params_dict->SetString(
      "url", contents ? contents->GetLastCommittedURL().spec() : std::string());
  ::brave_rewards::OpenTipDialog(contents, std::move(params_dict));

  return RespondNow(NoArguments());
}

BraveRewardsTipTwitterUserFunction::BraveRewardsTipTwitterUserFunction()
    : weak_factory_(this) {
}

BraveRewardsTipTwitterUserFunction::~BraveRewardsTipTwitterUserFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsTipTwitterUserFunction::Run() {
  std::unique_ptr<brave_rewards::TipTwitterUser::Params> params(
      brave_rewards::TipTwitterUser::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Sanity check: don't allow tips in private / tor contexts,
  // although the command should not have been enabled in the first place.
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (profile->IsOffTheRecord()) {
    return RespondNow(
        Error("Cannot tip Twitter user in a private context"));
  }

  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    AddRef();
    std::map<std::string, std::string> args;
    args["user_id"] = params->media_meta_data.user_id;
    args["twitter_name"] = params->media_meta_data.twitter_name;
    args["screen_name"] = params->media_meta_data.screen_name;
    rewards_service->SaveInlineMediaInfo(
        params->media_meta_data.media_type,
        args,
        base::Bind(&BraveRewardsTipTwitterUserFunction::
                   OnTwitterPublisherInfoSaved,
                   weak_factory_.GetWeakPtr()));
  }

  return RespondNow(NoArguments());
}

BraveRewardsTipRedditUserFunction::BraveRewardsTipRedditUserFunction()
    : weak_factory_(this) {
}

BraveRewardsTipRedditUserFunction::~BraveRewardsTipRedditUserFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsTipRedditUserFunction::Run() {
  std::unique_ptr<brave_rewards::TipRedditUser::Params> params(
      brave_rewards::TipRedditUser::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (profile->IsOffTheRecord()) {
    return RespondNow(
        Error("Cannot tip Reddit user in a private context"));
  }

  RewardsService* rewards_service = RewardsServiceFactory::GetForProfile(
      profile);

  if (rewards_service) {
    AddRef();
    std::map<std::string, std::string> args;
    args["user_name"] = params->media_meta_data.user_name;
    args["post_text"] = params->media_meta_data.post_text;
    args["post_rel_date"] = params->media_meta_data.post_rel_date;
    rewards_service->SaveInlineMediaInfo(
        params->media_meta_data.media_type,
        args,
        base::Bind(
            &BraveRewardsTipRedditUserFunction::OnRedditPublisherInfoSaved,
            weak_factory_.GetWeakPtr()));
  }

  return RespondNow(NoArguments());
}

void BraveRewardsTipRedditUserFunction::OnRedditPublisherInfoSaved(
    ledger::type::PublisherInfoPtr publisher) {
  std::unique_ptr<brave_rewards::TipRedditUser::Params> params(
      brave_rewards::TipRedditUser::Params::Create(*args_));

  if (!publisher) {
    Release();
    return;
  }

  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
          params->tab_id,
          Profile::FromBrowserContext(browser_context()),
          false,
          nullptr,
          nullptr,
          &contents,
          nullptr)) {
      return;
  }

  std::unique_ptr<base::DictionaryValue> params_dict =
      std::make_unique<base::DictionaryValue>();
  params_dict->SetStringKey("publisherKey", publisher->id);
  params_dict->SetStringKey("url", publisher->url);

  base::Value media_meta_data_dict(base::Value::Type::DICTIONARY);
  media_meta_data_dict.SetStringKey("name", publisher->name);
  media_meta_data_dict.SetStringKey(
      "userName", params->media_meta_data.user_name);
  media_meta_data_dict.SetStringKey(
      "postText", params->media_meta_data.post_text);
  media_meta_data_dict.SetStringKey(
      "postRelDate", params->media_meta_data.post_rel_date);
  media_meta_data_dict.SetStringKey(
      "mediaType", params->media_meta_data.media_type);
  params_dict->SetPath(
      "mediaMetaData", std::move(media_meta_data_dict));

  ::brave_rewards::OpenTipDialog(
      contents, std::move(params_dict));

  Release();
}

void BraveRewardsTipTwitterUserFunction::OnTwitterPublisherInfoSaved(
    ledger::type::PublisherInfoPtr publisher) {
  std::unique_ptr<brave_rewards::TipTwitterUser::Params> params(
      brave_rewards::TipTwitterUser::Params::Create(*args_));

  if (!publisher) {
    // TODO(nejczdovc): what should we do in this case?
    Release();
    return;
  }

  // Get web contents for this tab
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
        params->tab_id,
        Profile::FromBrowserContext(browser_context()),
        false,
        nullptr,
        nullptr,
        &contents,
        nullptr)) {
    return;
  }

  auto params_dict = std::make_unique<base::DictionaryValue>();
  params_dict->SetString("publisherKey", publisher->id);
  params_dict->SetString("url", publisher->url);

  base::Value media_meta_data_dict(base::Value::Type::DICTIONARY);
  media_meta_data_dict.SetStringKey("twitter_name", publisher->name);
  media_meta_data_dict.SetStringKey("mediaType",
                                  params->media_meta_data.media_type);
  media_meta_data_dict.SetStringKey("screenName",
                                  params->media_meta_data.screen_name);
  media_meta_data_dict.SetStringKey("userId", params->media_meta_data.user_id);
  media_meta_data_dict.SetStringKey("tweetId",
                                  params->media_meta_data.tweet_id);
  media_meta_data_dict.SetDoubleKey("tweetTimestamp",
                                  params->media_meta_data.tweet_timestamp);
  media_meta_data_dict.SetStringKey("tweetText",
                                  params->media_meta_data.tweet_text);
  params_dict->SetPath("mediaMetaData", std::move(media_meta_data_dict));

  ::brave_rewards::OpenTipDialog(contents, std::move(params_dict));

  Release();
}
///////////////////////////////////////////////////
BraveRewardsTipGitHubUserFunction::BraveRewardsTipGitHubUserFunction()
    : weak_factory_(this) {
}

BraveRewardsTipGitHubUserFunction::~BraveRewardsTipGitHubUserFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsTipGitHubUserFunction::Run() {
  std::unique_ptr<brave_rewards::TipGitHubUser::Params> params(
      brave_rewards::TipGitHubUser::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  // Sanity check: don't allow tips in private / tor contexts,
  // although the command should not have been enabled in the first place.
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (profile->IsOffTheRecord()) {
    return RespondNow(
        Error("Cannot tip Twitter user in a private context"));
  }

  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    AddRef();
    std::map<std::string, std::string> args;
    args["user_name"] = params->media_meta_data.user_name;
    if (args["user_name"].empty()) {
      LOG(ERROR) << "Cannot tip user without username";
    } else {
      rewards_service->SaveInlineMediaInfo(
          params->media_meta_data.media_type,
          args,
          base::Bind(&BraveRewardsTipGitHubUserFunction::
                     OnGitHubPublisherInfoSaved,
                     weak_factory_.GetWeakPtr()));
    }
  }
  return RespondNow(NoArguments());
}


void BraveRewardsTipGitHubUserFunction::OnGitHubPublisherInfoSaved(
    ledger::type::PublisherInfoPtr publisher) {
  std::unique_ptr<brave_rewards::TipGitHubUser::Params> params(
      brave_rewards::TipGitHubUser::Params::Create(*args_));

  if (!publisher) {
    // TODO(nejczdovc): what should we do in this case?
    Release();
    return;
  }

  // Get web contents for this tab
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
        params->tab_id,
        Profile::FromBrowserContext(browser_context()),
        false,
        nullptr,
        nullptr,
        &contents,
        nullptr)) {
    return;
  }

  auto params_dict = std::make_unique<base::DictionaryValue>();
  params_dict->SetString("publisherKey", publisher->id);
  params_dict->SetString("url", publisher->url);

  base::Value media_meta_data_dict(base::Value::Type::DICTIONARY);
  media_meta_data_dict.SetStringKey("mediaType",
                                  params->media_meta_data.media_type);
  media_meta_data_dict.SetStringKey("name", publisher->name);
  media_meta_data_dict.SetStringKey("userName",
                                  params->media_meta_data.user_name);
  params_dict->SetPath("mediaMetaData",
                       std::move(media_meta_data_dict));

  ::brave_rewards::OpenTipDialog(contents, std::move(params_dict));

  Release();
}
//////////////////

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
    rewards_service->SetPublisherExclude(
      params->publisher_key,
      params->exclude);
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

BraveRewardsGetRewardsParametersFunction::
~BraveRewardsGetRewardsParametersFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetRewardsParametersFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    auto data = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
    return RespondNow(OneArgument(std::move(data)));
  }

  rewards_service->GetRewardsParameters(base::BindOnce(
      &BraveRewardsGetRewardsParametersFunction::OnGet,
      this));
  return RespondLater();
}

void BraveRewardsGetRewardsParametersFunction::OnGet(
    ledger::type::RewardsParametersPtr parameters) {
  auto data = std::make_unique<base::DictionaryValue>();

  if (!parameters) {
    return Respond(OneArgument(std::move(data)));
  }

  data->SetDouble("rate", parameters->rate);
  auto monthly_choices = std::make_unique<base::ListValue>();
  for (auto const& item : parameters->monthly_tip_choices) {
    monthly_choices->Append(base::Value(item));
  }
  data->SetList("monthlyTipChoices", std::move(monthly_choices));

  Respond(OneArgument(std::move(data)));
}

BraveRewardsGetBalanceReportFunction::
~BraveRewardsGetBalanceReportFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsGetBalanceReportFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    auto data = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
    return RespondNow(OneArgument(std::move(data)));
  }

  std::unique_ptr<brave_rewards::GetBalanceReport::Params> params(
      brave_rewards::GetBalanceReport::Params::Create(*args_));

  rewards_service->GetBalanceReport(
      params->month,
      params->year,
      base::BindOnce(
          &BraveRewardsGetBalanceReportFunction::OnBalanceReport,
          this));
  return RespondLater();
}

void BraveRewardsGetBalanceReportFunction::OnBalanceReport(
    const ledger::type::Result result,
    ledger::type::BalanceReportInfoPtr report) {
  auto data = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
  if (!report) {
    Respond(OneArgument(std::move(data)));
    return;
  }
  data->SetDoubleKey("ads", report->earning_from_ads);
  data->SetDoubleKey("contribute", report->auto_contribute);
  data->SetDoubleKey("grant", report->grants);
  data->SetDoubleKey("tips", report->one_time_donation);
  data->SetDoubleKey("monthly", report->recurring_donation);
  Respond(OneArgument(std::move(data)));
}

BraveRewardsFetchPromotionsFunction::
~BraveRewardsFetchPromotionsFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsFetchPromotionsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service) {
    rewards_service->FetchPromotions();
  }
  return RespondNow(NoArguments());
}

BraveRewardsClaimPromotionFunction::
~BraveRewardsClaimPromotionFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsClaimPromotionFunction::Run() {
  std::unique_ptr<brave_rewards::ClaimPromotion::Params> params(
      brave_rewards::ClaimPromotion::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    auto data = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
    data->SetIntKey("result", 1);
    return RespondNow(OneArgument(std::move(data)));
  }

  rewards_service->ClaimPromotion(
      params->promotion_id,
      base::BindOnce(
          &BraveRewardsClaimPromotionFunction::OnClaimPromotion,
          this,
          params->promotion_id));
  return RespondLater();
}

void BraveRewardsClaimPromotionFunction::OnClaimPromotion(
    const std::string& promotion_id,
    const ledger::type::Result result,
    const std::string& captcha_image,
    const std::string& hint,
    const std::string& captcha_id) {

  auto data = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
  data->SetIntKey("result", static_cast<int>(result));
  data->SetStringKey("promotionId", promotion_id);
  data->SetStringKey("captchaImage", captcha_image);
  data->SetStringKey("captchaId", captcha_id);
  data->SetStringKey("hint", hint);
  Respond(OneArgument(std::move(data)));
}

BraveRewardsAttestPromotionFunction::
~BraveRewardsAttestPromotionFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsAttestPromotionFunction::Run() {
  std::unique_ptr<brave_rewards::AttestPromotion::Params> params(
      brave_rewards::AttestPromotion::Params::Create(*args_));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(OneArgument(std::make_unique<base::Value>(1)));
  }

  rewards_service->AttestPromotion(params->promotion_id, params->solution,
      base::BindOnce(
        &BraveRewardsAttestPromotionFunction::OnAttestPromotion,
        this,
        params->promotion_id));
  return RespondLater();
}

void BraveRewardsAttestPromotionFunction::OnAttestPromotion(
    const std::string& promotion_id,
    const ledger::type::Result result,
    ledger::type::PromotionPtr promotion) {
  auto data = std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
  data->SetStringKey("promotionId", promotion_id);

  if (!promotion) {
    Respond(
        TwoArguments(
            std::make_unique<base::Value>(static_cast<int>(result)),
            std::move(data)));
    return;
  }

  data->SetIntKey("expiresAt", promotion->expires_at);
  data->SetDoubleKey("amount", promotion->approximate_value);
  data->SetIntKey("type", static_cast<int>(promotion->type));
  Respond(TwoArguments(
      std::make_unique<base::Value>(static_cast<int>(result)),
      std::move(data)));
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
      const auto is_enabled =
          params->value == "true" && ads_service_->IsSupportedLocale();
      ads_service_->SetEnabled(is_enabled);
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

  rewards_service->GetAutoContributeEnabled(base::BindOnce(
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

BraveRewardsSaveRecurringTipFunction::
~BraveRewardsSaveRecurringTipFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsSaveRecurringTipFunction::Run() {
  std::unique_ptr<brave_rewards::SaveRecurringTip::Params> params(
    brave_rewards::SaveRecurringTip::Params::Create(*args_));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ =
    RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service_) {
    return RespondNow(NoArguments());
  }

  rewards_service_->SaveRecurringTip(
      params->publisher_key,
      params->new_amount,
      base::Bind(
          &BraveRewardsSaveRecurringTipFunction::OnSaveRecurringTip,
          this));

  return RespondLater();
}

void BraveRewardsSaveRecurringTipFunction::OnSaveRecurringTip(bool success) {
  if (!success) {
    Respond(Error("Failed to save"));
    return;
  }
  Respond(NoArguments());
}

BraveRewardsRemoveRecurringTipFunction::
~BraveRewardsRemoveRecurringTipFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsRemoveRecurringTipFunction::Run() {
  std::unique_ptr<brave_rewards::RemoveRecurringTip::Params> params(
    brave_rewards::RemoveRecurringTip::Params::Create(*args_));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service_ =
    RewardsServiceFactory::GetForProfile(profile);

  if (rewards_service_) {
    rewards_service_->RemoveRecurringTip(params->publisher_key);
  }

  return RespondNow(NoArguments());
}

BraveRewardsGetRecurringTipsFunction::
~BraveRewardsGetRecurringTipsFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetRecurringTipsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->GetRecurringTips(base::Bind(
        &BraveRewardsGetRecurringTipsFunction::OnGetRecurringTips,
        this));
  return RespondLater();
}

void BraveRewardsGetRecurringTipsFunction::OnGetRecurringTips(
    ledger::type::PublisherInfoList list) {
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());
  auto recurringTips = std::make_unique<base::ListValue>();

  if (!list.empty()) {
    for (const auto& item : list) {
      auto tip = std::make_unique<base::DictionaryValue>();
      tip->SetString("publisherKey", item->id);
      tip->SetDouble("amount", item->weight);
      recurringTips->Append(std::move(tip));
    }
  }

  result->SetList("recurringTips", std::move(recurringTips));
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
    ledger::type::PublisherBannerPtr banner) {
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());

  if (banner) {
    result->SetString("publisherKey", banner->publisher_key);
    result->SetString("title", banner->title);
    result->SetString("name", banner->name);
    result->SetString("description", banner->description);
    result->SetString("background", banner->background);
    result->SetString("logo", banner->logo);
    result->SetString("provider", banner->provider);
    result->SetInteger("verified", static_cast<int>(banner->status));

    auto amounts = std::make_unique<base::ListValue>();
    for (auto const& value : banner->amounts) {
      amounts->AppendInteger(value);
    }
    result->SetList("amounts", std::move(amounts));

    auto links = std::make_unique<base::DictionaryValue>();
    for (auto const& item : banner->links) {
      links->SetString(item.first, item.second);
    }
    result->SetDictionary("links", std::move(links));
  }

  Respond(OneArgument(std::move(result)));
}

BraveRewardsRefreshPublisherFunction::~BraveRewardsRefreshPublisherFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsRefreshPublisherFunction::Run() {
  std::unique_ptr<brave_rewards::RefreshPublisher::Params> params(
      brave_rewards::RefreshPublisher::Params::Create(*args_));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(TwoArguments(
        std::make_unique<base::Value>(false),
        std::make_unique<base::Value>(std::string())));
  }
  rewards_service->RefreshPublisher(
      params->publisher_key,
      base::BindOnce(
        &BraveRewardsRefreshPublisherFunction::OnRefreshPublisher,
        this));
  return RespondLater();
}

void BraveRewardsRefreshPublisherFunction::OnRefreshPublisher(
    const ledger::type::PublisherStatus status,
    const std::string& publisher_key) {
  Respond(TwoArguments(
      std::make_unique<base::Value>(static_cast<int>(status)),
      std::make_unique<base::Value>(publisher_key)));
}

BraveRewardsGetAllNotificationsFunction::
~BraveRewardsGetAllNotificationsFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetAllNotificationsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  auto list = std::make_unique<base::ListValue>();

  if (!rewards_service) {
    return RespondNow(OneArgument(std::move(list)));
  }

  auto notifications = rewards_service->GetAllNotifications();

  for (auto const& notification : notifications) {
    auto item = std::make_unique<base::DictionaryValue>();
    item->SetString("id", notification.second.id_);
    item->SetInteger("type", notification.second.type_);
    item->SetInteger("timestamp", notification.second.timestamp_);

    auto args = std::make_unique<base::ListValue>();
    for (auto const& arg : notification.second.args_) {
      args->AppendString(arg);
    }

    item->SetList("args", std::move(args));
    list->Append(std::move(item));
  }

  return RespondNow(OneArgument(std::move(list)));
}

BraveRewardsGetInlineTippingPlatformEnabledFunction::
~BraveRewardsGetInlineTippingPlatformEnabledFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetInlineTippingPlatformEnabledFunction::Run() {
  std::unique_ptr<brave_rewards::GetInlineTippingPlatformEnabled::Params>
      params(brave_rewards::GetInlineTippingPlatformEnabled::Params::Create(
          *args_));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(OneArgument(std::make_unique<base::Value>(false)));
  }

  rewards_service->GetInlineTippingPlatformEnabled(
      params->key,
      base::BindOnce(
          &BraveRewardsGetInlineTippingPlatformEnabledFunction::
          OnInlineTipSetting,
          this));
  return RespondLater();
}

void BraveRewardsGetInlineTippingPlatformEnabledFunction::OnInlineTipSetting(
    bool value) {
  Respond(OneArgument(std::make_unique<base::Value>(value)));
}

BraveRewardsFetchBalanceFunction::
~BraveRewardsFetchBalanceFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsFetchBalanceFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    auto balance_value = std::make_unique<base::DictionaryValue>();
    return RespondNow(OneArgument(std::move(balance_value)));
  }

  rewards_service->FetchBalance(
      base::BindOnce(
          &BraveRewardsFetchBalanceFunction::OnBalance,
          this));
  return RespondLater();
}

void BraveRewardsFetchBalanceFunction::OnBalance(
    const ledger::type::Result result,
    ledger::type::BalancePtr balance) {
  auto balance_value = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);
  if (result == ledger::type::Result::LEDGER_OK && balance) {
    balance_value->SetDoubleKey("total", balance->total);

    base::Value wallets(base::Value::Type::DICTIONARY);
    for (auto const& rate : balance->wallets) {
      wallets.SetDoubleKey(rate.first, rate.second);
    }
    balance_value->SetKey("wallets", std::move(wallets));
  } else {
    balance_value->SetDoubleKey("total", 0.0);
    base::Value wallets(base::Value::Type::DICTIONARY);
    balance_value->SetKey("wallets", std::move(wallets));
  }

  Respond(OneArgument(std::move(balance_value)));
}

BraveRewardsGetExternalWalletFunction::
~BraveRewardsGetExternalWalletFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetExternalWalletFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
  auto data = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);
    return RespondNow(OneArgument(std::move(data)));
  }

  std::unique_ptr<brave_rewards::GetExternalWallet::Params> params(
    brave_rewards::GetExternalWallet::Params::Create(*args_));

  rewards_service->GetExternalWallet(
      params->type,
      base::BindOnce(
          &BraveRewardsGetExternalWalletFunction::OnExternalWalet,
          this));
  return RespondLater();
}

void BraveRewardsGetExternalWalletFunction::OnExternalWalet(
    const ledger::type::Result result,
    ledger::type::ExternalWalletPtr wallet) {
  auto data = std::make_unique<base::Value>(
      base::Value::Type::DICTIONARY);

  if (wallet) {
    data->SetStringKey("token", wallet->token);
    data->SetStringKey("address", wallet->address);
    data->SetIntKey("status", static_cast<int>(wallet->status));
    data->SetStringKey("type", wallet->type);
    data->SetStringKey("verifyUrl", wallet->verify_url);
    data->SetStringKey("addUrl", wallet->add_url);
    data->SetStringKey("withdrawUrl", wallet->withdraw_url);
    data->SetStringKey("userName", wallet->user_name);
    data->SetStringKey("accountUrl", wallet->account_url);
    data->SetStringKey("loginUrl", wallet->login_url);
  }

  Respond(TwoArguments(
        std::make_unique<base::Value>(static_cast<int>(result)),
        std::move(data)));
}

BraveRewardsDisconnectWalletFunction::
~BraveRewardsDisconnectWalletFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsDisconnectWalletFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(NoArguments());
  }

  std::unique_ptr<brave_rewards::DisconnectWallet::Params> params(
    brave_rewards::DisconnectWallet::Params::Create(*args_));

  rewards_service->DisconnectWallet(params->type);
  return RespondNow(NoArguments());
}

BraveRewardsOnlyAnonWalletFunction::
~BraveRewardsOnlyAnonWalletFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsOnlyAnonWalletFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(OneArgument(std::make_unique<base::Value>(false)));
  }

  const auto only = rewards_service->OnlyAnonWallet();
  return RespondNow(OneArgument(std::make_unique<base::Value>(only)));
}

BraveRewardsGetAdsEnabledFunction::
~BraveRewardsGetAdsEnabledFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetAdsEnabledFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service_ =
      AdsServiceFactory::GetForProfile(profile);

  if (!ads_service_) {
    return RespondNow(Error("Ads service is not initialized"));
  }

  const bool enabled = ads_service_->IsEnabled();
  return RespondNow(
      OneArgument(std::make_unique<base::Value>(enabled)));
}

BraveRewardsGetAdsEstimatedEarningsFunction::
~BraveRewardsGetAdsEstimatedEarningsFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetAdsEstimatedEarningsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service_ =
      AdsServiceFactory::GetForProfile(profile);

  if (!ads_service_) {
    return RespondNow(Error("Ads service is not initialized"));
  }

  ads_service_->GetTransactionHistory(base::Bind(
      &BraveRewardsGetAdsEstimatedEarningsFunction::OnAdsEstimatedEarnings,
      this));
  return RespondLater();
}

void BraveRewardsGetAdsEstimatedEarningsFunction::OnAdsEstimatedEarnings(
    const bool success,
    const double estimated_pending_rewards,
    const uint64_t next_payment_date_in_seconds,
    const uint64_t ad_notifications_received_this_month) {
  Respond(OneArgument(
      std::make_unique<base::Value>(estimated_pending_rewards)));
}

BraveRewardsGetWalletExistsFunction::
~BraveRewardsGetWalletExistsFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetWalletExistsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->IsWalletCreated(base::Bind(
        &BraveRewardsGetWalletExistsFunction::OnGetWalletExists,
        this));
  return RespondLater();
}

void BraveRewardsGetWalletExistsFunction::OnGetWalletExists(
    const bool exists) {
  Respond(OneArgument(std::make_unique<base::Value>(exists)));
}

BraveRewardsGetAdsSupportedFunction::
~BraveRewardsGetAdsSupportedFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetAdsSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service_ =
      AdsServiceFactory::GetForProfile(profile);

  if (!ads_service_) {
    return RespondNow(Error("Ads service is not initialized"));
  }

  const bool supported = ads_service_->IsSupportedLocale();
  return RespondNow(
      OneArgument(std::make_unique<base::Value>(supported)));
}

BraveRewardsGetAnonWalletStatusFunction::
~BraveRewardsGetAnonWalletStatusFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetAnonWalletStatusFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  rewards_service->GetAnonWalletStatus(base::Bind(
        &BraveRewardsGetAnonWalletStatusFunction::OnGetAnonWalletStatus,
        this));
  return RespondLater();
}

void BraveRewardsGetAnonWalletStatusFunction::OnGetAnonWalletStatus(
    const ledger::type::Result result) {
  Respond(OneArgument(std::make_unique<base::Value>(static_cast<int>(result))));
}

BraveRewardsIsInitializedFunction::
~BraveRewardsIsInitializedFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsIsInitializedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  const bool initialized = rewards_service->IsInitialized();
  return RespondNow(
      OneArgument(std::make_unique<base::Value>(initialized)));
}

}  // namespace api
}  // namespace extensions
