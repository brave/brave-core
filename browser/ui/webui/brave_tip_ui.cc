/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_tip_ui.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ui/webui/basic_ui.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_tip_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/base/l10n/l10n_util.h"

using content::WebUIMessageHandler;

namespace {

// The handler for Javascript messages for Brave about: pages
class RewardsTipDOMHandler : public WebUIMessageHandler,
                             public brave_rewards::RewardsServiceObserver {
 public:
  RewardsTipDOMHandler();
  ~RewardsTipDOMHandler() override;

  void Init();

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void GetPublisherTipData(const base::ListValue* args);
  void GetWalletProperties(const base::ListValue* args);
  void OnTip(const base::ListValue* args);
  void GetRecurringTips(const base::ListValue* args);
  void GetReconcileStamp(const base::ListValue* args);
  void OnReconcileStamp(uint64_t reconcile_stamp);
  void OnGetRecurringTips(
      std::unique_ptr<brave_rewards::ContentSiteList> list);
  void TweetTip(const base::ListValue *args);
  void OnlyAnonWallet(const base::ListValue* args);
  void GetExternalWallet(const base::ListValue* args);
  void OnExternalWallet(
      const int32_t result,
      std::unique_ptr<brave_rewards::ExternalWallet> wallet);

  void OnPublisherBanner(
      std::unique_ptr<brave_rewards::PublisherBanner> banner);

  void OnTwitterShareURL(const std::string& url);

  void FetchBalance(const base::ListValue* args);
  void OnFetchBalance(
    int32_t result,
    std::unique_ptr<brave_rewards::Balance> balance);

  // RewardsServiceObserver implementation
  void OnWalletProperties(
      brave_rewards::RewardsService* rewards_service,
      int error_code,
      std::unique_ptr<brave_rewards::WalletProperties> wallet_properties)
      override;

  void OnRecurringTipSaved(brave_rewards::RewardsService* rewards_service,
                           bool success) override;

  void OnRecurringTipRemoved(brave_rewards::RewardsService* rewards_service,
                             bool success) override;

  void OnReconcileComplete(
      brave_rewards::RewardsService* rewards_service,
      unsigned int result,
      const std::string& contribution_id,
      const double amount,
      const int32_t type) override;

  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED
  base::WeakPtrFactory<RewardsTipDOMHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RewardsTipDOMHandler);
};

RewardsTipDOMHandler::RewardsTipDOMHandler() : weak_factory_(this) {}

RewardsTipDOMHandler::~RewardsTipDOMHandler() {
  if (rewards_service_)
    rewards_service_->RemoveObserver(this);
}

void RewardsTipDOMHandler::Init() {
  Profile* profile = Profile::FromWebUI(web_ui());
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_)
    rewards_service_->AddObserver(this);
}

void RewardsTipDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "brave_rewards_tip.getPublisherBanner",
      base::BindRepeating(&RewardsTipDOMHandler::GetPublisherTipData,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_tip.getWalletProperties",
      base::BindRepeating(&RewardsTipDOMHandler::GetWalletProperties,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_tip.onTip",
       base::BindRepeating(&RewardsTipDOMHandler::OnTip,
                           base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_tip.getRecurringTips",
      base::BindRepeating(&RewardsTipDOMHandler::GetRecurringTips,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_tip.getReconcileStamp",
      base::BindRepeating(&RewardsTipDOMHandler::GetReconcileStamp,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_tip.tweetTip",
      base::BindRepeating(&RewardsTipDOMHandler::TweetTip,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_tip.fetchBalance",
      base::BindRepeating(&RewardsTipDOMHandler::FetchBalance,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_tip.getExternalWallet",
      base::BindRepeating(
          &RewardsTipDOMHandler::GetExternalWallet,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_tip.onlyAnonWallet",
      base::BindRepeating(
          &RewardsTipDOMHandler::OnlyAnonWallet,
          base::Unretained(this)));
}

void RewardsTipDOMHandler::GetPublisherTipData(
    const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  const std::string publisher_key = args->GetList()[0].GetString();
  rewards_service_->GetPublisherBanner(
      publisher_key,
      base::Bind(&RewardsTipDOMHandler::OnPublisherBanner,
                 weak_factory_.GetWeakPtr()));
}

void RewardsTipDOMHandler::GetWalletProperties(const base::ListValue* args) {
  if (!rewards_service_)
    return;

  rewards_service_->FetchWalletProperties();
}

static std::unique_ptr<base::ListValue> CreateListOfDoubles(
    const std::vector<double>& items) {
  auto result = std::make_unique<base::ListValue>();
  for (double const& item : items) {
    result->AppendDouble(item);
  }
  return result;
}

void RewardsTipDOMHandler::OnWalletProperties(
    brave_rewards::RewardsService* rewards_service,
    int error_code,
    std::unique_ptr<brave_rewards::WalletProperties> wallet_properties) {

  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::DictionaryValue result;
  result.SetInteger("status", error_code);
  auto walletInfo = std::make_unique<base::DictionaryValue>();

  if (error_code == 0 && wallet_properties) {
    walletInfo->SetList("choices",
        CreateListOfDoubles(wallet_properties->parameters_choices));
    walletInfo->SetList("defaultTipChoices",
        CreateListOfDoubles(wallet_properties->default_tip_choices));
    walletInfo->SetList("defaultMonthlyTipChoices",
        CreateListOfDoubles(wallet_properties->default_monthly_tip_choices));
  }

  result.SetDictionary("wallet", std::move(walletInfo));

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_tip.walletProperties", result);
}

void RewardsTipDOMHandler::OnTip(const base::ListValue* args) {
  if (!rewards_service_ || !args) {
    return;
  }

  CHECK_EQ(3U, args->GetSize());

  const std::string publisher_key = args->GetList()[0].GetString();
  const double amount = args->GetList()[1].GetDouble();
  const bool recurring = args->GetList()[2].GetBool();

  if (publisher_key.empty() || amount < 1) {
    // TODO(nejczdovc) add error
    return;
  }

  rewards_service_->OnTip(publisher_key, amount, recurring);
}

void RewardsTipDOMHandler::GetRecurringTips(
    const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->GetRecurringTips(base::BindOnce(
          &RewardsTipDOMHandler::OnGetRecurringTips,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsTipDOMHandler::OnGetRecurringTips(
    std::unique_ptr<brave_rewards::ContentSiteList> list) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  auto publishers = std::make_unique<base::ListValue>();

  if (list) {
    for (auto const& item : *list) {
      auto publisher = std::make_unique<base::DictionaryValue>();
      publisher->SetString("publisherKey", item.id);
      publisher->SetInteger("monthlyDate", item.reconcile_stamp);
      publishers->Append(std::move(publisher));
    }
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_tip.recurringTips", *publishers);
}

void RewardsTipDOMHandler::OnPublisherBanner(
    std::unique_ptr<brave_rewards::PublisherBanner> banner) {
  if (!web_ui()->CanCallJavascript()) {
     return;
  }

  base::DictionaryValue result;
  if (banner) {
    result.SetString("publisherKey", banner->publisher_key);
    result.SetString("title", banner->title);
    result.SetString("name", banner->name);
    result.SetString("description", banner->description);
    result.SetString("background", banner->background);
    result.SetString("logo", banner->logo);
    result.SetString("provider", banner->provider);
    result.SetInteger("status", static_cast<int>(banner->status));

    auto amounts = std::make_unique<base::ListValue>();
    for (auto const& value : banner->amounts) {
      amounts->AppendInteger(value);
    }
    result.SetList("amounts", std::move(amounts));

    auto links = std::make_unique<base::DictionaryValue>();
    for (auto const& item : banner->links) {
      links->SetString(item.first, item.second);
    }
    result.SetDictionary("links", std::move(links));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_tip.publisherBanner", result);
}

}  // namespace

BraveTipUI::BraveTipUI(content::WebUI* web_ui, const std::string& name)
    : ConstrainedWebDialogUI(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  // Show error for non-supported contexts
  if (profile->IsOffTheRecord()) {
    return;
  }
  content::WebUIDataSource* data_source = CreateBasicUIHTMLSource(profile,
                                              name,
                                              kBraveRewardsTipGenerated,
                                              kBraveRewardsTipGeneratedSize,
                                              IDR_BRAVE_REWARDS_TIP_HTML);
  content::WebUIDataSource::Add(profile, data_source);

  auto handler_owner = std::make_unique<RewardsTipDOMHandler>();
  RewardsTipDOMHandler * handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveTipUI::~BraveTipUI() {
}

void RewardsTipDOMHandler::GetReconcileStamp(const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->GetReconcileStamp(base::Bind(
          &RewardsTipDOMHandler::OnReconcileStamp,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsTipDOMHandler::OnReconcileStamp(uint64_t reconcile_stamp) {
  if (!web_ui()->CanCallJavascript()) {
     return;
  }

  const std::string stamp = std::to_string(reconcile_stamp);
  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards_tip.reconcileStamp",
      base::Value(stamp));
}

void RewardsTipDOMHandler::OnRecurringTipRemoved(
    brave_rewards::RewardsService* rewards_service,
    bool success) {
  if (!web_ui()->CanCallJavascript()) {
     return;
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_tip.recurringTipRemoved", base::Value(success));
}

void RewardsTipDOMHandler::OnRecurringTipSaved(
    brave_rewards::RewardsService* rewards_service,
    bool success) {
  if (!web_ui()->CanCallJavascript()) {
     return;
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_tip.recurringTipSaved", base::Value(success));
}

void RewardsTipDOMHandler::TweetTip(const base::ListValue *args) {
  DCHECK_EQ(args->GetSize(), 2U);

  if (!rewards_service_)
    return;

  // Retrieve the relevant metadata from arguments.
  std::string name;
  if (!args->GetString(0, &name))
    return;
  std::string tweet_id;
  if (!args->GetString(1, &tweet_id))
    return;

  // Share the tip comment/compliment on Twitter.
  std::string comment = l10n_util::GetStringFUTF8(
      IDS_BRAVE_REWARDS_LOCAL_COMPLIMENT_TWEET, base::UTF8ToUTF16(name));
  std::string hashtag = l10n_util::GetStringUTF8(
      IDS_BRAVE_REWARDS_LOCAL_COMPLIMENT_TWEET_HASHTAG);
  std::map<std::string, std::string> share_url_args;
  share_url_args["comment"] = comment;
  share_url_args["hashtag"] = hashtag;
  share_url_args["name"] = name.erase(0, 1);
  share_url_args["tweet_id"] = tweet_id;
  rewards_service_->GetShareURL(
      "twitter", share_url_args,
      base::BindOnce(&RewardsTipDOMHandler::OnTwitterShareURL,
                     base::Unretained(this)));
}

void RewardsTipDOMHandler::OnTwitterShareURL(const std::string& url) {
  GURL gurl(url);
  if (!gurl.is_valid())
    return;

  // Open a new tab with the prepopulated tweet ready to share.
  chrome::ScopedTabbedBrowserDisplayer browser_displayer(
      Profile::FromWebUI(web_ui()));
  content::OpenURLParams open_url_params(
      gurl, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false);
  browser_displayer.browser()->OpenURL(open_url_params);
}

void RewardsTipDOMHandler::OnFetchBalance(
    int32_t result,
    std::unique_ptr<brave_rewards::Balance> balance) {
  if (web_ui()->CanCallJavascript()) {
    base::DictionaryValue data;
    data.SetInteger("status", result);
    auto balance_value = std::make_unique<base::DictionaryValue>();

    if (result == 0 && balance) {
      balance_value->SetDouble("total", balance->total);

      auto rates = std::make_unique<base::DictionaryValue>();
      for (auto const& rate : balance->rates) {
        rates->SetDouble(rate.first, rate.second);
      }
      balance_value->SetDictionary("rates", std::move(rates));

      auto wallets = std::make_unique<base::DictionaryValue>();
      for (auto const& wallet : balance->wallets) {
        wallets->SetDouble(wallet.first, wallet.second);
      }
      balance_value->SetDictionary("wallets", std::move(wallets));

      data.SetDictionary("balance", std::move(balance_value));
    }

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards_tip.balance", data);
  }
}

void RewardsTipDOMHandler::FetchBalance(const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->FetchBalance(base::BindOnce(
          &RewardsTipDOMHandler::OnFetchBalance,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsTipDOMHandler::GetExternalWallet(
    const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }

  const std::string type = args->GetList()[0].GetString();

  rewards_service_->GetExternalWallet(type,
     base::BindOnce(
         &RewardsTipDOMHandler::OnExternalWallet,
         weak_factory_.GetWeakPtr()));
}

void RewardsTipDOMHandler::OnExternalWallet(
    const int32_t result,
    std::unique_ptr<brave_rewards::ExternalWallet> wallet) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::DictionaryValue data;

  if (wallet) {
    data.SetString("token", wallet->token);
    data.SetString("address", wallet->address);
    data.SetString("type", wallet->type);
    data.SetString("verifyUrl", wallet->verify_url);
    data.SetString("addUrl", wallet->add_url);
    data.SetString("withdrawUrl", wallet->withdraw_url);
    data.SetString("userName", wallet->user_name);
    data.SetString("accountUrl", wallet->account_url);
    data.SetInteger("status", static_cast<int>(wallet->status));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_tip.externalWallet", data);
}

void RewardsTipDOMHandler::OnlyAnonWallet(const base::ListValue* args) {
  if (!rewards_service_ || !web_ui()->CanCallJavascript()) {
    return;
  }

  const bool allow = rewards_service_->OnlyAnonWallet();

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_tip.onlyAnonWallet",
      base::Value(allow));
}

void RewardsTipDOMHandler::OnReconcileComplete(
    brave_rewards::RewardsService* rewards_service,
    unsigned int result,
    const std::string& contribution_id,
    const double amount,
    const int32_t type) {
  if (!web_ui()->CanCallJavascript()) {
     return;
  }

  base::DictionaryValue complete;
  complete.SetKey("result", base::Value(static_cast<int>(result)));
  complete.SetKey("type", base::Value(type));

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_tip.reconcileComplete",
      complete);
}
