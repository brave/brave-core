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

#include "base/base64.h"
#include "base/i18n/time_formatting.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_page_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/bindings_policy.h"
#if defined(OS_ANDROID)
#include "content/public/browser/url_data_source.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#endif

using content::WebUIMessageHandler;

namespace {

// The handler for Javascript messages for Brave about: pages
class RewardsDOMHandler : public WebUIMessageHandler,
    public brave_rewards::RewardsNotificationServiceObserver,
    public brave_rewards::RewardsServiceObserver {
 public:
  RewardsDOMHandler();
  ~RewardsDOMHandler() override;

  void Init();

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void GetAllBalanceReports();
  void HandleCreateWalletRequested(const base::ListValue* args);
  void GetWalletProperties(const base::ListValue* args);
  void GetGrants(const base::ListValue* args);
  void GetGrantCaptcha(const base::ListValue* args);
  void GetWalletPassphrase(const base::ListValue* args);
  void RecoverWallet(const base::ListValue* args);
  void SolveGrantCaptcha(const base::ListValue* args);
  void GetReconcileStamp(const base::ListValue* args);
  void SaveSetting(const base::ListValue* args);
  void UpdateAdsRewards(const base::ListValue* args);
  void OnContentSiteList(
      std::unique_ptr<brave_rewards::ContentSiteList>,
      uint32_t record);
  void OnExcludedSiteList(
      std::unique_ptr<brave_rewards::ContentSiteList>,
      uint32_t record);
  void OnGetAllBalanceReports(
      const std::map<std::string, brave_rewards::BalanceReport>& reports);
  void GetBalanceReports(const base::ListValue* args);
  void ExcludePublisher(const base::ListValue* args);
  void RestorePublishers(const base::ListValue* args);
  void RestorePublisher(const base::ListValue* args);
  void WalletExists(const base::ListValue* args);
  void GetContributionAmount(const base::ListValue* args);
  void RemoveRecurringTip(const base::ListValue* args);
  void GetRecurringTips(const base::ListValue* args);
  void GetOneTimeTips(const base::ListValue* args);
  void GetContributionList(const base::ListValue* args);
  void CheckImported(const base::ListValue* args);
  void GetAdsData(const base::ListValue* args);
  void GetAdsHistory(const base::ListValue* args);
  void OnGetAdsHistory(const base::ListValue& ads_history);
  void ToggleAdThumbUp(const base::ListValue* args);
  void OnToggleAdThumbUp(const std::string& id, int action);
  void ToggleAdThumbDown(const base::ListValue* args);
  void OnToggleAdThumbDown(const std::string& id, int action);
  void ToggleAdOptInAction(const base::ListValue* args);
  void OnToggleAdOptInAction(const std::string& category, int action);
  void ToggleAdOptOutAction(const base::ListValue* args);
  void OnToggleAdOptOutAction(const std::string& category, int action);
  void ToggleSaveAd(const base::ListValue* args);
  void OnToggleSaveAd(const std::string& id, bool saved);
  void ToggleFlagAd(const base::ListValue* args);
  void OnToggleFlagAd(const std::string& id, bool flagged);
  void SaveAdsSetting(const base::ListValue* args);
  void SetBackupCompleted(const base::ListValue* args);
  void OnGetWalletPassphrase(const std::string& pass);
  void OnGetContributionAmount(double amount);
  void OnGetAutoContributeProps(
      int error_code,
      std::unique_ptr<brave_rewards::WalletProperties> wallet_properties,
      std::unique_ptr<brave_rewards::AutoContributeProps> auto_contri_props);
  void OnGetReconcileStamp(uint64_t reconcile_stamp);
  void OnAutoContributePropsReady(
      std::unique_ptr<brave_rewards::AutoContributeProps> auto_contri_props);
  void OnIsWalletCreated(bool created);
  void GetPendingContributionsTotal(const base::ListValue* args);
  void OnGetPendingContributionsTotal(double amount);
  void OnContentSiteUpdated(
      brave_rewards::RewardsService* rewards_service) override;
  void GetTransactionHistory(const base::ListValue* args);
  void GetRewardsMainEnabled(const base::ListValue* args);
  void OnGetRewardsMainEnabled(bool enabled);
  void GetExcludedSites(const base::ListValue* args);
  void OnAutoContributePropsReadyExcluded(
      std::unique_ptr<brave_rewards::AutoContributeProps> auto_contri_props);

  void OnTransactionHistory(
      double estimated_pending_rewards,
      uint64_t next_payment_date_in_seconds,
      uint64_t ad_notifications_received_this_month);

  void OnGetRecurringTips(
    std::unique_ptr<brave_rewards::ContentSiteList> list);

  void OnGetOneTimeTips(
    std::unique_ptr<brave_rewards::ContentSiteList> list);

  void SetInlineTipSetting(const base::ListValue* args);

  void GetPendingContributions(const base::ListValue* args);
  void OnGetPendingContributions(
    std::unique_ptr<brave_rewards::PendingContributionInfoList> list);
  void RemovePendingContribution(const base::ListValue* args);
  void RemoveAllPendingContributions(const base::ListValue* args);
  void FetchBalance(const base::ListValue* args);
  void OnFetchBalance(
    int32_t result,
    std::unique_ptr<brave_rewards::Balance> balance);

  void GetExternalWallet(const base::ListValue* args);
  void OnGetExternalWallet(
      int32_t result,
      std::unique_ptr<brave_rewards::ExternalWallet> wallet);
  void ProcessRewardsPageUrl(const base::ListValue* args);

  void OnProcessRewardsPageUrl(
    int32_t result,
    const std::string& wallet_type,
    const std::string& action,
    const std::map<std::string, std::string>& args);

  void DisconnectWallet(const base::ListValue* args);

  void OnlyAnonWallet(const base::ListValue* args);

  // RewardsServiceObserver implementation
  void OnWalletInitialized(brave_rewards::RewardsService* rewards_service,
                       int32_t result) override;
  void OnWalletProperties(
      brave_rewards::RewardsService* rewards_service,
      int error_code,
      std::unique_ptr<brave_rewards::WalletProperties>
      wallet_properties) override;
  void OnGrant(brave_rewards::RewardsService* rewards_service,
                   unsigned int result,
                   brave_rewards::Grant grant) override;
  void OnGrantCaptcha(brave_rewards::RewardsService* rewards_service,
                          std::string image, std::string hint) override;
  void OnRecoverWallet(brave_rewards::RewardsService* rewards_service,
                       unsigned int result,
                       double balance,
                       std::vector<brave_rewards::Grant> grants) override;
  void OnGrantFinish(brave_rewards::RewardsService* rewards_service,
                       unsigned int result,
                       brave_rewards::Grant grant) override;
  void OnExcludedSitesChanged(brave_rewards::RewardsService* rewards_service,
                              std::string publisher_id,
                              bool excluded) override;
  void OnReconcileComplete(brave_rewards::RewardsService* rewards_service,
                           unsigned int result,
                           const std::string& viewing_id,
                           const std::string& probi,
                           const int32_t category) override;
  void OnPendingContributionSaved(
      brave_rewards::RewardsService* rewards_service,
      int result) override;

  void OnRewardsMainEnabled(
      brave_rewards::RewardsService* rewards_service,
      bool rewards_main_enabled) override;

  void OnPublisherListNormalized(
      brave_rewards::RewardsService* rewards_service,
      const brave_rewards::ContentSiteList& list) override;

  void OnTransactionHistoryChanged(
      brave_rewards::RewardsService* rewards_service) override;

  void OnRecurringTipSaved(brave_rewards::RewardsService* rewards_service,
                           bool success) override;

  void OnRecurringTipRemoved(brave_rewards::RewardsService* rewards_service,
                             bool success) override;

  void OnContributionSaved(
    brave_rewards::RewardsService* rewards_service,
    bool success,
    int category) override;

  void OnPendingContributionRemoved(
      brave_rewards::RewardsService* rewards_service,
      int32_t result) override;

  void OnDisconnectWallet(
      brave_rewards::RewardsService* rewards_service,
      int32_t result,
      const std::string& wallet_type) override;

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

  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED
  brave_ads::AdsService* ads_service_;
  base::WeakPtrFactory<RewardsDOMHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RewardsDOMHandler);
};

RewardsDOMHandler::RewardsDOMHandler() : weak_factory_(this) {}

RewardsDOMHandler::~RewardsDOMHandler() {
  if (rewards_service_)
    rewards_service_->RemoveObserver(this);
}

void RewardsDOMHandler::RegisterMessages() {

#if defined(OS_ANDROID)
  // Create our favicon data source.
  Profile* profile = Profile::FromWebUI(web_ui());
  content::URLDataSource::Add(profile,
                              std::make_unique<FaviconSource>(profile));
#endif

  web_ui()->RegisterMessageCallback("brave_rewards.createWalletRequested",
      base::BindRepeating(&RewardsDOMHandler::HandleCreateWalletRequested,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getWalletProperties",
      base::BindRepeating(&RewardsDOMHandler::GetWalletProperties,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getGrants",
      base::BindRepeating(&RewardsDOMHandler::GetGrants,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getGrantCaptcha",
      base::BindRepeating(&RewardsDOMHandler::GetGrantCaptcha,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getWalletPassphrase",
      base::BindRepeating(&RewardsDOMHandler::GetWalletPassphrase,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.recoverWallet",
      base::BindRepeating(&RewardsDOMHandler::RecoverWallet,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.solveGrantCaptcha",
      base::BindRepeating(&RewardsDOMHandler::SolveGrantCaptcha,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getReconcileStamp",
      base::BindRepeating(&RewardsDOMHandler::GetReconcileStamp,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.saveSetting",
      base::BindRepeating(&RewardsDOMHandler::SaveSetting,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.updateAdsRewards",
      base::BindRepeating(&RewardsDOMHandler::UpdateAdsRewards,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getBalanceReports",
      base::BindRepeating(&RewardsDOMHandler::GetBalanceReports,
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
      base::BindRepeating(&RewardsDOMHandler::GetContributionAmount,
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
  web_ui()->RegisterMessageCallback("brave_rewards.checkImported",
      base::BindRepeating(&RewardsDOMHandler::CheckImported,
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
  web_ui()->RegisterMessageCallback("brave_rewards.setInlineTipSetting",
      base::BindRepeating(&RewardsDOMHandler::SetInlineTipSetting,
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
}

void RewardsDOMHandler::Init() {
  Profile* profile = Profile::FromWebUI(web_ui());
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  ads_service_ =
      brave_ads::AdsServiceFactory::GetForProfile(profile);

  if (rewards_service_)
    rewards_service_->AddObserver(this);
}

void RewardsDOMHandler::OnGetAllBalanceReports(
    const std::map<std::string, brave_rewards::BalanceReport>& reports) {
  if (web_ui()->CanCallJavascript()) {
    base::DictionaryValue newReports;
    if (!reports.empty()) {
      for (auto const& report : reports) {
        const brave_rewards::BalanceReport oldReport = report.second;
        auto newReport = std::make_unique<base::DictionaryValue>();
        newReport->SetString("grant", oldReport.grants);
        newReport->SetString("deposit", oldReport.deposits);
        newReport->SetString("ads", oldReport.earning_from_ads);
        newReport->SetString("contribute", oldReport.auto_contribute);
        newReport->SetString("donation", oldReport.recurring_donation);
        newReport->SetString("tips", oldReport.one_time_donation);
        newReport->SetString("total", oldReport.total);
        newReports.SetDictionary(report.first, std::move(newReport));
      }
    }

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.balanceReports",
        newReports);
  }
}

void RewardsDOMHandler::GetAllBalanceReports() {
  if (rewards_service_)
    rewards_service_->GetAllBalanceReports(
        base::Bind(&RewardsDOMHandler::OnGetAllBalanceReports,
          weak_factory_.GetWeakPtr()));
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

void RewardsDOMHandler::GetWalletProperties(const base::ListValue* args) {
  if (!rewards_service_)
    return;

  rewards_service_->FetchWalletProperties();
}

void RewardsDOMHandler::OnWalletInitialized(
    brave_rewards::RewardsService* rewards_service,
    int32_t result) {
  if (!web_ui()->CanCallJavascript())
    return;

  // ledger::Result::WALLET_CREATED
  if (result == 12) {
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletCreated");
  } else if (result != 3 && result != 0) {
    // Report back all errors except when ledger_state is missing
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletCreateFailed");
  }
}

void RewardsDOMHandler::OnGetAutoContributeProps(
    int error_code,
    std::unique_ptr<brave_rewards::WalletProperties> wallet_properties,
    std::unique_ptr<brave_rewards::AutoContributeProps> auto_contri_props) {
  if (web_ui()->CanCallJavascript()) {
    base::DictionaryValue values;
    values.SetBoolean("enabledContribute",
        auto_contri_props->enabled_contribute);
    values.SetInteger("contributionMinTime",
        auto_contri_props->contribution_min_time);
    values.SetInteger("contributionMinVisits",
        auto_contri_props->contribution_min_visits);
    values.SetBoolean("contributionNonVerified",
        auto_contri_props->contribution_non_verified);
    values.SetBoolean("contributionVideos",
        auto_contri_props->contribution_videos);

    base::DictionaryValue result;
    result.SetInteger("status", error_code);
    auto walletInfo = std::make_unique<base::DictionaryValue>();

    if (error_code == 0 && wallet_properties) {
      auto choices = std::make_unique<base::ListValue>();
      for (double const& choice : wallet_properties->parameters_choices) {
        choices->AppendDouble(choice);
      }
      walletInfo->SetList("choices", std::move(choices));

      auto grants = std::make_unique<base::ListValue>();
      for (auto const& item : wallet_properties->grants) {
        auto grant = std::make_unique<base::DictionaryValue>();
        grant->SetString("probi", item.probi);
        grant->SetInteger("expiryTime", item.expiryTime);
        grant->SetString("type", item.type);
        grants->Append(std::move(grant));
      }
      walletInfo->SetList("grants", std::move(grants));

      result.SetDouble("monthlyAmount", wallet_properties->monthly_amount);
    }

    // TODO(Nejc Zdovc): this needs to be moved out of this flow, because now we
    // set this values every minute
    web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.initAutoContributeSettings", values);

    result.SetDictionary("wallet", std::move(walletInfo));

    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.walletProperties", result);
  }
}

void RewardsDOMHandler::OnWalletProperties(
    brave_rewards::RewardsService* rewards_service,
    int error_code,
    std::unique_ptr<brave_rewards::WalletProperties> wallet_properties) {
  rewards_service->GetAutoContributeProps(
      base::Bind(&RewardsDOMHandler::OnGetAutoContributeProps,
        weak_factory_.GetWeakPtr(), error_code,
        base::Passed(std::move(wallet_properties))));
}

void RewardsDOMHandler::OnGrant(
    brave_rewards::RewardsService* rewards_service,
    unsigned int result,
    brave_rewards::Grant grant) {
  if (web_ui()->CanCallJavascript()) {
    base::DictionaryValue newGrant;
    newGrant.SetInteger("status", result);
    newGrant.SetString("type", grant.type);
    newGrant.SetString("promotionId", grant.promotionId);

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.grant", newGrant);
  }
}

void RewardsDOMHandler::GetGrants(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  if (rewards_service_) {
    const std::string lang = args->GetList()[0].GetString();
    const std::string paymentId = args->GetList()[1].GetString();
    rewards_service_->FetchGrants(lang, paymentId);
  }
}

void RewardsDOMHandler::OnGrantCaptcha(
    brave_rewards::RewardsService* rewards_service,
    std::string image,
    std::string hint) {
  if (web_ui()->CanCallJavascript()) {
    std::string encoded_string;
    base::Base64Encode(image, &encoded_string);

    base::DictionaryValue captcha;
    captcha.SetString("image", std::move(encoded_string));
    captcha.SetString("hint", hint);

    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.grantCaptcha", captcha);
  }
}

void RewardsDOMHandler::GetGrantCaptcha(const base::ListValue* args) {
  CHECK_EQ(2U, args->GetSize());
  if (rewards_service_) {
#if defined(OS_ANDROID)
    // TODO(samartnik): we need different call from JS,
    // currently using this one to make sure it all work
    // As soon as @ryanml adds separate action for safetynet,
    // we will move that code
    rewards_service_->GetGrantViaSafetynetCheck();
#else
  if (rewards_service_) {
    const std::string promotion_id = args->GetList()[0].GetString();
    const std::string promotion_type = args->GetList()[1].GetString();
    rewards_service_->GetGrantCaptcha(promotion_id, promotion_type);
  }
#endif
  }
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
    const std::string passPhrase = args->GetList()[0].GetString();
    rewards_service_->RecoverWallet(passPhrase);
  }
}

void RewardsDOMHandler::OnRecoverWallet(
    brave_rewards::RewardsService* rewards_service,
    unsigned int result,
    double balance,
    std::vector<brave_rewards::Grant> grants) {
  GetAllBalanceReports();
  if (web_ui()->CanCallJavascript()) {
    base::DictionaryValue recover;
    recover.SetInteger("result", result);
    recover.SetDouble("balance", balance);

    auto newGrants = std::make_unique<base::ListValue>();
    for (auto const& item : grants) {
      auto grant = std::make_unique<base::DictionaryValue>();
      grant->SetString("probi", item.probi);
      grant->SetInteger("expiryTime", item.expiryTime);
      grant->SetString("type", item.type);
      newGrants->Append(std::move(grant));
    }
    recover.SetList("grants", std::move(newGrants));

    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.recoverWalletData", recover);
  }
}

void RewardsDOMHandler::SolveGrantCaptcha(const base::ListValue *args) {
  CHECK_EQ(2U, args->GetSize());
  if (rewards_service_) {
    const std::string solution = args->GetList()[0].GetString();
    const std::string promotionId = args->GetList()[1].GetString();
    rewards_service_->SolveGrantCaptcha(solution, promotionId);
  }
}

void RewardsDOMHandler::OnGrantFinish(
    brave_rewards::RewardsService* rewards_service,
    unsigned int result,
    brave_rewards::Grant grant) {
  if (web_ui()->CanCallJavascript()) {
    base::DictionaryValue finish;
    finish.SetInteger("status", result);
    finish.SetInteger("expiryTime", grant.expiryTime);
    finish.SetString("probi", grant.probi);
    finish.SetString("type", grant.type);

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.grantFinish", finish);
    GetAllBalanceReports();
  }
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
    std::unique_ptr<brave_rewards::AutoContributeProps> props) {
  rewards_service_->GetContentSiteList(
      0,
      0,
      props->contribution_min_time,
      props->reconcile_stamp,
      props->contribution_non_verified,
      props->contribution_min_visits,
      false,
      base::Bind(&RewardsDOMHandler::OnContentSiteList,
                 weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnAutoContributePropsReadyExcluded(
    std::unique_ptr<brave_rewards::AutoContributeProps> props) {
  rewards_service_->GetContentSiteList(
      0,
      0,
      props->contribution_min_time,
      props->reconcile_stamp,
      props->contribution_non_verified,
      props->contribution_min_visits,
      true,
      base::Bind(&RewardsDOMHandler::OnExcludedSiteList,
                 weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnContentSiteUpdated(
    brave_rewards::RewardsService* rewards_service) {
  rewards_service_->GetAutoContributeProps(
      base::Bind(&RewardsDOMHandler::OnAutoContributePropsReady,
        weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::GetExcludedSites(const base::ListValue* args) {
  rewards_service_->GetAutoContributeProps(
      base::Bind(&RewardsDOMHandler::OnAutoContributePropsReadyExcluded,
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
        notification) {}

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
      rewards_service_->SetUserChangedContribution();
      rewards_service_->SetContributionAmount(std::stod(value));
      GetAllBalanceReports();
    }

    if (key == "contributionMinTime") {
      rewards_service_->SetPublisherMinVisitTime(std::stoull(value));
    }

    if (key == "contributionMinVisits") {
      rewards_service_->SetPublisherMinVisits(std::stoul(value));
    }

    if (key == "contributionNonVerified") {
      rewards_service_->SetPublisherAllowNonVerified(value == "true");
    }

    if (key == "contributionVideos") {
      rewards_service_->SetPublisherAllowVideos(value == "true");
    }

    if (key == "enabledContribute") {
      rewards_service_->SetAutoContribute(value == "true");
    }
  }
}

void RewardsDOMHandler::UpdateAdsRewards(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->UpdateAdsRewards();
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

  rewards_service_->RestorePublishersUI();
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
    std::unique_ptr<brave_rewards::ContentSiteList> list,
    uint32_t record) {
  if (web_ui()->CanCallJavascript()) {
    auto publishers = std::make_unique<base::ListValue>();
    for (auto const& item : *list) {
      auto publisher = std::make_unique<base::DictionaryValue>();
      publisher->SetString("id", item.id);
      publisher->SetDouble("percentage", item.percentage);
      publisher->SetString("publisherKey", item.id);
      publisher->SetInteger("status", item.status);
      publisher->SetInteger("excluded", item.excluded);
      publisher->SetString("name", item.name);
      publisher->SetString("provider", item.provider);
      publisher->SetString("url", item.url);
      publisher->SetString("favIcon", item.favicon_url);
      publishers->Append(std::move(publisher));
    }

    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.contributeList", *publishers);
  }
}

void RewardsDOMHandler::OnExcludedSiteList(
    std::unique_ptr<brave_rewards::ContentSiteList> list,
    uint32_t record) {
  if (web_ui()->CanCallJavascript()) {
    auto publishers = std::make_unique<base::ListValue>();
    for (auto const& item : *list) {
      auto publisher = std::make_unique<base::DictionaryValue>();
      publisher->SetString("id", item.id);
      publisher->SetInteger("status", item.status);
      publisher->SetString("name", item.name);
      publisher->SetString("provider", item.provider);
      publisher->SetString("url", item.url);
      publisher->SetString("favIcon", item.favicon_url);
      publishers->Append(std::move(publisher));
    }

    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.excludedList", *publishers);
  }
}

void RewardsDOMHandler::GetBalanceReports(const base::ListValue* args) {
  GetAllBalanceReports();
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

void RewardsDOMHandler::GetContributionAmount(const base::ListValue* args) {
  if (rewards_service_)
    rewards_service_->GetContributionAmount(
        base::Bind(&RewardsDOMHandler::OnGetContributionAmount,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnReconcileComplete(
    brave_rewards::RewardsService* rewards_service,
    unsigned int result,
    const std::string& viewing_id,
    const std::string& probi,
    const int32_t category) {
  if (web_ui()->CanCallJavascript()) {
    base::DictionaryValue complete;
    complete.SetKey("result", base::Value(static_cast<int>(result)));
    complete.SetKey("category", base::Value(category));

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.reconcileComplete",
                                           complete);
  }
  GetAllBalanceReports();
}

void RewardsDOMHandler::RemoveRecurringTip(const base::ListValue *args) {
  CHECK_EQ(1U, args->GetSize());
  if (rewards_service_) {
    const std::string publisherKey = args->GetList()[0].GetString();
    rewards_service_->RemoveRecurringTipUI(publisherKey);
  }
}

void RewardsDOMHandler::GetRecurringTips(
    const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->GetRecurringTipsUI(base::BindOnce(
          &RewardsDOMHandler::OnGetRecurringTips,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::OnGetRecurringTips(
    std::unique_ptr<brave_rewards::ContentSiteList> list) {
  if (web_ui()->CanCallJavascript()) {
    auto publishers = std::make_unique<base::ListValue>();

    if (list) {
      for (auto const& item : *list) {
        auto publisher = std::make_unique<base::DictionaryValue>();
        publisher->SetString("id", item.id);
        publisher->SetDouble("percentage", item.percentage);
        publisher->SetString("publisherKey", item.id);
        publisher->SetInteger("status", item.status);
        publisher->SetInteger("excluded", item.excluded);
        publisher->SetString("name", item.name);
        publisher->SetString("provider", item.provider);
        publisher->SetString("url", item.url);
        publisher->SetString("favIcon", item.favicon_url);
        publisher->SetInteger("tipDate", 0);
        publishers->Append(std::move(publisher));
      }
    }

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.recurringTips",
                                           *publishers);
  }
}

void RewardsDOMHandler::OnGetOneTimeTips(
    std::unique_ptr<brave_rewards::ContentSiteList> list) {
  if (web_ui()->CanCallJavascript()) {
    auto publishers = std::make_unique<base::ListValue>();

    if (list) {
      for (auto const& item : *list) {
        auto publisher = std::make_unique<base::DictionaryValue>();
        publisher->SetString("id", item.id);
        publisher->SetDouble("percentage", item.percentage);
        publisher->SetString("publisherKey", item.id);
        publisher->SetInteger("status", item.status);
        publisher->SetInteger("excluded", item.excluded);
        publisher->SetString("name", item.name);
        publisher->SetString("provider", item.provider);
        publisher->SetString("url", item.url);
        publisher->SetString("favIcon", item.favicon_url);
        publisher->SetInteger("tipDate", item.reconcile_stamp);
        publishers->Append(std::move(publisher));
      }
    }

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.currentTips",
                                           *publishers);
  }
}

void RewardsDOMHandler::GetOneTimeTips(const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->GetOneTimeTipsUI(base::BindOnce(
          &RewardsDOMHandler::OnGetOneTimeTips,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::GetContributionList(const base::ListValue *args) {
  if (rewards_service_) {
    OnContentSiteUpdated(rewards_service_);
  }
}

void RewardsDOMHandler::CheckImported(const base::ListValue *args) {
  if (web_ui()->CanCallJavascript() && rewards_service_) {
    bool imported = rewards_service_->CheckImported();
    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.imported", base::Value(imported));
  }
}

void RewardsDOMHandler::GetAdsData(const base::ListValue *args) {
  if (!ads_service_ || !web_ui()->CanCallJavascript()) {
    return;
  }

  base::DictionaryValue ads_data;

  auto is_supported_region = ads_service_->IsSupportedRegion();
  ads_data.SetBoolean("adsIsSupported", is_supported_region);

  auto is_enabled = ads_service_->IsEnabled();
  ads_data.SetBoolean("adsEnabled", is_enabled);

  auto ads_per_hour = ads_service_->GetAdsPerHour();
  ads_data.SetInteger("adsPerHour", ads_per_hour);

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

  ads_service_->GetAdsHistory(base::Bind(&RewardsDOMHandler::OnGetAdsHistory,
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

void RewardsDOMHandler::OnToggleAdThumbUp(const std::string& id, int action) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("uuid", base::Value(id));
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

void RewardsDOMHandler::OnToggleAdThumbDown(const std::string& id, int action) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("uuid", base::Value(id));
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

  const std::string id = args->GetList()[0].GetString();
  const std::string creative_set_id = args->GetList()[1].GetString();
  const bool saved = args->GetList()[2].GetBool();
  ads_service_->ToggleSaveAd(id, creative_set_id, saved,
                             base::BindOnce(&RewardsDOMHandler::OnToggleSaveAd,
                                            weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleSaveAd(const std::string& id, bool saved) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("uuid", base::Value(id));
  result.SetKey("saved", base::Value(saved));
  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.onToggleSaveAd",
                                         result);
}

void RewardsDOMHandler::ToggleFlagAd(const base::ListValue* args) {
  CHECK_EQ(3U, args->GetSize());
  if (!ads_service_) {
    return;
  }

  const std::string id = args->GetList()[0].GetString();
  const std::string creative_set_id = args->GetList()[1].GetString();
  const bool flagged = args->GetList()[2].GetBool();
  ads_service_->ToggleFlagAd(id, creative_set_id, flagged,
                             base::BindOnce(&RewardsDOMHandler::OnToggleFlagAd,
                                            weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnToggleFlagAd(const std::string& id, bool flagged) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value result(base::Value::Type::DICTIONARY);
  result.SetKey("uuid", base::Value(id));
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
    ads_service_->SetEnabled(value == "true");
  } else if (key == "adsPerHour") {
    ads_service_->SetAdsPerHour(std::stoull(value));
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
    rewards_service_->GetPendingContributionsTotalUI(base::Bind(
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
    int result) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.onPendingContributionSaved", base::Value(result));
  }
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
    const brave_rewards::ContentSiteList& list) {
  OnContentSiteList(std::make_unique<brave_rewards::ContentSiteList>(list), 0);
}

void RewardsDOMHandler::GetTransactionHistory(
    const base::ListValue* args) {
  rewards_service_->GetTransactionHistory(base::Bind(
      &RewardsDOMHandler::OnTransactionHistory,
      weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnTransactionHistory(
    double estimated_pending_rewards,
    uint64_t next_payment_date_in_seconds,
    uint64_t ad_notifications_received_this_month) {
  if (web_ui()->CanCallJavascript()) {
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
}

void RewardsDOMHandler::OnTransactionHistoryChanged(
    brave_rewards::RewardsService* rewards_service) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.transactionHistoryChanged");
  }
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

void RewardsDOMHandler::OnContributionSaved(
    brave_rewards::RewardsService* rewards_service,
    bool success,
    int category) {
  if (!web_ui()->CanCallJavascript()) {
     return;
  }

  base::DictionaryValue result;
  result.SetBoolean("success", success);
  result.SetInteger("category", category);

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards.onContributionSaved", result);
}

void RewardsDOMHandler::SetInlineTipSetting(const base::ListValue* args) {
  std::string key;
  args->GetString(0, &key);

  std::string value;
  args->GetString(1, &value);

  if (rewards_service_) {
    rewards_service_->SetInlineTipSetting(key, value == "true");
  }
}

void RewardsDOMHandler::GetPendingContributions(
    const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->GetPendingContributionsUI(base::Bind(
          &RewardsDOMHandler::OnGetPendingContributions,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::OnGetPendingContributions(
    std::unique_ptr<brave_rewards::PendingContributionInfoList> list) {
  if (web_ui()->CanCallJavascript()) {
    auto contributions = std::make_unique<base::ListValue>();
    for (auto const& item : *list) {
      auto contribution =
          std::make_unique<base::Value>(base::Value::Type::DICTIONARY);
      contribution->SetKey("publisherKey", base::Value(item.publisher_key));
      contribution->SetKey("status",
          base::Value(static_cast<int>(item.status)));
      contribution->SetKey("name", base::Value(item.name));
      contribution->SetKey("provider", base::Value(item.provider));
      contribution->SetKey("url", base::Value(item.url));
      contribution->SetKey("favIcon", base::Value(item.favicon_url));
      contribution->SetKey("amount", base::Value(item.amount));
      contribution->SetKey("addedDate",
          base::Value(std::to_string(item.added_date)));
      contribution->SetKey("category", base::Value(item.category));
      contribution->SetKey("viewingId", base::Value(item.viewing_id));
      contribution->SetKey("expirationDate",
          base::Value(std::to_string(item.expiration_date)));
      contributions->Append(std::move(contribution));
    }

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.pendingContributions",
                                           *contributions);
  }
}

void RewardsDOMHandler::RemovePendingContribution(
    const base::ListValue* args) {
  CHECK_EQ(3U, args->GetSize());
  if (!rewards_service_) {
    return;
  }

  const std::string publisher_key = args->GetList()[0].GetString();
  const std::string viewing_id = args->GetList()[1].GetString();
  const std::string temp = args->GetList()[2].GetString();
  uint64_t added_date = std::stoull(temp);
  rewards_service_->RemovePendingContributionUI(
      publisher_key,
      viewing_id,
      added_date);
}

void RewardsDOMHandler::RemoveAllPendingContributions(
    const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->RemoveAllPendingContributionsUI();
  }
}

void RewardsDOMHandler::OnPendingContributionRemoved(
    brave_rewards::RewardsService* rewards_service,
    int32_t result) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.onRemovePendingContribution", base::Value(result));
  }
}

void RewardsDOMHandler::OnFetchBalance(
    int32_t result,
    std::unique_ptr<brave_rewards::Balance> balance) {
  if (web_ui()->CanCallJavascript()) {
    base::DictionaryValue data;
    data.SetIntKey("status", result);
    base::Value balance_value(base::Value::Type::DICTIONARY);

    if (result == 0 && balance) {
      balance_value.SetDoubleKey("total", balance->total);

      base::Value rates(base::Value::Type::DICTIONARY);
      for (auto const& rate : balance->rates) {
        rates.SetDoubleKey(rate.first, rate.second);
      }
      balance_value.SetKey("rates", std::move(rates));

      base::Value wallets(base::Value::Type::DICTIONARY);
      for (auto const& wallet : balance->wallets) {
        wallets.SetDoubleKey(wallet.first, wallet.second);
      }
      balance_value.SetKey("wallets", std::move(wallets));

      data.SetKey("balance", std::move(balance_value));
    }

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.balance", data);
  }
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
    int32_t result,
    std::unique_ptr<brave_rewards::ExternalWallet> wallet) {
  if (web_ui()->CanCallJavascript()) {
    base::Value data(base::Value::Type::DICTIONARY);

    data.SetIntKey("result", result);
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
    }

    data.SetKey("wallet", std::move(wallet_dict));

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.externalWallet",
                                           data);
  }
}

void RewardsDOMHandler::OnProcessRewardsPageUrl(
    int32_t result,
    const std::string& wallet_type,
    const std::string& action,
    const std::map<std::string, std::string>& args) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);

  data.SetIntKey("result", result);
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
      int32_t result,
      const std::string& wallet_type) {
  base::Value data(base::Value::Type::DICTIONARY);

  data.SetIntKey("result", result);
  data.SetStringKey("walletType", wallet_type);

  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.disconnectWallet",
                                         data);
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

}  // namespace

BraveRewardsPageUI::BraveRewardsPageUI(
    content::WebUI* web_ui,
    const std::string& name)
    : BasicUI(web_ui, name,
#if !defined(OS_ANDROID)
    kBraveRewardsPageGenerated,
    kBraveRewardsPageGeneratedSize,
#else
    kBraveRewardsSettingsGenerated, kBraveRewardsSettingsGeneratedSize,
#endif
    IDR_BRAVE_REWARDS_PAGE_HTML) {
  auto handler_owner = std::make_unique<RewardsDOMHandler>();
  RewardsDOMHandler * handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveRewardsPageUI::~BraveRewardsPageUI() {}
