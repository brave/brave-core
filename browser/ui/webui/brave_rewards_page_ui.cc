/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_page_ui.h"

#include <stdint.h>

#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "base/i18n/time_formatting.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/strings/string_number_conversions.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/bindings_policy.h"
#include "bat/ledger/mojom_structs.h"

#if defined(BRAVE_CHROMIUM_BUILD)
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_page_generated_map.h"
#else
#include "components/brave_rewards/settings/resources/grit/brave_rewards_settings_generated_map.h"
#include "components/grit/components_resources.h"
#include "components/grit/components_scaled_resources.h"
#endif
#if defined(OS_ANDROID)
#include "content/public/browser/url_data_source.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "components/favicon_base/favicon_url_parser.h"
#endif


using content::WebUIMessageHandler;

namespace {

// The handler for Javascript messages for Brave about: pages
class RewardsDOMHandler : public WebUIMessageHandler,
    public brave_ads::AdsServiceObserver,
    public brave_rewards::RewardsNotificationServiceObserver,
    public brave_rewards::RewardsServiceObserver {
 public:
  RewardsDOMHandler();
  ~RewardsDOMHandler() override;

  void Init();

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void IsInitialized(const base::ListValue* args);
  void HandleCreateWalletRequested(const base::ListValue* args);
  void GetRewardsParameters(const base::ListValue* args);
  void GetAutoContributeProperties(const base::ListValue* args);
  void FetchPromotions(const base::ListValue* args);
  void ClaimPromotion(const base::ListValue* args);
  void AttestPromotion(const base::ListValue* args);
  void GetWalletPassphrase(const base::ListValue* args);
  void RecoverWallet(const base::ListValue* args);
  void GetReconcileStamp(const base::ListValue* args);
  void SaveSetting(const base::ListValue* args);
  void UpdateAdRewards(const base::ListValue* args);
  void OnContentSiteList(ledger::type::PublisherInfoList list);
  void OnExcludedSiteList(ledger::type::PublisherInfoList list);
  void ExcludePublisher(const base::ListValue* args);
  void RestorePublishers(const base::ListValue* args);
  void RestorePublisher(const base::ListValue* args);
  void WalletExists(const base::ListValue* args);
  void GetAutoContributionAmount(const base::ListValue* args);
  void RemoveRecurringTip(const base::ListValue* args);
  void GetRecurringTips(const base::ListValue* args);
  void GetOneTimeTips(const base::ListValue* args);
  void GetContributionList(const base::ListValue* args);
  void GetAdsData(const base::ListValue* args);
  void GetAdsHistory(const base::ListValue* args);
  void OnGetAdsHistory(const base::ListValue& history);
  void ToggleAdThumbUp(const base::ListValue* args);
  void OnToggleAdThumbUp(
      const std::string& creative_instance_id,
      const int action);
  void ToggleAdThumbDown(const base::ListValue* args);
  void OnToggleAdThumbDown(
      const std::string& creative_instance_id,
      const int action);
  void ToggleAdOptInAction(const base::ListValue* args);
  void OnToggleAdOptInAction(
      const std::string& category,
      const int action);
  void ToggleAdOptOutAction(const base::ListValue* args);
  void OnToggleAdOptOutAction(
      const std::string& category,
      const int action);
  void ToggleSaveAd(const base::ListValue* args);
  void OnToggleSaveAd(
      const std::string& creative_instance_id,
      const bool saved);
  void ToggleFlagAd(const base::ListValue* args);
  void OnToggleFlagAd(
      const std::string& creative_instance_id,
      const bool flagged);
  void SaveAdsSetting(const base::ListValue* args);
  void SetBackupCompleted(const base::ListValue* args);
  void OnGetWalletPassphrase(const std::string& pass);
  void OnGetContributionAmount(double amount);
  void OnGetAutoContributeProperties(
      ledger::type::AutoContributePropertiesPtr properties);
  void OnGetReconcileStamp(uint64_t reconcile_stamp);
  void OnAutoContributePropsReady(
      ledger::type::AutoContributePropertiesPtr properties);
  void OnIsWalletCreated(bool created);
  void GetPendingContributionsTotal(const base::ListValue* args);
  void OnGetPendingContributionsTotal(double amount);
  void OnPublisherInfoUpdated(
      brave_rewards::RewardsService* rewards_service) override;
  void GetTransactionHistory(const base::ListValue* args);
  void GetRewardsMainEnabled(const base::ListValue* args);
  void OnGetRewardsMainEnabled(bool enabled);
  void GetExcludedSites(const base::ListValue* args);

  void OnGetTransactionHistory(
      const bool success,
      const double estimated_pending_rewards,
      const uint64_t next_payment_date_in_seconds,
      const uint64_t ad_notifications_received_this_month);

  void OnGetRecurringTips(ledger::type::PublisherInfoList list);

  void OnGetOneTimeTips(ledger::type::PublisherInfoList list);

  void SetInlineTippingPlatformEnabled(const base::ListValue* args);

  void GetPendingContributions(const base::ListValue* args);
  void OnGetPendingContributions(
      ledger::type::PendingContributionInfoList list);
  void RemovePendingContribution(const base::ListValue* args);
  void RemoveAllPendingContributions(const base::ListValue* args);
  void FetchBalance(const base::ListValue* args);
  void OnFetchBalance(
    const ledger::type::Result result,
    ledger::type::BalancePtr balance);

  void GetExternalWallet(const base::ListValue* args);
  void OnGetExternalWallet(
      const ledger::type::Result result,
      ledger::type::ExternalWalletPtr wallet);
  void ProcessRewardsPageUrl(const base::ListValue* args);

  void OnProcessRewardsPageUrl(
    const ledger::type::Result result,
    const std::string& wallet_type,
    const std::string& action,
    const std::map<std::string, std::string>& args);

  void DisconnectWallet(const base::ListValue* args);

  void OnlyAnonWallet(const base::ListValue* args);

  void GetBalanceReport(const base::ListValue* args);

  void OnGetBalanceReport(
      const uint32_t month,
      const uint32_t year,
      const ledger::type::Result result,
      ledger::type::BalanceReportInfoPtr report);

  void GetMonthlyReport(const base::ListValue* args);

  void GetAllMonthlyReportIds(const base::ListValue* args);
  void GetCountryCode(const base::ListValue* args);

  void OnGetMonthlyReport(
      const uint32_t month,
      const uint32_t year,
      ledger::type::MonthlyReportInfoPtr report);

  void OnGetAllMonthlyReportIds(const std::vector<std::string>& ids);

  void OnGetRewardsParameters(ledger::type::RewardsParametersPtr parameters);

  void CompleteReset(const base::ListValue* args);

  // RewardsServiceObserver implementation
  void OnWalletInitialized(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result) override;
  void OnFetchPromotions(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result,
      const ledger::type::PromotionList& list) override;
  void OnPromotionFinished(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result,
      ledger::type::PromotionPtr promotion) override;
  void OnRecoverWallet(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result) override;
  void OnExcludedSitesChanged(
      brave_rewards::RewardsService* rewards_service,
      std::string publisher_id,
      bool excluded) override;
  void OnReconcileComplete(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result,
      const std::string& contribution_id,
      const double amount,
      const ledger::type::RewardsType type,
      const ledger::type::ContributionProcessor processor) override;

  void OnPendingContributionSaved(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result) override;

  void OnRewardsMainEnabled(
      brave_rewards::RewardsService* rewards_service,
      bool rewards_main_enabled) override;

  void OnPublisherListNormalized(
      brave_rewards::RewardsService* rewards_service,
      ledger::type::PublisherInfoList list) override;

  void OnTransactionHistoryChanged(
      brave_rewards::RewardsService* rewards_service) override;

  void OnRecurringTipSaved(
      brave_rewards::RewardsService* rewards_service,
      bool success) override;

  void OnRecurringTipRemoved(
      brave_rewards::RewardsService* rewards_service,
      bool success) override;

  void OnPendingContributionRemoved(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result) override;

  void OnDisconnectWallet(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result,
      const std::string& wallet_type) override;

  void OnAdsEnabled(
    brave_rewards::RewardsService* rewards_service,
    bool ads_enabled) override;

  void OnClaimPromotion(
      const std::string& promotion_id,
      const ledger::type::Result result,
      const std::string& captcha_image,
      const std::string& hint,
      const std::string& captcha_id);

  void OnAttestPromotion(
      const std::string& promotion_id,
      const ledger::type::Result result,
      ledger::type::PromotionPtr promotion);

  void OnUnblindedTokensReady(
    brave_rewards::RewardsService* rewards_service) override;

  void ReconcileStampReset() override;

  void OnCompleteReset(const bool success) override;

  // RewardsNotificationsServiceObserver implementation
  void OnNotificationAdded(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
          notification) override;
  void OnNotificationDeleted(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
          notification) override;
  void OnAllNotificationsDeleted(brave_rewards::RewardsNotificationService*
        rewards_notification_service) override;
  void OnGetNotification(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotification&
          notification) override;
  void OnGetAllNotifications(
      brave_rewards::RewardsNotificationService* rewards_notification_service,
      const brave_rewards::RewardsNotificationService::RewardsNotificationsList&
          notifications_list) override;

  // AdsServiceObserver implementation
  void OnAdRewardsChanged() override;

  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED
  brave_ads::AdsService* ads_service_;  // NOT OWNED
  base::WeakPtrFactory<RewardsDOMHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RewardsDOMHandler);
};

namespace {

const int kDaysOfAdsHistory = 7;

const char kShouldAllowAdsSubdivisionTargeting[] =
    "shouldAllowAdsSubdivisionTargeting";
const char kAdsSubdivisionTargeting[] = "adsSubdivisionTargeting";
const char kAutomaticallyDetectedAdsSubdivisionTargeting[] =
    "automaticallyDetectedAdsSubdivisionTargeting";

}  // namespace

RewardsDOMHandler::RewardsDOMHandler() : weak_factory_(this) {}

RewardsDOMHandler::~RewardsDOMHandler() {
  if (rewards_service_) {
    rewards_service_->RemoveObserver(this);
  }

  if (ads_service_) {
    ads_service_->RemoveObserver(this);
  }
}

void RewardsDOMHandler::RegisterMessages() {
#if defined(OS_ANDROID)
  // Create our favicon data source.
  Profile* profile = Profile::FromWebUI(web_ui());
  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFaviconLegacy));
#endif

  web_ui()->RegisterMessageCallback("brave_rewards.isInitialized",
      base::BindRepeating(&RewardsDOMHandler::IsInitialized,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.createWalletRequested",
      base::BindRepeating(&RewardsDOMHandler::HandleCreateWalletRequested,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getRewardsParameters",
      base::BindRepeating(&RewardsDOMHandler::GetRewardsParameters,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getAutoContributeProperties",
      base::BindRepeating(&RewardsDOMHandler::GetAutoContributeProperties,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.fetchPromotions",
      base::BindRepeating(&RewardsDOMHandler::FetchPromotions,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.claimPromotion",
      base::BindRepeating(&RewardsDOMHandler::ClaimPromotion,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.attestPromotion",
      base::BindRepeating(&RewardsDOMHandler::AttestPromotion,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getWalletPassphrase",
      base::BindRepeating(&RewardsDOMHandler::GetWalletPassphrase,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.recoverWallet",
      base::BindRepeating(&RewardsDOMHandler::RecoverWallet,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getReconcileStamp",
      base::BindRepeating(&RewardsDOMHandler::GetReconcileStamp,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.saveSetting",
      base::BindRepeating(&RewardsDOMHandler::SaveSetting,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.updateAdRewards",
      base::BindRepeating(&RewardsDOMHandler::UpdateAdRewards,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.excludePublisher",
      base::BindRepeating(&RewardsDOMHandler::ExcludePublisher,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.restorePublishers",
      base::BindRepeating(&RewardsDOMHandler::RestorePublishers,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.restorePublisher",
      base::BindRepeating(&RewardsDOMHandler::RestorePublisher,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.checkWalletExistence",
      base::BindRepeating(&RewardsDOMHandler::WalletExists,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getContributionAmount",
      base::BindRepeating(&RewardsDOMHandler::GetAutoContributionAmount,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.removeRecurringTip",
      base::BindRepeating(&RewardsDOMHandler::RemoveRecurringTip,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getRecurringTips",
      base::BindRepeating(&RewardsDOMHandler::GetRecurringTips,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getOneTimeTips",
      base::BindRepeating(&RewardsDOMHandler::GetOneTimeTips,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getContributionList",
      base::BindRepeating(&RewardsDOMHandler::GetContributionList,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getAdsData",
      base::BindRepeating(&RewardsDOMHandler::GetAdsData,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getAdsHistory",
      base::BindRepeating(&RewardsDOMHandler::GetAdsHistory,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.toggleAdThumbUp",
      base::BindRepeating(&RewardsDOMHandler::ToggleAdThumbUp,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.toggleAdThumbDown",
      base::BindRepeating(&RewardsDOMHandler::ToggleAdThumbDown,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.toggleAdOptInAction",
      base::BindRepeating(&RewardsDOMHandler::ToggleAdOptInAction,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.toggleAdOptOutAction",
      base::BindRepeating(&RewardsDOMHandler::ToggleAdOptOutAction,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.toggleSaveAd",
      base::BindRepeating(&RewardsDOMHandler::ToggleSaveAd,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.toggleFlagAd",
      base::BindRepeating(&RewardsDOMHandler::ToggleFlagAd,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.saveAdsSetting",
      base::BindRepeating(&RewardsDOMHandler::SaveAdsSetting,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.setBackupCompleted",
      base::BindRepeating(&RewardsDOMHandler::SetBackupCompleted,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getPendingContributionsTotal",
      base::BindRepeating(&RewardsDOMHandler::GetPendingContributionsTotal,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.getTransactionHistory",
      base::BindRepeating(&RewardsDOMHandler::GetTransactionHistory,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getRewardsMainEnabled",
      base::BindRepeating(&RewardsDOMHandler::GetRewardsMainEnabled,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.setInlineTippingPlatformEnabled",
      base::BindRepeating(&RewardsDOMHandler::SetInlineTippingPlatformEnabled,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getPendingContributions",
      base::BindRepeating(&RewardsDOMHandler::GetPendingContributions,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.removePendingContribution",
      base::BindRepeating(&RewardsDOMHandler::RemovePendingContribution,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards.removeAllPendingContribution",
      base::BindRepeating(&RewardsDOMHandler::RemoveAllPendingContributions,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getExcludedSites",
      base::BindRepeating(&RewardsDOMHandler::GetExcludedSites,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.fetchBalance",
      base::BindRepeating(&RewardsDOMHandler::FetchBalance,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getExternalWallet",
      base::BindRepeating(&RewardsDOMHandler::GetExternalWallet,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.processRewardsPageUrl",
      base::BindRepeating(&RewardsDOMHandler::ProcessRewardsPageUrl,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.disconnectWallet",
      base::BindRepeating(&RewardsDOMHandler::DisconnectWallet,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.onlyAnonWallet",
      base::BindRepeating(&RewardsDOMHandler::OnlyAnonWallet,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getBalanceReport",
      base::BindRepeating(&RewardsDOMHandler::GetBalanceReport,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getMonthlyReport",
      base::BindRepeating(&RewardsDOMHandler::GetMonthlyReport,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getMonthlyReportIds",
      base::BindRepeating(&RewardsDOMHandler::GetAllMonthlyReportIds,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getCountryCode",
      base::BindRepeating(&RewardsDOMHandler::GetCountryCode,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.completeReset",
      base::BindRepeating(&RewardsDOMHandler::CompleteReset,
      base::Unretained(this)));
}

void RewardsDOMHandler::Init() {
  Profile* profile = Profile::FromWebUI(web_ui());

  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_) {
    rewards_service_->AddObserver(this);
  }

  ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile);
  if (ads_service_) {
    ads_service_->AddObserver(this);
  }
}

void RewardsDOMHandler::IsInitialized(
    const base::ListValue* args) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  if (rewards_service_ && rewards_service_->IsInitialized()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.initialized",
        base::Value(0));
  }
}

void RewardsDOMHandler::HandleCreateWalletRequested(
    const base::ListValue* args) {
  if (!rewards_service_)
    return;

  rewards_service_->CreateWallet(
      base::BindOnce(&RewardsDOMHandler::OnWalletInitialized,
                     weak_factory_.GetWeakPtr(),
                     rewards_service_));
}

void RewardsDOMHandler::GetRewardsParameters(const base::ListValue* args) {
  if (!rewards_service_)
    return;

  rewards_service_->GetRewardsParameters(
      base::BindOnce(&RewardsDOMHandler::OnGetRewardsParameters,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetRewardsParameters(
    ledger::type::RewardsParametersPtr parameters) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::DictionaryValue data;
  if (parameters) {
    auto choices = std::make_unique<base::ListValue>();
    for (double const& choice : parameters->auto_contribute_choices) {
      choices->AppendDouble(choice);
    }

    data.SetDouble("rate", parameters->rate);
    data.SetDouble("autoContributeChoice", parameters->auto_contribute_choice);
    data.SetList("autoContributeChoices", std::move(choices));
  }
  web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.rewardsParameters", data);
}

void RewardsDOMHandler::OnWalletInitialized(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result) {
  if (!web_ui()->CanCallJavascript())
    return;

  if (result == ledger::type::Result::WALLET_CREATED) {
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletCreated");
    return;
  }

  if (result != ledger::type::Result::NO_LEDGER_STATE &&
      result != ledger::type::Result::LEDGER_OK) {
    // Report back all errors except when ledger_state is missing
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletCreateFailed");
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.initialized",
      base::Value(static_cast<int>(result)));
}

void RewardsDOMHandler::GetAutoContributeProperties(
    const base::ListValue* args) {
  if (!rewards_service_)
    return;

  rewards_service_->GetAutoContributeProperties(
      base::Bind(&RewardsDOMHandler::OnGetAutoContributeProperties,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetAutoContributeProperties(
    ledger::type::AutoContributePropertiesPtr properties) {
  if (!web_ui()->CanCallJavascript() || !properties) {
    return;
  }

  base::DictionaryValue values;
  values.SetBoolean("enabledContribute", properties->enabled_contribute);
  values.SetInteger("contributionMinTime", properties->contribution_min_time);
  values.SetInteger("contributionMinVisits",
      properties->contribution_min_visits);
  values.SetBoolean("contributionNonVerified",
      properties->contribution_non_verified);
  values.SetBoolean("contributionVideos", properties->contribution_videos);

  web_ui()->CallJavascriptFunctionUnsafe(
    "brave_rewards.autoContributeProperties",
    values);
}

void RewardsDOMHandler::OnFetchPromotions(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result,
    const ledger::type::PromotionList& list) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::ListValue promotions;
  for (const auto& item : list) {
    auto dict = std::make_unique<base::DictionaryValue>();
    dict->SetString("promotionId", item->id);
    dict->SetInteger("type", static_cast<int>(item->type));
    dict->SetInteger("status", static_cast<int>(item->status));
    dict->SetInteger("expiresAt", item->expires_at);
    dict->SetDouble("amount", item->approximate_value);
    promotions.Append(std::move(dict));
  }

  base::DictionaryValue dict;
  dict.SetInteger("result", static_cast<int>(result));
  dict.SetKey("promotions", std::move(promotions));

  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.promotions", dict);
}

void RewardsDOMHandler::FetchPromotions(const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->FetchPromotions();
  }
}

void RewardsDOMHandler::OnClaimPromotion(
      const std::string& promotion_id,
      const ledger::type::Result result,
      const std::string& captcha_image,
      const std::string& hint,
      const std::string& captcha_id) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::DictionaryValue response;
  response.SetInteger("result", static_cast<int>(result));
  response.SetString("promotionId", promotion_id);
  response.SetString("captchaImage", captcha_image);
  response.SetString("captchaId", captcha_id);
  response.SetString("hint", hint);

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.claimPromotion",
      response);
}

void RewardsDOMHandler::ClaimPromotion(const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  if (!rewards_service_) {
    return;
  }

  const std::string promotion_id = args->GetList()[0].GetString();

#if !defined(OS_ANDROID)
  rewards_service_->ClaimPromotion(
      promotion_id,
      base::Bind(&RewardsDOMHandler::OnClaimPromotion,
          weak_factory_.GetWeakPtr(),
          promotion_id));
#else
  // No need for a callback. The UI receives "brave_rewards.promotionFinish".
  brave_rewards::AttestPromotionCallback callback = base::DoNothing();
  rewards_service_->ClaimPromotion(promotion_id, std::move(callback));
#endif
}


void RewardsDOMHandler::AttestPromotion(const base::ListValue *args) {
  CHECK_EQ(2U, args->GetSize());
  if (!rewards_service_) {
    base::DictionaryValue finish;
    finish.SetInteger("status", 1);
    web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.promotionFinish",
      finish);
  }

  const std::string promotion_id = args->GetList()[0].GetString();
  const std::string solution = args->GetList()[1].GetString();
  rewards_service_->AttestPromotion(
      promotion_id,
      solution,
      base::BindOnce(
        &RewardsDOMHandler::OnAttestPromotion,
        weak_factory_.GetWeakPtr(),
        promotion_id));
}

void RewardsDOMHandler::OnAttestPromotion(
    const std::string& promotion_id,
      const ledger::type::Result result,
      ledger::type::PromotionPtr promotion) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::DictionaryValue promotion_dict;
  promotion_dict.SetString("promotionId", promotion_id);

  if (promotion) {
    promotion_dict.SetInteger("expiresAt", promotion->expires_at);
    promotion_dict.SetDouble("amount", promotion->approximate_value);
    promotion_dict.SetInteger("type", static_cast<int>(promotion->type));
  }

  base::DictionaryValue finish;
  finish.SetInteger("result", static_cast<int>(result));
  finish.SetKey("promotion", std::move(promotion_dict));

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.promotionFinish",
      finish);
}

void RewardsDOMHandler::OnPromotionFinished(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result,
    ledger::type::PromotionPtr promotion) {
  if (result != ledger::type::Result::LEDGER_OK) {
    return;
  }

  OnAttestPromotion(
      promotion->id,
      result,
      promotion->Clone());
}

void RewardsDOMHandler::OnGetWalletPassphrase(const std::string& pass) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletPassphrase",
        base::Value(pass));
  }
}

void RewardsDOMHandler::GetWalletPassphrase(const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->GetWalletPassphrase(
        base::Bind(&RewardsDOMHandler::OnGetWalletPassphrase,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::RecoverWallet(const base::ListValue *args) {
  CHECK_EQ(1U, args->GetSize());
  if (rewards_service_) {
    const std::string pass_phrase = args->GetList()[0].GetString();
    rewards_service_->RecoverWallet(pass_phrase);
  }
}

void RewardsDOMHandler::OnRecoverWallet(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.recoverWalletData",
        base::Value(static_cast<int>(result)));
}

void RewardsDOMHandler::OnGetReconcileStamp(uint64_t reconcile_stamp) {
  if (web_ui()->CanCallJavascript()) {
    std::string stamp = std::to_string(reconcile_stamp);
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.reconcileStamp",
        base::Value(stamp));
  }
}

void RewardsDOMHandler::GetReconcileStamp(const base::ListValue* args) {
  if (rewards_service_)
    rewards_service_->GetReconcileStamp(base::Bind(
          &RewardsDOMHandler::OnGetReconcileStamp,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnAutoContributePropsReady(
    ledger::type::AutoContributePropertiesPtr properties) {
  rewards_service_->GetContentSiteList(
      0,
      0,
      properties->contribution_min_time,
      properties->reconcile_stamp,
      properties->contribution_non_verified,
      properties->contribution_min_visits,
      base::Bind(&RewardsDOMHandler::OnContentSiteList,
                 weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnPublisherInfoUpdated(
    brave_rewards::RewardsService* rewards_service) {
  rewards_service_->GetAutoContributeProperties(
      base::Bind(&RewardsDOMHandler::OnAutoContributePropsReady,
        weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::GetExcludedSites(const base::ListValue* args) {
  rewards_service_->GetExcludedList(
      base::Bind(&RewardsDOMHandler::OnExcludedSiteList,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnExcludedSitesChanged(
    brave_rewards::RewardsService* rewards_service,
    std::string publisher_id,
    bool excluded) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.excludedSiteChanged");
}

void RewardsDOMHandler::OnNotificationAdded(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotification&
        notification) {}

void RewardsDOMHandler::OnNotificationDeleted(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotification&
        notification) {
#if defined(OS_ANDROID)
  if (notification.type_ ==
      brave_rewards::RewardsNotificationService::REWARDS_NOTIFICATION_GRANT
      && web_ui()->CanCallJavascript()) {
    base::DictionaryValue finish;
    finish.SetInteger("status", false);
    finish.SetInteger("expiryTime", 0);
    finish.SetString("probi", "0");

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.grantFinish", finish);
  }
#endif
}

void RewardsDOMHandler::OnAllNotificationsDeleted(
    brave_rewards::RewardsNotificationService* rewards_notification_service) {}

void RewardsDOMHandler::OnGetNotification(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotification&
        notification) {}

void RewardsDOMHandler::OnGetAllNotifications(
    brave_rewards::RewardsNotificationService* rewards_notification_service,
    const brave_rewards::RewardsNotificationService::RewardsNotificationsList&
        notifications_list) {}

void RewardsDOMHandler::SaveSetting(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  if (rewards_service_) {
    const std::string key = args->GetList()[0].GetString();
    const std::string value = args->GetList()[1].GetString();

    if (key == "enabledMain") {
      rewards_service_->SetRewardsMainEnabled(value == "true");
    }

    if (key == "contributionMonthly") {
      rewards_service_->SetAutoContributionAmount(std::stod(value));
    }

    if (key == "contributionMinTime") {
      int int_value;
      if (!base::StringToInt(value, &int_value)) {
        LOG(ERROR) << "Min time was not converted to int";
        return;
      }

      rewards_service_->SetPublisherMinVisitTime(int_value);
    }

    if (key == "contributionMinVisits") {
      int int_value;
      if (!base::StringToInt(value, &int_value)) {
        LOG(ERROR) << "Min visits was not converted to int";
        return;
      }

      rewards_service_->SetPublisherMinVisits(int_value);
    }

    if (key == "contributionNonVerified") {
      rewards_service_->SetPublisherAllowNonVerified(value == "true");
    }

    if (key == "contributionVideos") {
      rewards_service_->SetPublisherAllowVideos(value == "true");
    }

    if (key == "enabledContribute") {
      rewards_service_->SetAutoContributeEnabled(value == "true");
    }
  }
}

void RewardsDOMHandler::UpdateAdRewards(const base::ListValue* args) {
  if (!ads_service_) {
    return;
  }

  ads_service_->UpdateAdRewards(/*should_reconcile*/false);
}

void RewardsDOMHandler::ExcludePublisher(const base::ListValue *args) {
  CHECK_EQ(1U, args->GetSize());
  if (!rewards_service_) {
    return;
  }

  const std::string publisherKey = args->GetList()[0].GetString();
  rewards_service_->SetPublisherExclude(publisherKey, true);
}

void RewardsDOMHandler::RestorePublishers(const base::ListValue *args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->RestorePublishers();
}

void RewardsDOMHandler::RestorePublisher(const base::ListValue *args) {
  CHECK_EQ(1U, args->GetSize());
  if (!rewards_service_) {
    return;
  }

  std::string publisherKey = args->GetList()[0].GetString();
  rewards_service_->SetPublisherExclude(publisherKey, false);
}

void RewardsDOMHandler::OnContentSiteList(
    ledger::type::PublisherInfoList list) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  auto publishers = std::make_unique<base::ListValue>();
  for (auto const& item : list) {
    auto publisher = std::make_unique<base::DictionaryValue>();
    publisher->SetString("id", item->id);
    publisher->SetDouble("percentage", item->percent);
    publisher->SetString("publisherKey", item->id);
    publisher->SetInteger("status", static_cast<int>(item->status));
    publisher->SetInteger("excluded", static_cast<int>(item->excluded));
    publisher->SetString("name", item->name);
    publisher->SetString("provider", item->provider);
    publisher->SetString("url", item->url);
    publisher->SetString("favIcon", item->favicon_url);
    publishers->Append(std::move(publisher));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.contributeList",
      *publishers);
}

void RewardsDOMHandler::OnExcludedSiteList(
    ledger::type::PublisherInfoList list) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  auto publishers = std::make_unique<base::ListValue>();
  for (auto const& item : list) {
    auto publisher = std::make_unique<base::DictionaryValue>();
    publisher->SetString("id", item->id);
    publisher->SetInteger("status", static_cast<int>(item->status));
    publisher->SetString("name", item->name);
    publisher->SetString("provider", item->provider);
    publisher->SetString("url", item->url);
    publisher->SetString("favIcon", item->favicon_url);
    publishers->Append(std::move(publisher));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.excludedList",
      *publishers);
}

void RewardsDOMHandler::OnIsWalletCreated(bool created) {
  if (web_ui()->CanCallJavascript())
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletExists",
        base::Value(created));
}

void RewardsDOMHandler::WalletExists(const base::ListValue* args) {
  if (rewards_service_)
    rewards_service_->IsWalletCreated(
        base::Bind(&RewardsDOMHandler::OnIsWalletCreated,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetContributionAmount(double amount) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.contributionAmount",
        base::Value(amount));
  }
}

void RewardsDOMHandler::GetAutoContributionAmount(const base::ListValue* args) {
  if (rewards_service_)
    rewards_service_->GetAutoContributionAmount(
        base::Bind(&RewardsDOMHandler::OnGetContributionAmount,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnReconcileComplete(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result,
    const std::string& contribution_id,
    const double amount,
    const ledger::type::RewardsType type,
    const ledger::type::ContributionProcessor processor) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::DictionaryValue complete;
  complete.SetKey("result", base::Value(static_cast<int>(result)));
  complete.SetKey("type", base::Value(static_cast<int>(type)));

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.reconcileComplete",
      complete);
}

void RewardsDOMHandler::RemoveRecurringTip(const base::ListValue *args) {
  CHECK_EQ(1U, args->GetSize());
  if (rewards_service_) {
    const std::string publisherKey = args->GetList()[0].GetString();
    rewards_service_->RemoveRecurringTip(publisherKey);
  }
}

void RewardsDOMHandler::GetRecurringTips(
    const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->GetRecurringTips(base::BindOnce(
          &RewardsDOMHandler::OnGetRecurringTips,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::OnGetRecurringTips(
    ledger::type::PublisherInfoList list) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }
  auto publishers = std::make_unique<base::ListValue>();

  for (auto const& item : list) {
    auto publisher = std::make_unique<base::DictionaryValue>();
    publisher->SetString("id", item->id);
    publisher->SetDouble("percentage", item->weight);
    publisher->SetString("publisherKey", item->id);
    publisher->SetInteger("status", static_cast<int>(item->status));
    publisher->SetInteger("excluded", static_cast<int>(item->excluded));
    publisher->SetString("name", item->name);
    publisher->SetString("provider", item->provider);
    publisher->SetString("url", item->url);
    publisher->SetString("favIcon", item->favicon_url);
    publisher->SetInteger("tipDate", 0);
    publishers->Append(std::move(publisher));
  }

  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.recurringTips",
                                         *publishers);
}

void RewardsDOMHandler::OnGetOneTimeTips(ledger::type::PublisherInfoList list) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }
  auto publishers = std::make_unique<base::ListValue>();

  for (auto const& item : list) {
    auto publisher = std::make_unique<base::DictionaryValue>();
    publisher->SetString("id", item->id);
    publisher->SetDouble("percentage", item->weight);
    publisher->SetString("publisherKey", item->id);
    publisher->SetInteger("status", static_cast<int>(item->status));
    publisher->SetInteger("excluded", static_cast<int>(item->excluded));
    publisher->SetString("name", item->name);
    publisher->SetString("provider", item->provider);
    publisher->SetString("url", item->url);
    publisher->SetString("favIcon", item->favicon_url);
    publisher->SetInteger("tipDate", item->reconcile_stamp);
    publishers->Append(std::move(publisher));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.currentTips",
      *publishers);
}

void RewardsDOMHandler::GetOneTimeTips(const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->GetOneTimeTips(base::BindOnce(
          &RewardsDOMHandler::OnGetOneTimeTips,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::GetContributionList(const base::ListValue *args) {
  if (rewards_service_) {
    OnPublisherInfoUpdated(rewards_service_);
  }
}

void RewardsDOMHandler::GetAdsData(const base::ListValue *args) {
  if (!ads_service_ || !web_ui()->CanCallJavascript()) {
    return;
  }

  base::DictionaryValue ads_data;

  auto is_supported_locale = ads_service_->IsSupportedLocale();
  ads_data.SetBoolean("adsIsSupported", is_supported_locale);

  auto is_enabled = ads_service_->IsEnabled();
  ads_data.SetBoolean("adsEnabled", is_enabled);

  auto ads_per_hour = ads_service_->GetAdsPerHour();
  ads_data.SetInteger("adsPerHour", ads_per_hour);

  const std::string subdivision_targeting_code =
      ads_service_->GetAdsSubdivisionTargetingCode();
  ads_data.SetString(kAdsSubdivisionTargeting, subdivision_targeting_code);

  const std::string automatically_detected_subdivision_targeting_code =
      ads_service_->GetAutomaticallyDetectedAdsSubdivisionTargetingCode();
  ads_data.SetString(kAutomaticallyDetectedAdsSubdivisionTargeting,
      automatically_detected_subdivision_targeting_code);

  const bool should_allow_subdivision_ad_targeting =
      ads_service_->ShouldAllowAdsSubdivisionTargeting();
  ads_data.SetBoolean(kShouldAllowAdsSubdivisionTargeting,
      should_allow_subdivision_ad_targeting);

#if BUILDFLAG(BRAVE_ADS_ENABLED)
    auto ads_ui_enabled = true;
#else
    auto ads_ui_enabled = false;
#endif
  ads_data.SetBoolean("adsUIEnabled", ads_ui_enabled);

  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.adsData", ads_data);
}

void RewardsDOMHandler::GetAdsHistory(const base::ListValue* args) {
  if (!ads_service_) {
    return;
  }

  const base::Time to_time = base::Time::Now();
  const uint64_t to_timestamp = to_time.ToDoubleT();

  const base::Time from_time = to_time -
      base::TimeDelta::FromDays(kDaysOfAdsHistory - 1);
  const base::Time from_time_local_midnight = from_time.LocalMidnight();
  const uint64_t from_timestamp = from_time_local_midnight.ToDoubleT();

  ads_service_->GetAdsHistory(from_timestamp, to_timestamp,
      base::BindOnce(&RewardsDOMHandler::OnGetAdsHistory,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetAdsHistory(const base::ListValue& ads_history) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.adsHistory",
                                         ads_history);
}

void RewardsDOMHandler::ToggleAdThumbUp(const base::ListValue* args) {
  CHECK_EQ(3U, args->GetSize());
  if (!ads_service_) {
    return;
  }

  const std::string id = args->GetList()[0].GetString();
  const std::string creative_set_id = args->GetList()[1].GetString();
  const int action = args->GetList()[2].GetInt();
  ads_service_->ToggleAdThumbUp(
      id, creative_set_id, action,
      base::BindOnce(&RewardsDOMHandler::OnToggleAdThumbUp,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdThumbUp(
    const std::string& creative_instance_id,
    const int action) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("creativeInstanceId", base::Value(creative_instance_id));
  result.SetKey("action", base::Value(action));
  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.onToggleAdThumbUp",
                                         result);
}

void RewardsDOMHandler::ToggleAdThumbDown(const base::ListValue* args) {
  CHECK_EQ(3U, args->GetSize());
  if (!ads_service_) {
    return;
  }

  const std::string id = args->GetList()[0].GetString();
  const std::string creative_set_id = args->GetList()[1].GetString();
  const int action = args->GetList()[2].GetInt();
  ads_service_->ToggleAdThumbDown(
      id, creative_set_id, action,
      base::BindOnce(&RewardsDOMHandler::OnToggleAdThumbDown,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdThumbDown(
    const std::string& creative_instance_id,
    const int action) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("creativeInstanceId", base::Value(creative_instance_id));
  result.SetKey("action", base::Value(action));
  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.onToggleAdThumbDown",
                                         result);
}

void RewardsDOMHandler::ToggleAdOptInAction(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  if (!ads_service_) {
    return;
  }

  const std::string category = args->GetList()[0].GetString();
  const int action = args->GetList()[1].GetInt();
  ads_service_->ToggleAdOptInAction(
      category, action,
      base::BindOnce(&RewardsDOMHandler::OnToggleAdOptInAction,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdOptInAction(const std::string& category,
                                              int action) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("category", base::Value(category));
  result.SetKey("action", base::Value(action));
  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.onToggleAdOptInAction",
                                         result);
}

void RewardsDOMHandler::ToggleAdOptOutAction(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  if (!ads_service_) {
    return;
  }

  const std::string category = args->GetList()[0].GetString();
  const int action = args->GetList()[1].GetInt();
  ads_service_->ToggleAdOptOutAction(
      category, action,
      base::BindOnce(&RewardsDOMHandler::OnToggleAdOptOutAction,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleAdOptOutAction(const std::string& category,
                                               int action) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("category", base::Value(category));
  result.SetKey("action", base::Value(action));
  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.onToggleAdOptOutAction",
                                         result);
}

void RewardsDOMHandler::ToggleSaveAd(const base::ListValue* args) {
  CHECK_EQ(3U, args->GetSize());
  if (!ads_service_) {
    return;
  }

  const std::string creative_instance_id = args->GetList()[0].GetString();
  const std::string creative_set_id = args->GetList()[1].GetString();
  const bool saved = args->GetList()[2].GetBool();
  ads_service_->ToggleSaveAd(creative_instance_id, creative_set_id, saved,
      base::BindOnce(&RewardsDOMHandler::OnToggleSaveAd,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleSaveAd(
    const std::string& creative_instance_id,
    bool saved) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("creativeInstanceId", base::Value(creative_instance_id));
  result.SetKey("saved", base::Value(saved));
  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.onToggleSaveAd",
                                         result);
}

void RewardsDOMHandler::ToggleFlagAd(const base::ListValue* args) {
  CHECK_EQ(3U, args->GetSize());
  if (!ads_service_) {
    return;
  }

  const std::string creative_instance_id = args->GetList()[0].GetString();
  const std::string creative_set_id = args->GetList()[1].GetString();
  const bool flagged = args->GetList()[2].GetBool();
  ads_service_->ToggleFlagAd(creative_instance_id, creative_set_id, flagged,
      base::BindOnce(&RewardsDOMHandler::OnToggleFlagAd,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleFlagAd(
      const std::string& creative_instance_id, bool flagged) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("creativeInstanceId", base::Value(creative_instance_id));
  result.SetKey("flagged", base::Value(flagged));
  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.onToggleFlagAd",
                                         result);
}

void RewardsDOMHandler::SaveAdsSetting(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  if (!ads_service_) {
    return;
  }

  const std::string key = args->GetList()[0].GetString();
  const std::string value = args->GetList()[1].GetString();

  if (key == "adsEnabled") {
    const auto is_enabled =
        value == "true" && ads_service_->IsSupportedLocale();
    ads_service_->SetEnabled(is_enabled);
  } else if (key == "adsPerHour") {
    ads_service_->SetAdsPerHour(std::stoull(value));
  } else if (key == kAdsSubdivisionTargeting) {
    ads_service_->SetAdsSubdivisionTargetingCode(value);
  } else if (key == kAutomaticallyDetectedAdsSubdivisionTargeting) {
    ads_service_->SetAutomaticallyDetectedAdsSubdivisionTargetingCode(value);
  }

  base::ListValue* emptyArgs = nullptr;
  GetAdsData(emptyArgs);
}

void RewardsDOMHandler::SetBackupCompleted(const base::ListValue *args) {
  if (web_ui()->CanCallJavascript() && rewards_service_) {
    rewards_service_->SetBackupCompleted();
  }
}

void RewardsDOMHandler::GetPendingContributionsTotal(
    const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->GetPendingContributionsTotal(base::Bind(
          &RewardsDOMHandler::OnGetPendingContributionsTotal,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::OnGetPendingContributionsTotal(double amount) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.pendingContributionTotal", base::Value(amount));
  }
}

void RewardsDOMHandler::OnPendingContributionSaved(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }
  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.onPendingContributionSaved",
      base::Value(static_cast<int>(result)));
}

void RewardsDOMHandler::OnRewardsMainEnabled(
    brave_rewards::RewardsService* rewards_service,
    bool rewards_main_enabled) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.rewardsEnabled",
        base::Value(rewards_main_enabled));
  }
}


void RewardsDOMHandler::OnPublisherListNormalized(
    brave_rewards::RewardsService* rewards_service,
    ledger::type::PublisherInfoList list) {
  OnContentSiteList(std::move(list));
}

void RewardsDOMHandler::GetTransactionHistory(
    const base::ListValue* args) {
  ads_service_->GetTransactionHistory(base::Bind(
      &RewardsDOMHandler::OnGetTransactionHistory,
      weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetTransactionHistory(
    const bool success,
    const double estimated_pending_rewards,
    const uint64_t next_payment_date_in_seconds,
    const uint64_t ad_notifications_received_this_month) {
  if (!success) {
    return;
  }

  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::DictionaryValue history;

  history.SetDouble("adsEstimatedPendingRewards",
      estimated_pending_rewards);

  if (next_payment_date_in_seconds == 0) {
    history.SetString("adsNextPaymentDate", "");
  } else {
    base::Time next_payment_date =
        base::Time::FromDoubleT(next_payment_date_in_seconds);
    history.SetString("adsNextPaymentDate",
        base::TimeFormatWithPattern(next_payment_date, "MMMd"));
  }

  history.SetInteger("adsAdNotificationsReceivedThisMonth",
      ad_notifications_received_this_month);

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.transactionHistory", history);
}

void RewardsDOMHandler::OnTransactionHistoryChanged(
    brave_rewards::RewardsService* rewards_service) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.transactionHistoryChanged");
  }
}

void RewardsDOMHandler::OnAdRewardsChanged() {
  ads_service_->GetTransactionHistory(base::Bind(
      &RewardsDOMHandler::OnGetTransactionHistory,
      weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::GetRewardsMainEnabled(
    const base::ListValue* args) {
  rewards_service_->GetRewardsMainEnabled(base::Bind(
      &RewardsDOMHandler::OnGetRewardsMainEnabled,
      weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetRewardsMainEnabled(
    bool enabled) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.rewardsEnabled",
        base::Value(enabled));
  }
}

void RewardsDOMHandler::OnRecurringTipSaved(
    brave_rewards::RewardsService* rewards_service,
    bool success) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.recurringTipSaved", base::Value(success));
  }
}

void RewardsDOMHandler::OnRecurringTipRemoved(
    brave_rewards::RewardsService* rewards_service,
    bool success) {
  if (!web_ui()->CanCallJavascript()) {
     return;
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.recurringTipRemoved", base::Value(success));
}

void RewardsDOMHandler::SetInlineTippingPlatformEnabled(
    const base::ListValue* args) {
  std::string key;
  args->GetString(0, &key);

  std::string value;
  args->GetString(1, &value);

  if (rewards_service_) {
    rewards_service_->SetInlineTippingPlatformEnabled(key, value == "true");
  }
}

void RewardsDOMHandler::GetPendingContributions(
    const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->GetPendingContributions(base::Bind(
          &RewardsDOMHandler::OnGetPendingContributions,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::OnGetPendingContributions(
    ledger::type::PendingContributionInfoList list) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  auto contributions = std::make_unique<base::ListValue>();
  for (auto const& item : list) {
    auto contribution =
        std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
    contribution->SetKey("id", base::Value(static_cast<int>(item->id)));
    contribution->SetKey("publisherKey", base::Value(item->publisher_key));
    contribution->SetKey("status",
        base::Value(static_cast<int>(item->status)));
    contribution->SetKey("name", base::Value(item->name));
    contribution->SetKey("provider", base::Value(item->provider));
    contribution->SetKey("url", base::Value(item->url));
    contribution->SetKey("favIcon", base::Value(item->favicon_url));
    contribution->SetKey("amount", base::Value(item->amount));
    contribution->SetKey("addedDate",
        base::Value(std::to_string(item->added_date)));
    contribution->SetKey("type", base::Value(static_cast<int>(item->type)));
    contribution->SetKey("viewingId", base::Value(item->viewing_id));
    contribution->SetKey("expirationDate",
        base::Value(std::to_string(item->expiration_date)));
    contributions->Append(std::move(contribution));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.pendingContributions",
      *contributions);
}

void RewardsDOMHandler::RemovePendingContribution(
    const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  if (!rewards_service_) {
    return;
  }

  const uint64_t id = args->GetList()[0].GetInt();
  rewards_service_->RemovePendingContribution(id);
}

void RewardsDOMHandler::RemoveAllPendingContributions(
    const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->RemoveAllPendingContributions();
  }
}

void RewardsDOMHandler::OnPendingContributionRemoved(
    brave_rewards::RewardsService* rewards_service,
    const ledger::type::Result result) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.onRemovePendingContribution",
        base::Value(static_cast<int>(result)));
  }
}

void RewardsDOMHandler::OnFetchBalance(
    const ledger::type::Result result,
    ledger::type::BalancePtr balance) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value balance_value(base::Value::Type::DICTIONARY);

  if (balance) {
    balance_value.SetDoubleKey("total", balance->total);

    if (result == ledger::type::Result::LEDGER_OK) {
      base::Value wallets(base::Value::Type::DICTIONARY);
      for (auto const& wallet : balance->wallets) {
        wallets.SetDoubleKey(wallet.first, wallet.second);
      }
      balance_value.SetKey("wallets", std::move(wallets));
    }
  } else {
    balance_value.SetDoubleKey("total", 0.0);
  }

  base::DictionaryValue data;
  data.SetIntKey("status", static_cast<int>(result));
  data.SetKey("balance", std::move(balance_value));
  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.balance", data);
}

void RewardsDOMHandler::FetchBalance(const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->FetchBalance(base::BindOnce(
          &RewardsDOMHandler::OnFetchBalance,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::GetExternalWallet(const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  if (!rewards_service_) {
    return;
  }

  const std::string wallet_type = args->GetList()[0].GetString();
  rewards_service_->GetExternalWallet(
      wallet_type,
      base::BindOnce(&RewardsDOMHandler::OnGetExternalWallet,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetExternalWallet(
    const ledger::type::Result result,
    ledger::type::ExternalWalletPtr wallet) {
  if (web_ui()->CanCallJavascript()) {
    base::Value data(base::Value::Type::DICTIONARY);

    data.SetIntKey("result", static_cast<int>(result));
    base::Value wallet_dict(base::Value::Type::DICTIONARY);

    if (wallet) {
      wallet_dict.SetStringKey("token", wallet->token);
      wallet_dict.SetStringKey("address", wallet->address);
      wallet_dict.SetIntKey("status", static_cast<int>(wallet->status));
      wallet_dict.SetStringKey("type", wallet->type);
      wallet_dict.SetStringKey("verifyUrl", wallet->verify_url);
      wallet_dict.SetStringKey("addUrl", wallet->add_url);
      wallet_dict.SetStringKey("withdrawUrl", wallet->withdraw_url);
      wallet_dict.SetStringKey("userName", wallet->user_name);
      wallet_dict.SetStringKey("accountUrl", wallet->account_url);
      wallet_dict.SetStringKey("loginUrl", wallet->login_url);
    }

    data.SetKey("wallet", std::move(wallet_dict));

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.externalWallet",
                                           data);
  }
}

void RewardsDOMHandler::OnProcessRewardsPageUrl(
    const ledger::type::Result result,
    const std::string& wallet_type,
    const std::string& action,
    const std::map<std::string, std::string>& args) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("result", static_cast<int>(result));
  data.SetStringKey("walletType", wallet_type);
  data.SetStringKey("action", action);

  base::Value new_args(base::Value::Type::DICTIONARY);
  for (auto const& arg : args) {
    new_args.SetStringKey(arg.first, arg.second);
  }
  data.SetKey("args", std::move(new_args));

  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.processRewardsPageUrl",
                                         data);
}

void RewardsDOMHandler::ProcessRewardsPageUrl(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  if (!rewards_service_) {
    return;
  }

  const std::string path = args->GetList()[0].GetString();
  const std::string query = args->GetList()[1].GetString();
  rewards_service_->ProcessRewardsPageUrl(
      path,
      query,
      base::BindOnce(&RewardsDOMHandler::OnProcessRewardsPageUrl,
                     weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::DisconnectWallet(const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  if (!rewards_service_) {
    return;
  }

  const std::string wallet_type = args->GetList()[0].GetString();
  rewards_service_->DisconnectWallet(wallet_type);
}

void RewardsDOMHandler::OnDisconnectWallet(
      brave_rewards::RewardsService* rewards_service,
      const ledger::type::Result result,
      const std::string& wallet_type) {
  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("result", static_cast<int>(result));
  data.SetStringKey("walletType", wallet_type);

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.disconnectWallet",
      data);
}

void RewardsDOMHandler::OnAdsEnabled(
    brave_rewards::RewardsService* rewards_service,
    bool ads_enabled) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::ListValue* emptyArgs = nullptr;
  GetAdsData(emptyArgs);
}

void RewardsDOMHandler::OnlyAnonWallet(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }

  const bool allow = rewards_service_->OnlyAnonWallet();

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.onlyAnonWallet",
      base::Value(allow));
}

void RewardsDOMHandler::OnUnblindedTokensReady(
    brave_rewards::RewardsService* rewards_service) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.unblindedTokensReady");
}

void RewardsDOMHandler::ReconcileStampReset() {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.reconcileStampReset");
}

void RewardsDOMHandler::OnGetBalanceReport(
    const uint32_t month,
    const uint32_t year,
    const ledger::type::Result result,
    ledger::type::BalanceReportInfoPtr report) {
  if (!web_ui()->CanCallJavascript() || !report) {
    return;
  }

  base::Value report_base(base::Value::Type::DICTIONARY);
  report_base.SetDoubleKey("grant", report->grants);
  report_base.SetDoubleKey("ads", report->earning_from_ads);
  report_base.SetDoubleKey("contribute", report->auto_contribute);
  report_base.SetDoubleKey("monthly", report->recurring_donation);
  report_base.SetDoubleKey("tips", report->one_time_donation);

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("month", month);
  data.SetIntKey("year", year);
  data.SetKey("report", std::move(report_base));

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.balanceReport",
      data);
}

void RewardsDOMHandler::GetBalanceReport(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  if (!rewards_service_) {
    return;
  }

  const uint32_t month = args->GetList()[0].GetInt();
  const uint32_t year = args->GetList()[1].GetInt();
  rewards_service_->GetBalanceReport(
      month,
      year,
      base::BindOnce(&RewardsDOMHandler::OnGetBalanceReport,
                     weak_factory_.GetWeakPtr(),
                     month,
                     year));
}

void RewardsDOMHandler::OnGetMonthlyReport(
    const uint32_t month,
    const uint32_t year,
    ledger::type::MonthlyReportInfoPtr report) {
  if (!web_ui()->CanCallJavascript() || !report) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("month", month);
  data.SetIntKey("year", year);

  base::Value balance_report(base::Value::Type::DICTIONARY);
  balance_report.SetDoubleKey("grant", report->balance->grants);
  balance_report.SetDoubleKey("ads", report->balance->earning_from_ads);
  balance_report.SetDoubleKey("contribute", report->balance->auto_contribute);
  balance_report.SetDoubleKey("monthly", report->balance->recurring_donation);
  balance_report.SetDoubleKey("tips", report->balance->one_time_donation);

  base::Value transactions(base::Value::Type::LIST);
  for (const auto& item : report->transactions) {
    base::Value transaction_report(base::Value::Type::DICTIONARY);
    transaction_report.SetDoubleKey("amount", item->amount);
    transaction_report.SetIntKey("type", static_cast<int>(item->type));
    transaction_report.SetIntKey(
        "processor",
        static_cast<int>(item->processor));
    transaction_report.SetIntKey("created_at", item->created_at);

    transactions.Append(std::move(transaction_report));
  }

  base::Value contributions(base::Value::Type::LIST);
  for (const auto& contribution : report->contributions) {
    base::Value publishers(base::Value::Type::LIST);
    for (const auto& item : contribution->publishers) {
      base::Value publisher(base::Value::Type::DICTIONARY);
      publisher.SetStringKey("id", item->id);
      publisher.SetDoubleKey("percentage", item->percent);
      publisher.SetDoubleKey("weight", item->weight);
      publisher.SetStringKey("publisherKey", item->id);
      publisher.SetIntKey("status", static_cast<int>(item->status));
      publisher.SetStringKey("name", item->name);
      publisher.SetStringKey("provider", item->provider);
      publisher.SetStringKey("url", item->url);
      publisher.SetStringKey("favIcon", item->favicon_url);
      publishers.Append(std::move(publisher));
    }

    base::Value contribution_report(base::Value::Type::DICTIONARY);
    contribution_report.SetDoubleKey("amount", contribution->amount);
    contribution_report.SetIntKey("type", static_cast<int>(contribution->type));
    contribution_report.SetIntKey(
        "processor",
        static_cast<int>(contribution->processor));
    contribution_report.SetIntKey("created_at", contribution->created_at);
    contribution_report.SetKey("publishers", std::move(publishers));
    contributions.Append(std::move(contribution_report));
  }

  base::Value report_base(base::Value::Type::DICTIONARY);
  report_base.SetKey("balance", std::move(balance_report));
  report_base.SetKey("transactions", std::move(transactions));
  report_base.SetKey("contributions", std::move(contributions));

  data.SetKey("report", std::move(report_base));

  web_ui()->CallJavascriptFunctionUnsafe(
    "brave_rewards.monthlyReport",
    data);
}

void RewardsDOMHandler::GetMonthlyReport(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  if (!rewards_service_) {
    return;
  }

  const uint32_t month = args->GetList()[0].GetInt();
  const uint32_t year = args->GetList()[1].GetInt();

  rewards_service_->GetMonthlyReport(
      month,
      year,
      base::BindOnce(&RewardsDOMHandler::OnGetMonthlyReport,
          weak_factory_.GetWeakPtr(),
          month,
          year));
}

void RewardsDOMHandler::OnGetAllMonthlyReportIds(
    const std::vector<std::string>& ids) {
  base::Value list(base::Value::Type::LIST);
  for (const auto& item : ids) {
    list.Append(base::Value(item));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.monthlyReportIds",
      list);
}

void RewardsDOMHandler::GetAllMonthlyReportIds(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->GetAllMonthlyReportIds(
      base::BindOnce(&RewardsDOMHandler::OnGetAllMonthlyReportIds,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::GetCountryCode(const base::ListValue* args) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.countryCode", base::Value(country_code));
}

void RewardsDOMHandler::CompleteReset(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->CompleteReset(base::DoNothing());
}

void RewardsDOMHandler::OnCompleteReset(const bool success) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.completeReset", base::Value(success));
}

}  // namespace

BraveRewardsPageUI::BraveRewardsPageUI(content::WebUI* web_ui,
                                       const std::string& name)
    : BasicUI(web_ui,
              name,
#if defined(BRAVE_CHROMIUM_BUILD)
              kBraveRewardsPageGenerated,
              kBraveRewardsPageGeneratedSize,
#else
              kBraveRewardsSettingsGenerated,
              kBraveRewardsSettingsGeneratedSize,
#endif
#if defined(OS_ANDROID)
              IDR_BRAVE_REWARDS_ANDROID_PAGE_HTML) {
#else
              IDR_BRAVE_REWARDS_PAGE_HTML) {
#endif
  auto handler_owner = std::make_unique<RewardsDOMHandler>();
  RewardsDOMHandler * handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveRewardsPageUI::~BraveRewardsPageUI() {}
