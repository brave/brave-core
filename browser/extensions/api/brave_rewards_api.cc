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
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_rewards/tip_dialog.h"
#include "brave/browser/extensions/api/brave_action_api.h"
#include "brave/browser/extensions/brave_component_loader.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/extensions/api/brave_rewards.h"
#include "brave/components/brave_adaptive_captcha/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
#include "brave/browser/brave_adaptive_captcha/brave_adaptive_captcha_service_factory.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#endif

using brave_ads::AdsService;
using brave_ads::AdsServiceFactory;
using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;

namespace {

const char kShouldAllowAdsSubdivisionTargeting[] =
    "shouldAllowAdsSubdivisionTargeting";
const char kAdsSubdivisionTargeting[] = "adsSubdivisionTargeting";
const char kAutoDetectedAdsSubdivisionTargeting[] =
    "automaticallyDetectedAdsSubdivisionTargeting";

}  // namespace

namespace extensions {
namespace api {

BraveRewardsGetLocaleFunction::~BraveRewardsGetLocaleFunction() {}

ExtensionFunction::ResponseAction BraveRewardsGetLocaleFunction::Run() {
  std::string locale = brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  return RespondNow(OneArgument(base::Value(std::move(locale))));
}

BraveRewardsOpenBrowserActionUIFunction::
~BraveRewardsOpenBrowserActionUIFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsOpenBrowserActionUIFunction::Run() {
  std::unique_ptr<brave_rewards::OpenBrowserActionUI::Params> params(
      brave_rewards::OpenBrowserActionUI::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  auto* profile = Profile::FromBrowserContext(browser_context());

  // Start the rewards ledger process if it is not already started
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service)
    return RespondNow(Error("Rewards service is not initialized"));

  rewards_service->StartProcess(base::DoNothing());

  // Load the rewards extension if it is not already loaded
  auto* extension_service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  if (!extension_service)
    return RespondNow(Error("Extension service is not initialized"));

  static_cast<BraveComponentLoader*>(extension_service->component_loader())
      ->AddRewardsExtension();

  std::string error;
  if (!BraveActionAPI::ShowActionUI(this,
      brave_rewards_extension_id,
      std::move(params->window_id),
      std::move(params->relative_path), &error)) {
    return RespondNow(Error(error));
  }
  return RespondNow(NoArguments());
}

BraveRewardsUpdateMediaDurationFunction::
    ~BraveRewardsUpdateMediaDurationFunction() {}

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

  rewards_service->UpdateMediaDuration(
      params->window_id,
      params->publisher_key,
      params->duration,
      params->first_visit);

  return RespondNow(NoArguments());
}

BraveRewardsGetPublisherInfoFunction::
~BraveRewardsGetPublisherInfoFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetPublisherInfoFunction::Run() {
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
    const ledger::type::Result result,
    ledger::type::PublisherInfoPtr info) {
  if (!info) {
    Respond(OneArgument(base::Value(static_cast<int>(result))));
    return;
  }

  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("publisherKey", info->id);
  dict.SetStringKey("name", info->name);
  dict.SetIntKey("percentage", info->percent);
  dict.SetIntKey("status", static_cast<int>(info->status));
  dict.SetBoolKey("excluded",
                  info->excluded == ledger::type::PublisherExclude::EXCLUDED);
  dict.SetStringKey("url", info->url);
  dict.SetStringKey("provider", info->provider);
  dict.SetStringKey("favIconUrl", info->favicon_url);

  Respond(TwoArguments(base::Value(static_cast<int>(result)), std::move(dict)));
}

BraveRewardsGetPublisherPanelInfoFunction::
    ~BraveRewardsGetPublisherPanelInfoFunction() {}

ExtensionFunction::ResponseAction
BraveRewardsGetPublisherPanelInfoFunction::Run() {
  std::unique_ptr<brave_rewards::GetPublisherPanelInfo::Params> params(
      brave_rewards::GetPublisherPanelInfo::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(NoArguments());
  }

  rewards_service->GetPublisherPanelInfo(
      params->publisher_key,
      base::BindOnce(
          &BraveRewardsGetPublisherPanelInfoFunction::OnGetPublisherPanelInfo,
          this));

  return RespondLater();
}

void BraveRewardsGetPublisherPanelInfoFunction::OnGetPublisherPanelInfo(
    const ledger::type::Result result,
    ledger::type::PublisherInfoPtr info) {
  if (!info) {
    Respond(OneArgument(base::Value(static_cast<int>(result))));
    return;
  }

  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("publisherKey", info->id);
  dict.SetStringKey("name", info->name);
  dict.SetIntKey("percentage", info->percent);
  dict.SetIntKey("status", static_cast<int>(info->status));
  dict.SetBoolKey("excluded",
                  info->excluded == ledger::type::PublisherExclude::EXCLUDED);
  dict.SetStringKey("url", info->url);
  dict.SetStringKey("provider", info->provider);
  dict.SetStringKey("favIconUrl", info->favicon_url);

  Respond(TwoArguments(base::Value(static_cast<int>(result)), std::move(dict)));
}

BraveRewardsSavePublisherInfoFunction::
    ~BraveRewardsSavePublisherInfoFunction() {}

ExtensionFunction::ResponseAction
BraveRewardsSavePublisherInfoFunction::Run() {
  std::unique_ptr<brave_rewards::SavePublisherInfo::Params> params(
      brave_rewards::SavePublisherInfo::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (!rewards_service) {
    return RespondNow(NoArguments());
  }

  auto publisher_info = ledger::type::PublisherInfo::New();
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
    const ledger::type::Result result) {
  Respond(OneArgument(base::Value(static_cast<int>(result))));
}

BraveRewardsTipSiteFunction::~BraveRewardsTipSiteFunction() {
}

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
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
        params->tab_id,
        Profile::FromBrowserContext(browser_context()),
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
  params_dict->SetString("entryPoint", params->entry_point);
  params_dict->SetString(
      "url", contents ? contents->GetLastCommittedURL().spec() : std::string());
  ::brave_rewards::OpenTipDialog(contents, std::move(params_dict));

  return RespondNow(NoArguments());
}

BraveRewardsTipUserFunction::~BraveRewardsTipUserFunction() {
}

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

  extensions::ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  if (!extension_service) {
    return RespondNow(Error("Extension service is not initialized"));
  }

  AddRef();

  extensions::ComponentLoader* component_loader =
      extension_service->component_loader();
  static_cast<extensions::BraveComponentLoader*>(component_loader)
      ->AddRewardsExtension();

  rewards_service->StartProcess(
      base::BindOnce(&BraveRewardsTipUserFunction::OnProcessStarted, this,
                     params->publisher_key));

  return RespondNow(NoArguments());
}

void BraveRewardsTipUserFunction::OnProcessStarted(
    const std::string& publisher_key) {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    Release();
    return;
  }
  rewards_service->GetPublisherInfo(
      publisher_key,
      base::BindOnce(&BraveRewardsTipUserFunction::OnTipUserGetPublisherInfo,
                     this));
}

void BraveRewardsTipUserFunction::OnTipUserGetPublisherInfo(
    const ledger::type::Result result,
    ledger::type::PublisherInfoPtr info) {
  if (result != ledger::type::Result::LEDGER_OK &&
      result != ledger::type::Result::NOT_FOUND) {
    Release();
    return;
  }

  if (result == ledger::type::Result::LEDGER_OK) {
    ShowTipDialog();
    Release();
    return;
  }

  std::unique_ptr<brave_rewards::TipUser::Params> params(
      brave_rewards::TipUser::Params::Create(args()));

  auto publisher_info = ledger::type::PublisherInfo::New();
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
    const ledger::type::Result result) {
  if (result != ledger::type::Result::LEDGER_OK) {
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
  content::WebContents* contents = nullptr;
  if (!ExtensionTabUtil::GetTabById(
        params->tab_id,
        Profile::FromBrowserContext(browser_context()),
        false,
        nullptr,
        nullptr,
        &contents,
        nullptr)) {
    Release();
    return;
  }

  base::Value media_meta_data_dict(base::Value::Type::DICTIONARY);
  media_meta_data_dict.SetStringKey("mediaType", params->media_type);
  media_meta_data_dict.SetStringKey("publisherKey", params->publisher_key);
  media_meta_data_dict.SetStringKey("publisherName", params->publisher_name);
  media_meta_data_dict.SetStringKey(
      "publisherScreenName",
      params->publisher_screen_name);
  media_meta_data_dict.SetStringKey("postId", params->post_id);
  media_meta_data_dict.SetStringKey("postTimestamp", params->post_timestamp);
  media_meta_data_dict.SetStringKey("postText", params->post_text);

  auto params_dict = std::make_unique<base::DictionaryValue>();
  params_dict->SetString("publisherKey", params->publisher_key);
  params_dict->SetString("url", params->url);
  params_dict->SetPath("mediaMetaData", std::move(media_meta_data_dict));

  ::brave_rewards::OpenTipDialog(contents, std::move(params_dict));
}

BraveRewardsGetPublisherDataFunction::~BraveRewardsGetPublisherDataFunction() {
}

BraveRewardsIncludeInAutoContributionFunction::
  ~BraveRewardsIncludeInAutoContributionFunction() {
}

ExtensionFunction::ResponseAction
  BraveRewardsIncludeInAutoContributionFunction::Run() {
  std::unique_ptr<brave_rewards::IncludeInAutoContribution::Params> params(
      brave_rewards::IncludeInAutoContribution::Params::Create(args()));
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
      brave_rewards::GetPublisherData::Params::Create(args()));
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
    base::Value data(base::Value::Type::DICTIONARY);
    return RespondNow(OneArgument(std::move(data)));
  }

  rewards_service->GetRewardsParameters(base::BindOnce(
      &BraveRewardsGetRewardsParametersFunction::OnGet,
      this));
  return RespondLater();
}

void BraveRewardsGetRewardsParametersFunction::OnGet(
    ledger::type::RewardsParametersPtr parameters) {
  base::DictionaryValue data;

  if (!parameters) {
    return Respond(OneArgument(std::move(data)));
  }

  data.SetDouble("rate", parameters->rate);
  auto monthly_choices = std::make_unique<base::ListValue>();
  for (auto const& item : parameters->monthly_tip_choices) {
    monthly_choices->Append(base::Value(item));
  }
  data.SetList("monthlyTipChoices", std::move(monthly_choices));

  auto ac_choices = std::make_unique<base::ListValue>();
  for (double const& choice : parameters->auto_contribute_choices) {
    ac_choices->Append(choice);
  }
  data.SetList("autoContributeChoices", std::move(ac_choices));

  Respond(OneArgument(std::move(data)));
}

BraveRewardsGetBalanceReportFunction::
~BraveRewardsGetBalanceReportFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsGetBalanceReportFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    base::Value data(base::Value::Type::DICTIONARY);
    return RespondNow(OneArgument(std::move(data)));
  }

  std::unique_ptr<brave_rewards::GetBalanceReport::Params> params(
      brave_rewards::GetBalanceReport::Params::Create(args()));

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
  base::Value data(base::Value::Type::DICTIONARY);
  if (!report) {
    Respond(OneArgument(std::move(data)));
    return;
  }
  data.SetDoubleKey("ads", report->earning_from_ads);
  data.SetDoubleKey("contribute", report->auto_contribute);
  data.SetDoubleKey("grant", report->grants);
  data.SetDoubleKey("tips", report->one_time_donation);
  data.SetDoubleKey("monthly", report->recurring_donation);
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
      brave_rewards::ClaimPromotion::Params::Create(args()));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    base::Value data(base::Value::Type::DICTIONARY);
    data.SetIntKey("result", 1);
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
  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("result", static_cast<int>(result));
  data.SetStringKey("promotionId", promotion_id);
  data.SetStringKey("captchaImage", captcha_image);
  data.SetStringKey("captchaId", captcha_id);
  data.SetStringKey("hint", hint);
  Respond(OneArgument(std::move(data)));
}

BraveRewardsAttestPromotionFunction::
~BraveRewardsAttestPromotionFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsAttestPromotionFunction::Run() {
  std::unique_ptr<brave_rewards::AttestPromotion::Params> params(
      brave_rewards::AttestPromotion::Params::Create(args()));
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(OneArgument(base::Value(1)));
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
  base::Value data(base::Value::Type::DICTIONARY);
  data.SetStringKey("promotionId", promotion_id);

  if (!promotion) {
    Respond(
        TwoArguments(base::Value(static_cast<int>(result)), std::move(data)));
    return;
  }

  data.SetIntKey("expiresAt", promotion->expires_at);
  data.SetDoubleKey("amount", promotion->approximate_value);
  data.SetIntKey("type", static_cast<int>(promotion->type));
  Respond(TwoArguments(base::Value(static_cast<int>(result)), std::move(data)));
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
    return RespondNow(OneArgument(base::Value(0.0)));
  }

  rewards_service->GetPendingContributionsTotal(base::BindOnce(
      &BraveRewardsGetPendingContributionsTotalFunction::OnGetPendingTotal,
      this));
  return RespondLater();
}

void BraveRewardsGetPendingContributionsTotalFunction::OnGetPendingTotal(
    double amount) {
  Respond(OneArgument(base::Value(amount)));
}

BraveRewardsSaveAdsSettingFunction::~BraveRewardsSaveAdsSettingFunction() {
}

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
    const auto is_enabled =
        params->value == "true" && ads_service->IsSupportedLocale();
    rewards_service->SetAdsEnabled(is_enabled);
  }

  return RespondNow(NoArguments());
}

BraveRewardsSetAutoContributeEnabledFunction::
~BraveRewardsSetAutoContributeEnabledFunction() {
}

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
  Respond(OneArgument(base::Value(enabled)));
}

BraveRewardsSaveRecurringTipFunction::
~BraveRewardsSaveRecurringTipFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsSaveRecurringTipFunction::Run() {
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
    ledger::type::Result result) {
  Respond(result == ledger::type::Result::LEDGER_OK ? NoArguments()
                                                    : Error("Failed to save"));
}

BraveRewardsRemoveRecurringTipFunction::
~BraveRewardsRemoveRecurringTipFunction() {
}

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

  rewards_service->GetRecurringTips(base::BindOnce(
      &BraveRewardsGetRecurringTipsFunction::OnGetRecurringTips, this));
  return RespondLater();
}

void BraveRewardsGetRecurringTipsFunction::OnGetRecurringTips(
    ledger::type::PublisherInfoList list) {
  base::DictionaryValue result;
  auto recurringTips = std::make_unique<base::ListValue>();

  if (!list.empty()) {
    for (const auto& item : list) {
      auto tip = std::make_unique<base::DictionaryValue>();
      tip->SetString("publisherKey", item->id);
      tip->SetDouble("amount", item->weight);
      recurringTips->Append(std::move(tip));
    }
  }

  result.SetList("recurringTips", std::move(recurringTips));
  Respond(OneArgument(std::move(result)));
}

BraveRewardsGetPublisherBannerFunction::
~BraveRewardsGetPublisherBannerFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetPublisherBannerFunction::Run() {
  std::unique_ptr<brave_rewards::GetPublisherBanner::Params> params(
      brave_rewards::GetPublisherBanner::Params::Create(args()));

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
  base::DictionaryValue result;

  if (banner) {
    result.SetString("publisherKey", banner->publisher_key);
    result.SetString("title", banner->title);
    result.SetString("name", banner->name);
    result.SetString("description", banner->description);
    result.SetString("background", banner->background);
    result.SetString("logo", banner->logo);
    result.SetString("provider", banner->provider);
    result.SetInteger("verified", static_cast<int>(banner->status));

    auto amounts = std::make_unique<base::ListValue>();
    for (auto const& value : banner->amounts) {
      amounts->Append(value);
    }
    result.SetList("amounts", std::move(amounts));

    base::Value links = base::Value(base::Value::Type::DICTIONARY);
    for (auto const& item : banner->links) {
      links.SetStringKey(item.first, item.second);
    }
    result.SetPath("links", std::move(links));
  }

  Respond(OneArgument(std::move(result)));
}

BraveRewardsRefreshPublisherFunction::~BraveRewardsRefreshPublisherFunction() {
}

ExtensionFunction::ResponseAction BraveRewardsRefreshPublisherFunction::Run() {
  std::unique_ptr<brave_rewards::RefreshPublisher::Params> params(
      brave_rewards::RefreshPublisher::Params::Create(args()));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(
        TwoArguments(base::Value(false), base::Value(std::string())));
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
  Respond(TwoArguments(base::Value(static_cast<int>(status)),
                       base::Value(publisher_key)));
}

BraveRewardsGetAllNotificationsFunction::
~BraveRewardsGetAllNotificationsFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetAllNotificationsFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);

  base::ListValue list;

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
      args->Append(arg);
    }

    item->SetList("args", std::move(args));
    list.Append(std::move(item));
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
          args()));

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
    RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(OneArgument(base::Value(false)));
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
  Respond(OneArgument(base::Value(value)));
}

BraveRewardsIsAutoContributeSupportedFunction::
    ~BraveRewardsIsAutoContributeSupportedFunction() {}

ExtensionFunction::ResponseAction
BraveRewardsIsAutoContributeSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  return RespondNow(
      OneArgument(base::Value(rewards_service->IsAutoContributeSupported())));
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
    base::DictionaryValue balance_value;
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
  base::Value balance_value(base::Value::Type::DICTIONARY);
  if (result == ledger::type::Result::LEDGER_OK && balance) {
    balance_value.SetDoubleKey("total", balance->total);

    base::Value wallets(base::Value::Type::DICTIONARY);
    for (auto const& rate : balance->wallets) {
      wallets.SetDoubleKey(rate.first, rate.second);
    }
    balance_value.SetKey("wallets", std::move(wallets));
  } else {
    balance_value.SetDoubleKey("total", 0.0);
    base::Value wallets(base::Value::Type::DICTIONARY);
    balance_value.SetKey("wallets", std::move(wallets));
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
    base::Value data(base::Value::Type::DICTIONARY);
    return RespondNow(OneArgument(std::move(data)));
  }

  rewards_service->GetExternalWallet(base::BindOnce(
      &BraveRewardsGetExternalWalletFunction::OnGetExternalWallet, this));
  return RespondLater();
}

void BraveRewardsGetExternalWalletFunction::OnGetExternalWallet(
    const ledger::type::Result result,
    ledger::type::ExternalWalletPtr wallet) {
  if (!wallet) {
    Respond(OneArgument(base::Value(static_cast<int>(result))));
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);

  data.SetStringKey("type", wallet->type);
  data.SetStringKey("address", wallet->address);
  data.SetIntKey("status", static_cast<int>(wallet->status));
  data.SetStringKey("verifyUrl", wallet->verify_url);
  data.SetStringKey("addUrl", wallet->add_url);
  data.SetStringKey("withdrawUrl", wallet->withdraw_url);
  data.SetStringKey("userName", wallet->user_name);
  data.SetStringKey("accountUrl", wallet->account_url);
  data.SetStringKey("loginUrl", wallet->login_url);
  data.SetStringKey("activityUrl", wallet->activity_url);

  Respond(TwoArguments(base::Value(static_cast<int>(result)), std::move(data)));
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

  rewards_service->DisconnectWallet();
  return RespondNow(NoArguments());
}

BraveRewardsGetRewardsEnabledFunction::
    ~BraveRewardsGetRewardsEnabledFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsGetRewardsEnabledFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  bool enabled =
      profile->GetPrefs()->GetBoolean(::brave_rewards::prefs::kEnabled);
  return RespondNow(OneArgument(base::Value(enabled)));
}

BraveRewardsGetAdsEnabledFunction::
~BraveRewardsGetAdsEnabledFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetAdsEnabledFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service =
      AdsServiceFactory::GetForProfile(profile);

  if (!ads_service) {
    return RespondNow(Error("Ads service is not initialized"));
  }

  const bool enabled = ads_service->IsEnabled();
  return RespondNow(OneArgument(base::Value(enabled)));
}

BraveRewardsGetAdsAccountStatementFunction::
    ~BraveRewardsGetAdsAccountStatementFunction() {}

ExtensionFunction::ResponseAction
BraveRewardsGetAdsAccountStatementFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service =
      AdsServiceFactory::GetForProfile(profile);

  if (!ads_service) {
    return RespondNow(Error("Ads service is not initialized"));
  }

  AddRef();  // Balanced in OnGetAdsAccountStatement().

  ads_service->GetAccountStatement(base::BindOnce(
      &BraveRewardsGetAdsAccountStatementFunction::OnGetAdsAccountStatement,
      this));
  return RespondLater();
}

void BraveRewardsGetAdsAccountStatementFunction::OnGetAdsAccountStatement(
    const bool success,
    const double next_payment_date,
    const int ads_received_this_month,
    const double earnings_this_month,
    const double earnings_last_month) {
  if (!success) {
    Respond(OneArgument(base::Value(success)));
  } else {
    base::Value statement(base::Value::Type::DICTIONARY);
    statement.SetDoubleKey("nextPaymentDate", next_payment_date * 1000);
    statement.SetIntKey("adsReceivedThisMonth", ads_received_this_month);
    statement.SetDoubleKey("earningsThisMonth", earnings_this_month);
    statement.SetDoubleKey("earningsLastMonth", earnings_last_month);

    Respond(TwoArguments(base::Value(success), std::move(statement)));
  }

  Release();  // Balanced in Run()
}

BraveRewardsGetAdsSupportedFunction::
~BraveRewardsGetAdsSupportedFunction() {
}

ExtensionFunction::ResponseAction
BraveRewardsGetAdsSupportedFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service =
      AdsServiceFactory::GetForProfile(profile);

  if (!ads_service) {
    return RespondNow(Error("Ads service is not initialized"));
  }

  const bool supported = ads_service->IsSupportedLocale();
  return RespondNow(OneArgument(base::Value(supported)));
}

BraveRewardsGetAdsDataFunction::~BraveRewardsGetAdsDataFunction() {}

ExtensionFunction::ResponseAction BraveRewardsGetAdsDataFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  AdsService* ads_service = AdsServiceFactory::GetForProfile(profile);

  if (!ads_service) {
    return RespondNow(Error("Ads service is not initialized"));
  }

  base::DictionaryValue ads_data;

  auto is_supported_locale = ads_service->IsSupportedLocale();
  ads_data.SetBoolean("adsIsSupported", is_supported_locale);

  auto is_enabled = ads_service->IsEnabled();
  ads_data.SetBoolean("adsEnabled", is_enabled);

  auto ads_per_hour = ads_service->GetAdsPerHour();
  ads_data.SetInteger("adsPerHour", ads_per_hour);

  const std::string subdivision_targeting_code =
      ads_service->GetAdsSubdivisionTargetingCode();
  ads_data.SetString(kAdsSubdivisionTargeting, subdivision_targeting_code);

  const std::string auto_detected_subdivision_targeting_code =
      ads_service->GetAutoDetectedAdsSubdivisionTargetingCode();
  ads_data.SetString(kAutoDetectedAdsSubdivisionTargeting,
                     auto_detected_subdivision_targeting_code);

  const bool should_allow_subdivision_ad_targeting =
      ads_service->ShouldAllowAdsSubdivisionTargeting();
  ads_data.SetBoolean(kShouldAllowAdsSubdivisionTargeting,
                      should_allow_subdivision_ad_targeting);

  ads_data.SetBoolean("adsUIEnabled", true);

  return RespondNow(OneArgument(std::move(ads_data)));
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

  rewards_service->GetAnonWalletStatus(base::BindOnce(
      &BraveRewardsGetAnonWalletStatusFunction::OnGetAnonWalletStatus, this));
  return RespondLater();
}

void BraveRewardsGetAnonWalletStatusFunction::OnGetAnonWalletStatus(
    const ledger::type::Result result) {
  Respond(OneArgument(base::Value(static_cast<int>(result))));
}

BraveRewardsIsInitializedFunction::
~BraveRewardsIsInitializedFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsIsInitializedFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  return RespondNow(OneArgument(
      base::Value(rewards_service && rewards_service->IsInitialized())));
}

BraveRewardsShouldShowOnboardingFunction::
~BraveRewardsShouldShowOnboardingFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsShouldShowOnboardingFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service) {
    return RespondNow(Error("Rewards service is not initialized"));
  }

  const bool should_show = rewards_service->ShouldShowOnboarding();
  return RespondNow(OneArgument(base::Value(should_show)));
}

BraveRewardsGetScheduledCaptchaInfoFunction::
    ~BraveRewardsGetScheduledCaptchaInfoFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsGetScheduledCaptchaInfoFunction::Run() {
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
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

  base::DictionaryValue dict;
  dict.SetString("url", url);
  dict.SetBoolean("maxAttemptsExceeded", max_attempts_exceeded);

  return RespondNow(OneArgument(std::move(dict)));
#else
  return RespondNow(Error("Adaptive captcha not supported"));
#endif
}

BraveRewardsUpdateScheduledCaptchaResultFunction::
    ~BraveRewardsUpdateScheduledCaptchaResultFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsUpdateScheduledCaptchaResultFunction::Run() {
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
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
#else
  return RespondNow(Error("Adaptive captcha not supported"));
#endif
}

BraveRewardsEnableRewardsFunction::~BraveRewardsEnableRewardsFunction() =
    default;

ExtensionFunction::ResponseAction BraveRewardsEnableRewardsFunction::Run() {
  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  if (!rewards_service)
    return RespondNow(Error("Rewards service is not initialized"));

  rewards_service->EnableRewards();
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
    ledger::type::AutoContributePropertiesPtr properties) {
  base::Value prefs(base::Value::Type::DICTIONARY);
  prefs.SetBoolKey("autoContributeEnabled", properties->enabled_contribute);
  prefs.SetDoubleKey("autoContributeAmount", properties->amount);

  auto* ads_service = AdsServiceFactory::GetForProfile(
      Profile::FromBrowserContext(browser_context()));

  if (ads_service) {
    prefs.SetBoolKey("adsEnabled", ads_service->IsEnabled());
    prefs.SetDoubleKey("adsPerHour",
                       static_cast<double>(ads_service->GetAdsPerHour()));
  } else {
    prefs.SetBoolKey("adsEnabled", false);
    prefs.SetDoubleKey("adsPerHour", 0);
  }

  Respond(OneArgument(std::move(prefs)));
}

BraveRewardsUpdatePrefsFunction::~BraveRewardsUpdatePrefsFunction() = default;

ExtensionFunction::ResponseAction BraveRewardsUpdatePrefsFunction::Run() {
  auto params = brave_rewards::UpdatePrefs::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  auto* profile = Profile::FromBrowserContext(browser_context());
  auto* rewards_service = RewardsServiceFactory::GetForProfile(profile);
  auto* ads_service = AdsServiceFactory::GetForProfile(profile);

  if (rewards_service) {
    bool* ac_enabled = params->prefs.auto_contribute_enabled.get();
    if (ac_enabled)
      rewards_service->SetAutoContributeEnabled(*ac_enabled);

    double* ac_amount = params->prefs.auto_contribute_amount.get();
    if (ac_amount)
      rewards_service->SetAutoContributionAmount(*ac_amount);
  }

  if (ads_service) {
    bool* ads_enabled = params->prefs.ads_enabled.get();
    if (ads_enabled)
      ads_service->SetEnabled(*ads_enabled);

    int* ads_per_hour = params->prefs.ads_per_hour.get();
    if (ads_per_hour)
      ads_service->SetAdsPerHour(*ads_per_hour);
  }

  return RespondNow(NoArguments());
}

BraveRewardsRequestAdsEnabledPopupClosedFunction::
    ~BraveRewardsRequestAdsEnabledPopupClosedFunction() = default;

ExtensionFunction::ResponseAction
BraveRewardsRequestAdsEnabledPopupClosedFunction::Run() {
  auto params(
      brave_rewards::RequestAdsEnabledPopupClosed::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  Profile* profile = Profile::FromBrowserContext(browser_context());
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile);

  if (rewards_service) {
    rewards_service->RequestAdsEnabledPopupClosed(params->ads_enabled);
  }

  return RespondNow(NoArguments());
}

}  // namespace api
}  // namespace extensions
