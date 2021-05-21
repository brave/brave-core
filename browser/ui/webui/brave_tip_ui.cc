/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_tip_ui.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_tip_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/base/l10n/l10n_util.h"

using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;
using brave_rewards::RewardsServiceObserver;
using content::WebUIMessageHandler;

namespace {

class TipMessageHandler : public WebUIMessageHandler,
                          public RewardsServiceObserver {
 public:
  TipMessageHandler();
  ~TipMessageHandler() override;

  TipMessageHandler(const TipMessageHandler&) = delete;
  TipMessageHandler operator=(const TipMessageHandler&) = delete;

  // WebUIMessageHandler:
  void RegisterMessages() override;

  // RewardsServiceObserver:
  void OnRecurringTipSaved(
      RewardsService* rewards_service,
      bool success) override;

  void OnRecurringTipRemoved(
      RewardsService* rewards_service,
      bool success) override;

  void OnReconcileComplete(
      RewardsService* rewards_service,
      const ledger::type::Result result,
      const std::string& contribution_id,
      const double amount,
      const ledger::type::RewardsType type,
      const ledger::type::ContributionProcessor processor) override;

  void OnUnblindedTokensReady(RewardsService* rewards_service) override;

 private:
  // Message handlers
  void DialogReady(const base::ListValue* args);
  void GetPublisherBanner(const base::ListValue* args);
  void GetRewardsParameters(const base::ListValue* args);
  void GetOnboardingStatus(const base::ListValue* args);
  void SaveOnboardingResult(const base::ListValue* args);
  void OnTip(const base::ListValue* args);
  void GetRecurringTips(const base::ListValue* args);
  void GetReconcileStamp(const base::ListValue* args);
  void GetAutoContributeAmount(const base::ListValue* args);
  void SetAutoContributeAmount(const base::ListValue* args);
  void GetAdsPerHour(const base::ListValue* args);
  void SetAdsPerHour(const base::ListValue* args);
  void TweetTip(const base::ListValue* args);
  void GetOnlyAnonWallet(const base::ListValue* args);
  void GetExternalWallet(const base::ListValue* args);
  void FetchBalance(const base::ListValue* args);

  // Rewards service callbacks
  void GetReconcileStampCallback(uint64_t reconcile_stamp);

  void GetAutoContributeAmountCallback(double amount);

  void GetRecurringTipsCallback(ledger::type::PublisherInfoList list);

  void GetExternalWalletCallback(const ledger::type::Result result,
                                 ledger::type::ExternalWalletPtr wallet);

  void GetPublisherBannerCallback(ledger::type::PublisherBannerPtr banner);

  void GetShareURLCallback(const std::string& url);

  void FetchBalanceCallback(
    const ledger::type::Result result,
    ledger::type::BalancePtr balance);

  void GetRewardsParametersCallback(
    ledger::type::RewardsParametersPtr parameters);

  RewardsService* rewards_service_ = nullptr;  // NOT OWNED
  brave_ads::AdsService* ads_service_ = nullptr;  // NOT OWNED
  base::WeakPtrFactory<TipMessageHandler> weak_factory_{this};
};

TipMessageHandler::TipMessageHandler() = default;

TipMessageHandler::~TipMessageHandler() {
  if (rewards_service_) {
    rewards_service_->RemoveObserver(this);
  }
}

void TipMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "dialogReady",
      base::BindRepeating(
          &TipMessageHandler::DialogReady,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getPublisherBanner",
      base::BindRepeating(
          &TipMessageHandler::GetPublisherBanner,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getRewardsParameters",
      base::BindRepeating(
          &TipMessageHandler::GetRewardsParameters,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getOnboardingStatus",
      base::BindRepeating(
          &TipMessageHandler::GetOnboardingStatus,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "saveOnboardingResult",
      base::BindRepeating(
          &TipMessageHandler::SaveOnboardingResult,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "onTip",
       base::BindRepeating(
           &TipMessageHandler::OnTip,
           base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getRecurringTips",
      base::BindRepeating(
          &TipMessageHandler::GetRecurringTips,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getReconcileStamp",
      base::BindRepeating(
          &TipMessageHandler::GetReconcileStamp,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getAutoContributeAmount",
      base::BindRepeating(&TipMessageHandler::GetAutoContributeAmount,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "setAutoContributeAmount",
      base::BindRepeating(&TipMessageHandler::SetAutoContributeAmount,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getAdsPerHour", base::BindRepeating(&TipMessageHandler::GetAdsPerHour,
                                           base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "setAdsPerHour", base::BindRepeating(&TipMessageHandler::SetAdsPerHour,
                                           base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "tweetTip",
      base::BindRepeating(
          &TipMessageHandler::TweetTip,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "fetchBalance",
      base::BindRepeating(
          &TipMessageHandler::FetchBalance,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getExternalWallet",
      base::BindRepeating(
          &TipMessageHandler::GetExternalWallet,
          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getOnlyAnonWallet",
      base::BindRepeating(
          &TipMessageHandler::GetOnlyAnonWallet,
          base::Unretained(this)));
}

void TipMessageHandler::OnRecurringTipRemoved(
    RewardsService* rewards_service,
    bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  FireWebUIListener("recurringTipRemoved", base::Value(success));
}

void TipMessageHandler::OnRecurringTipSaved(
    RewardsService* rewards_service,
    bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  FireWebUIListener("recurringTipSaved", base::Value(success));
}

void TipMessageHandler::OnReconcileComplete(
    RewardsService* rewards_service,
    const ledger::type::Result result,
    const std::string& contribution_id,
    const double amount,
    const ledger::type::RewardsType type,
    const ledger::type::ContributionProcessor processor) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("result", static_cast<int>(result));
  data.SetIntKey("type", static_cast<int>(type));

  FireWebUIListener("reconcileCompleted", data);
}

void TipMessageHandler::OnUnblindedTokensReady(
    RewardsService* rewards_service) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  FireWebUIListener("unblindedTokensReady");
}

void TipMessageHandler::DialogReady(const base::ListValue* args) {
  Profile* profile = Profile::FromWebUI(web_ui());

  if (!rewards_service_) {
    rewards_service_ = RewardsServiceFactory::GetForProfile(profile);
    if (rewards_service_) {
      rewards_service_->AddObserver(this);
    }
  }

  if (!ads_service_) {
    ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile);
  }

  AllowJavascript();
  if (rewards_service_ && rewards_service_->IsInitialized()) {
    FireWebUIListener("rewardsInitialized");
  }
}

void TipMessageHandler::GetOnlyAnonWallet(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }
  const bool only_anon = rewards_service_->OnlyAnonWallet();
  FireWebUIListener("onlyAnonWalletUpdated", base::Value(only_anon));
}

void TipMessageHandler::GetPublisherBanner(const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  const std::string publisher_key = args->GetList()[0].GetString();

  if (publisher_key.empty() || !rewards_service_) {
    return;
  }

  rewards_service_->GetPublisherBanner(
      publisher_key,
      base::BindOnce(&TipMessageHandler::GetPublisherBannerCallback,
                     weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::GetRewardsParameters(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->GetRewardsParameters(
      base::BindOnce(&TipMessageHandler::GetRewardsParametersCallback,
                     weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::GetOnboardingStatus(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }
  AllowJavascript();
  base::Value data(base::Value::Type::DICTIONARY);
  data.SetBoolKey("showOnboarding", rewards_service_->ShouldShowOnboarding());
  FireWebUIListener("onboardingStatusUpdated", data);
}

void TipMessageHandler::SaveOnboardingResult(const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  if (!rewards_service_)
    return;

  if (args->GetList()[0].GetString() == "opted-in")
    rewards_service_->EnableRewards();
}

void TipMessageHandler::OnTip(const base::ListValue* args) {
  CHECK_EQ(3U, args->GetSize());
  const std::string publisher_key = args->GetList()[0].GetString();
  const double amount = args->GetList()[1].GetDouble();
  const bool recurring = args->GetList()[2].GetBool();

  if (publisher_key.empty() || !rewards_service_) {
    return;
  }

  if (recurring && amount <= 0) {
    rewards_service_->RemoveRecurringTip(publisher_key);
  } else if (amount > 0) {
    rewards_service_->OnTip(publisher_key, amount, recurring);
  }
}

void TipMessageHandler::GetReconcileStamp(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->GetReconcileStamp(
      base::BindOnce(&TipMessageHandler::GetReconcileStampCallback,
                     weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::GetAutoContributeAmount(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->GetAutoContributionAmount(
      base::BindOnce(&TipMessageHandler::GetAutoContributeAmountCallback,
                     weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::SetAutoContributeAmount(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  double amount = args->GetList()[0].GetDouble();
  if (!rewards_service_ || amount < 0) {
    return;
  }
  AllowJavascript();
  rewards_service_->SetAutoContributionAmount(amount);
  FireWebUIListener("autoContributeAmountUpdated", base::Value(amount));
}

void TipMessageHandler::GetAdsPerHour(const base::ListValue* args) {
  if (!ads_service_) {
    return;
  }
  AllowJavascript();
  double adsPerHour = static_cast<double>(ads_service_->GetAdsPerHour());
  FireWebUIListener("adsPerHourUpdated", base::Value(adsPerHour));
}

void TipMessageHandler::SetAdsPerHour(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 1U);
  const double adsPerHour = args->GetList()[0].GetDouble();
  if (!ads_service_ || adsPerHour < 0) {
    return;
  }
  AllowJavascript();
  ads_service_->SetAdsPerHour(adsPerHour);
  FireWebUIListener("adsPerHourUpdated", base::Value(adsPerHour));
}

void TipMessageHandler::TweetTip(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 2U);
  const std::string name = args->GetList()[0].GetString();
  const std::string tweet_id = args->GetList()[1].GetString();

  if (name.empty() || !rewards_service_) {
    return;
  }

  const std::string comment = l10n_util::GetStringFUTF8(
      IDS_BRAVE_REWARDS_LOCAL_COMPLIMENT_TWEET,
      base::UTF8ToUTF16(name));

  const std::string hashtag = l10n_util::GetStringUTF8(
      IDS_BRAVE_REWARDS_LOCAL_COMPLIMENT_TWEET_HASHTAG);

  base::flat_map<std::string, std::string> share_url_args;
  share_url_args["comment"] = comment;
  share_url_args["hashtag"] = hashtag;
  share_url_args["name"] = name.substr(1);
  share_url_args["tweet_id"] = tweet_id;

  rewards_service_->GetShareURL(
      share_url_args, base::BindOnce(&TipMessageHandler::GetShareURLCallback,
                                     weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::FetchBalance(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }
  rewards_service_->FetchBalance(base::BindOnce(
      &TipMessageHandler::FetchBalanceCallback,
      weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::GetExternalWallet(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }
  rewards_service_->GetExternalWallet(
      base::BindOnce(&TipMessageHandler::GetExternalWalletCallback,
                     weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::GetRecurringTips(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }
  rewards_service_->GetRecurringTips(base::BindOnce(
      &TipMessageHandler::GetRecurringTipsCallback,
      weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::GetRewardsParametersCallback(
    ledger::type::RewardsParametersPtr parameters) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);
  if (parameters) {
    base::Value tip_choices(base::Value::Type::LIST);
    for (const auto& item : parameters->tip_choices) {
      tip_choices.Append(item);
    }

    base::Value monthly_choices(base::Value::Type::LIST);
    for (const auto& item : parameters->monthly_tip_choices) {
      monthly_choices.Append(item);
    }

    base::Value ac_choices(base::Value::Type::LIST);
    for (const auto& item : parameters->auto_contribute_choices) {
      ac_choices.Append(item);
    }

    data.SetDoubleKey("rate", parameters->rate);
    data.SetKey("tipChoices", std::move(tip_choices));
    data.SetKey("monthlyTipChoices", std::move(monthly_choices));
    data.SetKey("autoContributeChoices", std::move(ac_choices));
  }

  FireWebUIListener("rewardsParametersUpdated", data);
}

void TipMessageHandler::GetRecurringTipsCallback(
    ledger::type::PublisherInfoList list) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value publishers(base::Value::Type::LIST);
  for (const auto& item : list) {
    base::Value publisher(base::Value::Type::DICTIONARY);
    publisher.SetStringKey("publisherKey", item->id);
    publisher.SetDoubleKey("amount", item->weight);
    publishers.Append(std::move(publisher));
  }

  FireWebUIListener("recurringTipsUpdated", publishers);
}

void TipMessageHandler::GetPublisherBannerCallback(
    ledger::type::PublisherBannerPtr banner) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value result(base::Value::Type::DICTIONARY);

  if (banner) {
    result.SetStringKey("publisherKey", banner->publisher_key);
    result.SetStringKey("title", banner->title);
    result.SetStringKey("name", banner->name);
    result.SetStringKey("description", banner->description);
    result.SetStringKey("background", banner->background);
    result.SetStringKey("logo", banner->logo);
    result.SetStringKey("provider", banner->provider);
    result.SetIntKey("status", static_cast<int>(banner->status));

    base::Value amounts(base::Value::Type::LIST);
    for (const auto& value : banner->amounts) {
      amounts.Append(value);
    }
    result.SetKey("amounts", std::move(amounts));

    base::Value links(base::Value::Type::DICTIONARY);
    for (const auto& item : banner->links) {
      links.SetStringKey(item.first, item.second);
    }
    result.SetKey("links", std::move(links));
  }

  FireWebUIListener("publisherBannerUpdated", result);
}

void TipMessageHandler::GetReconcileStampCallback(uint64_t reconcile_stamp) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  const std::string stamp = base::NumberToString(reconcile_stamp);
  FireWebUIListener("reconcileStampUpdated", base::Value(stamp));
}

void TipMessageHandler::GetAutoContributeAmountCallback(double amount) {
  if (!IsJavascriptAllowed()) {
    return;
  }
  FireWebUIListener("autoContributeAmountUpdated", base::Value(amount));
}

void TipMessageHandler::GetShareURLCallback(const std::string& url) {
  GURL gurl(url);
  if (!gurl.is_valid()) {
    return;
  }

  // Open a new tab with the prepopulated tweet ready to share.
  chrome::ScopedTabbedBrowserDisplayer browser_displayer(
      Profile::FromWebUI(web_ui()));

  browser_displayer.browser()->OpenURL(content::OpenURLParams(
      gurl,
      content::Referrer(),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
      false));
}

void TipMessageHandler::FetchBalanceCallback(
    const ledger::type::Result result,
    ledger::type::BalancePtr balance) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("status", static_cast<int>(result));

  if (result == ledger::type::Result::LEDGER_OK && balance) {
    base::Value wallets(base::Value::Type::DICTIONARY);
    for (const auto& wallet : balance->wallets) {
      wallets.SetDoubleKey(wallet.first, wallet.second);
    }

    base::Value balance_value(base::Value::Type::DICTIONARY);
    balance_value.SetDoubleKey("total", balance->total);
    balance_value.SetKey("wallets", std::move(wallets));

    data.SetKey("balance", std::move(balance_value));
  }

  FireWebUIListener("balanceUpdated", data);
}

void TipMessageHandler::GetExternalWalletCallback(
    const ledger::type::Result result,
    ledger::type::ExternalWalletPtr wallet) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);
  if (wallet) {
    data.SetStringKey("type", wallet->type);
    data.SetIntKey("status", static_cast<int>(wallet->status));
  }

  FireWebUIListener("externalWalletUpdated", data);
}

}  // namespace

BraveTipUI::BraveTipUI(content::WebUI* web_ui, const std::string& name)
    : ConstrainedWebDialogUI(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  if (!brave::IsRegularProfile(profile)) {
    return;
  }

  CreateAndAddWebUIDataSource(web_ui, name, kBraveRewardsTipGenerated,
                              kBraveRewardsTipGeneratedSize,
                              IDR_BRAVE_REWARDS_TIP_HTML);

  web_ui->AddMessageHandler(std::make_unique<TipMessageHandler>());
}

BraveTipUI::~BraveTipUI() = default;
