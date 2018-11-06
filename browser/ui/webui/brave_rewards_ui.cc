/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_ui.h"

#include "base/base64.h"
#include "base/memory/weak_ptr.h"

#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/brave_rewards/browser/balance_report.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/bindings_policy.h"


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
  void GetGrant(const base::ListValue* args);
  void GetGrantCaptcha(const base::ListValue* args);
  void GetWalletPassphrase(const base::ListValue* args);
  void RecoverWallet(const base::ListValue* args);
  void SolveGrantCaptcha(const base::ListValue* args);
  void GetReconcileStamp(const base::ListValue* args);
  void GetAddresses(const base::ListValue* args);
  void SaveSetting(const base::ListValue* args);
  void OnGetContentSiteList(std::unique_ptr<brave_rewards::ContentSiteList>, uint32_t record);
  void GetBalanceReports(const base::ListValue* args);
  void ExcludePublisher(const base::ListValue* args);
  void RestorePublishers(const base::ListValue* args);
  void WalletExists(const base::ListValue* args);
  void GetContributionAmount(const base::ListValue* args);
  void RemoveRecurring(const base::ListValue* args);
  void UpdateRecurringDonationsList(const base::ListValue* args);
  void UpdateTipsList(const base::ListValue* args);
  void GetContributionList(const base::ListValue* args);

  // RewardsServiceObserver implementation
  void OnWalletInitialized(brave_rewards::RewardsService* rewards_service,
                       int error_code) override;
  void OnWalletProperties(brave_rewards::RewardsService* rewards_service,
      int error_code,
      std::unique_ptr<brave_rewards::WalletProperties> wallet_properties) override;
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
  void OnContentSiteUpdated(brave_rewards::RewardsService* rewards_service) override;
  void OnExcludedSitesChanged(brave_rewards::RewardsService* rewards_service) override;
  void OnReconcileComplete(brave_rewards::RewardsService* rewards_service,
                           unsigned int result,
                           const std::string& viewing_id,
                           const std::string& probi) override;
  void OnRecurringDonationUpdated(brave_rewards::RewardsService* rewards_service,
                                  brave_rewards::ContentSiteList) override;
  void OnCurrentTips(brave_rewards::RewardsService* rewards_service,
                                  brave_rewards::ContentSiteList) override;

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
  web_ui()->RegisterMessageCallback("brave_rewards.getGrant",
                                    base::BindRepeating(&RewardsDOMHandler::GetGrant,
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
  web_ui()->RegisterMessageCallback("brave_rewards.removeRecurring",
                                    base::BindRepeating(&RewardsDOMHandler::RemoveRecurring,
                                                        base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.updateRecurringDonationsList",
                                    base::BindRepeating(&RewardsDOMHandler::UpdateRecurringDonationsList,
                                                        base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.updateTipsList",
                                    base::BindRepeating(&RewardsDOMHandler::UpdateTipsList,
                                                        base::Unretained(this)));
  web_ui()->RegisterMessageCallback("brave_rewards.getContributionList",
                                    base::BindRepeating(&RewardsDOMHandler::GetContributionList,
                                                        base::Unretained(this)));
}

void RewardsDOMHandler::Init() {
  Profile* profile = Profile::FromWebUI(web_ui());
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_)
    rewards_service_->AddObserver(this);
}

void RewardsDOMHandler::GetAllBalanceReports() {
  if (rewards_service_ && web_ui()->CanCallJavascript()) {
    std::map<std::string, brave_rewards::BalanceReport> reports = rewards_service_->GetAllBalanceReports();
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

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.balanceReports", newReports);
  }
}

void RewardsDOMHandler::HandleCreateWalletRequested(const base::ListValue* args) {
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
    int error_code) {
  if (!web_ui()->CanCallJavascript())
    return;

  if (error_code == 0)
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletCreated");
  else
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletCreateFailed");
}

void RewardsDOMHandler::OnWalletProperties(
    brave_rewards::RewardsService* rewards_service,
    int error_code,
    std::unique_ptr<brave_rewards::WalletProperties> wallet_properties) {

  if (web_ui()->CanCallJavascript()) {
    base::DictionaryValue result;
    result.SetInteger("status", error_code);
    auto walletInfo = std::make_unique<base::DictionaryValue>();

    if (error_code == 0 && wallet_properties) {
      walletInfo->SetDouble("balance", wallet_properties->balance);
      walletInfo->SetString("probi", wallet_properties->probi);

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
    }

    result.SetDictionary("wallet", std::move(walletInfo));

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletProperties", result);
  }
}

void RewardsDOMHandler::OnGrant(
    brave_rewards::RewardsService* rewards_service,
    unsigned int result,
    brave_rewards::Grant grant) {
  if (web_ui()->CanCallJavascript()) {
    base::DictionaryValue newGrant;
    newGrant.SetInteger("status", result);
    newGrant.SetString("promotionId", grant.promotionId);

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.grant", newGrant);
  }
}

void RewardsDOMHandler::GetGrant(const base::ListValue* args) {
  if (rewards_service_) {
    std::string lang;
    std::string paymentId;
    args->GetString(0, &lang);
    args->GetString(1, &paymentId);
    rewards_service_->FetchGrant(lang, paymentId);
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

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.grantCaptcha", captcha);
  }
}

void RewardsDOMHandler::GetGrantCaptcha(const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->GetGrantCaptcha();
  }
}

void RewardsDOMHandler::GetWalletPassphrase(const base::ListValue* args) {
  if (rewards_service_ && web_ui()->CanCallJavascript()) {
    std::string pass = rewards_service_->GetWalletPassphrase();

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletPassphrase", base::Value(pass));
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

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.recoverWalletData", recover);
  }
}

void RewardsDOMHandler::SolveGrantCaptcha(const base::ListValue *args) {
  if (rewards_service_) {
    std::string solution;
    args->GetString(0, &solution);
    rewards_service_->SolveGrantCaptcha(solution);
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

void RewardsDOMHandler::GetReconcileStamp(const base::ListValue* args) {
  if (rewards_service_ && web_ui()->CanCallJavascript()) {
    std::string stamp = std::to_string(rewards_service_->GetReconcileStamp());

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.reconcileStamp", base::Value(stamp));
  }
}

void RewardsDOMHandler::GetAddresses(const base::ListValue* args) {
  if (rewards_service_ && web_ui()->CanCallJavascript()) {
    std::map<std::string, std::string> addresses = rewards_service_->GetAddresses();

    base::DictionaryValue data;
    data.SetString("BAT", addresses["BAT"]);
    data.SetString("BTC", addresses["BTC"]);
    data.SetString("ETH", addresses["ETH"]);
    data.SetString("LTC", addresses["LTC"]);

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.addresses", data);
  }
}

void RewardsDOMHandler::OnContentSiteUpdated(brave_rewards::RewardsService* rewards_service) {
  rewards_service_->GetContentSiteList(0, 0,
      base::Bind(&RewardsDOMHandler::OnGetContentSiteList, weak_factory_.GetWeakPtr()));
}

void RewardsDOMHandler::OnExcludedSitesChanged(brave_rewards::RewardsService* rewards_service) {
  if (rewards_service_ && web_ui()->CanCallJavascript()) {
    int num = (int)rewards_service_->GetNumExcludedSites();
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.numExcludedSites", base::Value(num));
  }
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

void RewardsDOMHandler::OnGetContentSiteList(std::unique_ptr<brave_rewards::ContentSiteList> list, uint32_t record) {
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

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.contributeList", *publishers);
  }
}


void RewardsDOMHandler::GetBalanceReports(const base::ListValue* args) {
  GetAllBalanceReports();
}

void RewardsDOMHandler::WalletExists(const base::ListValue* args) {
  if (rewards_service_ && web_ui()->CanCallJavascript()) {
    bool exist = rewards_service_->IsWalletCreated();

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletExists", base::Value(exist));
  }
}

void RewardsDOMHandler::GetContributionAmount(const base::ListValue* args) {
  if (rewards_service_ && web_ui()->CanCallJavascript()) {
    double amount = rewards_service_->GetContributionAmount();

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.contributionAmount", base::Value(amount));
  }
}

void RewardsDOMHandler::OnReconcileComplete(brave_rewards::RewardsService* rewards_service,
  unsigned int result,
  const std::string& viewing_id,
  const std::string& probi) {
  GetAllBalanceReports();
  OnContentSiteUpdated(rewards_service);
  GetReconcileStamp(nullptr);
}

void RewardsDOMHandler::OnRecurringDonationUpdated(brave_rewards::RewardsService* rewards_service,
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
      publisher->SetInteger("tipDate", 0);
      publishers->Append(std::move(publisher));
    }

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.recurringDonationUpdate", *publishers);
  }
}

void RewardsDOMHandler::OnCurrentTips(brave_rewards::RewardsService* rewards_service,
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

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.currentTips", *publishers);
  }
}

void RewardsDOMHandler::RemoveRecurring(const base::ListValue *args) {
  if (rewards_service_) {
    std::string publisherKey;
    args->GetString(0, &publisherKey);
    rewards_service_->RemoveRecurring(publisherKey);
  }
}

void RewardsDOMHandler::UpdateRecurringDonationsList(const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->UpdateRecurringDonationsList();
  }
}

void RewardsDOMHandler::UpdateTipsList(const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->UpdateTipsList();
  }
}

void RewardsDOMHandler::GetContributionList(const base::ListValue *args) {
  if (rewards_service_) {
    OnContentSiteUpdated(rewards_service_);
  }
}

}  // namespace

BraveRewardsUI::BraveRewardsUI(content::WebUI* web_ui, const std::string& name)
    : BasicUI(web_ui, name, kRewardsJS,
        IDR_BRAVE_REWARDS_JS, IDR_BRAVE_REWARDS_HTML) {

  auto handler_owner = std::make_unique<RewardsDOMHandler>();
  RewardsDOMHandler * handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveRewardsUI::~BraveRewardsUI() {
}
