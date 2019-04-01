/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_ui.h"

#include <utility>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include "base/base64.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/bindings_policy.h"
#if !defined(OS_ANDROID)
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_generated_map.h"
#else
#include "components/brave_rewards/settings/resources/grit/brave_rewards_settings_generated_map.h"
#include "components/grit/components_resources.h"
#include "components/grit/components_scaled_resources.h"
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
  void GetAddresses(const base::ListValue* args);
  void SaveSetting(const base::ListValue* args);
  void OnContentSiteList(
      std::unique_ptr<brave_rewards::ContentSiteList>,
      uint32_t record);
  void OnGetAllBalanceReports(
      const std::map<std::string, brave_rewards::BalanceReport>& reports);
  void GetBalanceReports(const base::ListValue* args);
  void ExcludePublisher(const base::ListValue* args);
  void RestorePublishers(const base::ListValue* args);
  void WalletExists(const base::ListValue* args);
  void GetContributionAmount(const base::ListValue* args);
  void RemoveRecurringTip(const base::ListValue* args);
  void GetRecurringTips(const base::ListValue* args);
  void GetOneTimeTips(const base::ListValue* args);
  void GetContributionList(const base::ListValue* args);
  void CheckImported(const base::ListValue* args);
  void GetAdsData(const base::ListValue* args);
  void SaveAdsSetting(const base::ListValue* args);
  void SetBackupCompleted(const base::ListValue* args);
  void OnGetWalletPassphrase(const std::string& pass);
  void OnGetContributionAmount(double amount);
  void OnGetAddresses(const std::string func_name,
                      const std::map<std::string, std::string>& addresses);
  void OnGetExcludedPublishersNumber(uint32_t num);
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
  void GetAddressesForPaymentId(const base::ListValue* args);
  void GetConfirmationsHistory(const base::ListValue* args);
  void GetRewardsMainEnabled(const base::ListValue* args);
  void OnGetRewardsMainEnabled(bool enabled);
  void OnAdsIsSupportedRegion(bool is_supported);

  void GetExcludedPublishersNumber(const base::ListValue* args);
  void AdsIsSupportedRegion(const base::ListValue* args);

  void OnConfirmationsHistory(int total_viewed, double estimated_earnings);

  void OnGetRecurringTips(
    std::unique_ptr<brave_rewards::ContentSiteList> list);

  // RewardsServiceObserver implementation
  void OnWalletInitialized(brave_rewards::RewardsService* rewards_service,
                       int result) override;
  void OnWalletProperties(
      brave_rewards::RewardsService* rewards_service,
      int error_code,
      std::unique_ptr<brave_rewards::WalletProperties>
      wallet_properties) override;
  void OnGrant(brave_rewards::RewardsService* rewards_service,
                   unsigned int error_code,
                   brave_rewards::Grant result) override;
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
                           const std::string& category,
                           const std::string& probi) override;
  void OnCurrentTips(brave_rewards::RewardsService* rewards_service,
                     brave_rewards::ContentSiteList) override;

  void OnPendingContributionSaved(
      brave_rewards::RewardsService* rewards_service,
      int result) override;

  void OnRewardsMainEnabled(
      brave_rewards::RewardsService* rewards_service,
      bool rewards_main_enabled) override;

  void OnPublisherListNormalized(
      brave_rewards::RewardsService* rewards_service,
      brave_rewards::ContentSiteList list) override;

  void OnConfirmationsHistoryChanged(
      brave_rewards::RewardsService* rewards_service) override;

  void OnRecurringTipSaved(brave_rewards::RewardsService* rewards_service,
                           bool success) override;

  void OnRecurringTipRemoved(brave_rewards::RewardsService* rewards_service,
                             bool success) override;

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
  web_ui()->RegisterMessageCallback("brave_rewards.getAddresses",
      base::BindRepeating(&RewardsDOMHandler::GetAddresses,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.saveSetting",
      base::BindRepeating(&RewardsDOMHandler::SaveSetting,
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
  web_ui()->RegisterMessageCallback("brave_rewards.getAddressesForPaymentId",
      base::BindRepeating(&RewardsDOMHandler::GetAddressesForPaymentId,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getConfirmationsHistory",
      base::BindRepeating(&RewardsDOMHandler::GetConfirmationsHistory,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getRewardsMainEnabled",
      base::BindRepeating(&RewardsDOMHandler::GetRewardsMainEnabled,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getExcludedPublishersNumber",
      base::BindRepeating(&RewardsDOMHandler::GetExcludedPublishersNumber,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getAdsIsSupportedRegion",
      base::BindRepeating(&RewardsDOMHandler::AdsIsSupportedRegion,
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
        newReport->SetString("opening", oldReport.opening_balance);
        newReport->SetString("closing", oldReport.closing_balance);
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

  rewards_service_->CreateWallet();
}

void RewardsDOMHandler::GetWalletProperties(const base::ListValue* args) {
  if (!rewards_service_)
    return;

  rewards_service_->FetchWalletProperties();
}

void RewardsDOMHandler::OnWalletInitialized(
    brave_rewards::RewardsService* rewards_service,
    int result) {
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

    auto ui_values = std::make_unique<base::DictionaryValue>();

    base::DictionaryValue result;
    result.SetInteger("status", error_code);
    auto walletInfo = std::make_unique<base::DictionaryValue>();

    if (error_code == 0 && wallet_properties) {
      walletInfo->SetDouble("balance", wallet_properties->balance);
      walletInfo->SetString("probi", wallet_properties->probi);
      ui_values->SetBoolean("emptyWallet", (wallet_properties->balance == 0));

      auto rates = std::make_unique<base::DictionaryValue>();
      for (auto const& rate : wallet_properties->rates) {
        rates->SetDouble(rate.first, rate.second);
      }
      walletInfo->SetDictionary("rates", std::move(rates));

      auto choices = std::make_unique<base::ListValue>();
      for (double const& choice : wallet_properties->parameters_choices) {
        choices->AppendDouble(choice);
      }
      walletInfo->SetList("choices", std::move(choices));

      auto range = std::make_unique<base::ListValue>();
      for (double const& value : wallet_properties->parameters_range) {
        range->AppendDouble(value);
      }
      walletInfo->SetList("range", std::move(range));

      auto grants = std::make_unique<base::ListValue>();
      for (auto const& item : wallet_properties->grants) {
        auto grant = std::make_unique<base::DictionaryValue>();
        grant->SetString("probi", item.probi);
        grant->SetInteger("expiryTime", item.expiryTime);
        grants->Append(std::move(grant));
      }
      walletInfo->SetList("grants", std::move(grants));

      result.SetDouble("monthlyAmount", wallet_properties->monthly_amount);
    }

    values.SetDictionary("ui", std::move(ui_values));
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
  if (rewards_service_) {
    std::string lang;
    std::string paymentId;
    args->GetString(0, &lang);
    args->GetString(1, &paymentId);
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
  if (rewards_service_) {
    std::string promotion_id;
    std::string promotion_type;
    args->GetString(0, &promotion_id);
    args->GetString(1, &promotion_type);
    rewards_service_->GetGrantCaptcha(promotion_id, promotion_type);
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
  if (rewards_service_) {
    std::string passPhrase;
    args->GetString(0, &passPhrase);
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
      newGrants->Append(std::move(grant));
    }
    recover.SetList("grants", std::move(newGrants));

    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.recoverWalletData", recover);
  }
}

void RewardsDOMHandler::SolveGrantCaptcha(const base::ListValue *args) {
  if (rewards_service_) {
    std::string solution;
    std::string promotionId;
    args->GetString(0, &solution);
    args->GetString(1, &promotionId);
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

void RewardsDOMHandler::OnGetAddresses(
    const std::string func_name,
    const std::map<std::string, std::string>& addresses) {
  if (web_ui()->CanCallJavascript() && (
      func_name == "addresses" || func_name == "addressesForPaymentId")) {
    base::DictionaryValue data;
    data.SetString("BAT", addresses.at("BAT"));
    data.SetString("BTC", addresses.at("BTC"));
    data.SetString("ETH", addresses.at("ETH"));
    data.SetString("LTC", addresses.at("LTC"));

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards." + func_name, data);
  }
}

void RewardsDOMHandler::GetAddresses(const base::ListValue* args) {
  if (rewards_service_)
    rewards_service_->GetAddresses(base::Bind(
          &RewardsDOMHandler::OnGetAddresses,
          weak_factory_.GetWeakPtr(),
          "addresses"));
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
      base::Bind(&RewardsDOMHandler::OnContentSiteList,
                 weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnContentSiteUpdated(
    brave_rewards::RewardsService* rewards_service) {
  rewards_service_->GetAutoContributeProps(
      base::Bind(&RewardsDOMHandler::OnAutoContributePropsReady,
        weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnGetExcludedPublishersNumber(uint32_t num) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.excludedNumber",
        base::Value(std::to_string(num)));
  }
}
void RewardsDOMHandler::OnExcludedSitesChanged(
    brave_rewards::RewardsService* rewards_service,
    std::string publisher_id,
    bool excluded) {
  if (rewards_service_)
    rewards_service_->GetExcludedPublishersNumber(
        base::Bind(&RewardsDOMHandler::OnGetExcludedPublishersNumber,
                   weak_factory_.GetWeakPtr()));
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
  if (rewards_service_) {
    std::string key;
    std::string value;
    args->GetString(0, &key);
    args->GetString(1, &value);

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

void RewardsDOMHandler::ExcludePublisher(const base::ListValue *args) {
  if (rewards_service_) {
    std::string publisherKey;
    args->GetString(0, &publisherKey);
    rewards_service_->ExcludePublisher(publisherKey);
  }
}

void RewardsDOMHandler::RestorePublishers(const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->RestorePublishers();
  }
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
      publisher->SetBoolean("verified", item.verified);
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
    const std::string& category,
    const std::string& probi) {
  GetAllBalanceReports();
  OnContentSiteUpdated(rewards_service);
  GetReconcileStamp(nullptr);
}

void RewardsDOMHandler::OnCurrentTips(
    brave_rewards::RewardsService* rewards_service,
    const brave_rewards::ContentSiteList list) {
  if (web_ui()->CanCallJavascript()) {
    auto publishers = std::make_unique<base::ListValue>();
    for (auto const& item : list) {
      auto publisher = std::make_unique<base::DictionaryValue>();
      publisher->SetString("id", item.id);
      publisher->SetDouble("percentage", item.percentage);
      publisher->SetString("publisherKey", item.id);
      publisher->SetBoolean("verified", item.verified);
      publisher->SetInteger("excluded", item.excluded);
      publisher->SetString("name", item.name);
      publisher->SetString("provider", item.provider);
      publisher->SetString("url", item.url);
      publisher->SetString("favIcon", item.favicon_url);
      publisher->SetInteger("tipDate", item.reconcile_stamp);
      publishers->Append(std::move(publisher));
    }

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.currentTips",
                                           *publishers);
  }
}

void RewardsDOMHandler::RemoveRecurringTip(const base::ListValue *args) {
  if (rewards_service_) {
    std::string publisherKey;
    args->GetString(0, &publisherKey);
    rewards_service_->RemoveRecurringTip(publisherKey);
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
        publisher->SetBoolean("verified", item.verified);
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

void RewardsDOMHandler::GetOneTimeTips(const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->GetOneTimeTips();
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
  if (ads_service_ && web_ui()->CanCallJavascript()) {
    base::DictionaryValue adsData;

    bool ads_ui_enabled;
    bool ads_enabled = ads_service_->is_enabled();
    int ads_per_hour = ads_service_->ads_per_hour();

    #if BUILDFLAG(BRAVE_ADS_ENABLED)
      ads_ui_enabled = true;
    #else
      ads_ui_enabled = false;
    #endif

    adsData.SetBoolean("adsEnabled", ads_enabled);
    adsData.SetInteger("adsPerHour", ads_per_hour);
    adsData.SetBoolean("adsUIEnabled", ads_ui_enabled);

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.adsData", adsData);
  }
}

void RewardsDOMHandler::SaveAdsSetting(const base::ListValue* args) {
  if (ads_service_) {
    std::string key;
    std::string value;
    args->GetString(0, &key);
    args->GetString(1, &value);

    if (key == "adsEnabled") {
      ads_service_->set_ads_enabled(value == "true");
    }

    if (key == "adsPerHour") {
      ads_service_->set_ads_per_hour(std::stoi(value));
    }

    base::ListValue* emptyArgs;
    GetAdsData(emptyArgs);
  }
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
    brave_rewards::ContentSiteList list) {
  std::unique_ptr<brave_rewards::ContentSiteList> site_list(
      new brave_rewards::ContentSiteList);
  for (auto& publisher : list) {
    site_list->push_back(publisher);
  }

  OnContentSiteList(std::move(site_list), 0);
}

void RewardsDOMHandler::GetAddressesForPaymentId(
    const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->GetAddressesForPaymentId(base::Bind(
          &RewardsDOMHandler::OnGetAddresses,
          weak_factory_.GetWeakPtr(),
          "addressesForPaymentId"));
  }
}

void RewardsDOMHandler::GetConfirmationsHistory(
    const base::ListValue* args) {
  rewards_service_->GetConfirmationsHistory(base::Bind(
          &RewardsDOMHandler::OnConfirmationsHistory,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnConfirmationsHistory(
    int total_viewed,
    double estimated_earnings) {
  if (web_ui()->CanCallJavascript()) {
    base::DictionaryValue history;

    history.SetInteger("adsTotalPages", total_viewed);
    history.SetDouble("adsEstimatedEarnings", estimated_earnings);

    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.confirmationsHistory", history);
  }
}

void RewardsDOMHandler::OnConfirmationsHistoryChanged(
    brave_rewards::RewardsService* rewards_service) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards.confirmationsHistoryChanged");
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

void RewardsDOMHandler::GetExcludedPublishersNumber(
    const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->GetExcludedPublishersNumber(
        base::Bind(&RewardsDOMHandler::OnGetExcludedPublishersNumber,
                   weak_factory_.GetWeakPtr()));
  }
}

void RewardsDOMHandler::AdsIsSupportedRegion(
    const base::ListValue* args) {
  ads_service_->IsSupportedRegion(base::BindOnce(
          &RewardsDOMHandler::OnAdsIsSupportedRegion,
          weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnAdsIsSupportedRegion(
    bool is_supported) {
  if (web_ui()->CanCallJavascript()) {
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.adsIsSupportedRegion",
        base::Value(is_supported));
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

}  // namespace

BraveRewardsUI::BraveRewardsUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name,
#if !defined(OS_ANDROID)
    kBraveRewardsGenerated, kBraveRewardsGeneratedSize,
#else
    kBraveRewardsSettingsGenerated, kBraveRewardsSettingsGeneratedSize,
#endif
    IDR_BRAVE_REWARDS_HTML) {
  auto handler_owner = std::make_unique<RewardsDOMHandler>();
  RewardsDOMHandler * handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveRewardsUI::~BraveRewardsUI() {}
