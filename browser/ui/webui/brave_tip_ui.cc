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
#include "base/i18n/time_formatting.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
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

using brave_rewards::FetchBalanceResult;
using brave_rewards::GetExternalWalletResult;
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
  void OnRecurringTipSaved(RewardsService* rewards_service,
                           bool success) override;

  void OnRecurringTipRemoved(RewardsService* rewards_service,
                             bool success) override;

  void OnPendingContributionSaved(RewardsService* rewards_service,
                                  ledger::mojom::Result result) override;

  void OnReconcileComplete(
      RewardsService* rewards_service,
      const ledger::mojom::Result result,
      const std::string& contribution_id,
      const double amount,
      const ledger::mojom::RewardsType type,
      const ledger::mojom::ContributionProcessor processor) override;

  void OnUnblindedTokensReady(RewardsService* rewards_service) override;

 private:
  // Message handlers
  void DialogReady(const base::Value::List& args);
  void GetPublisherBanner(const base::Value::List& args);
  void GetUserType(const base::Value::List& args);
  void GetRewardsParameters(const base::Value::List& args);
  void OnTip(const base::Value::List& args);
  void GetRecurringTips(const base::Value::List& args);
  void GetReconcileStamp(const base::Value::List& args);
  void TweetTip(const base::Value::List& args);
  void GetExternalWallet(const base::Value::List& args);
  void FetchBalance(const base::Value::List& args);

  // Rewards service callbacks
  void GetUserTypeCallback(ledger::mojom::UserType user_type);

  void OnTipCallback(double amount, ledger::mojom::Result result);

  void GetReconcileStampCallback(uint64_t reconcile_stamp);

  void GetRecurringTipsCallback(
      std::vector<ledger::mojom::PublisherInfoPtr> list);

  void OnGetExternalWallet(GetExternalWalletResult result);

  void GetPublisherBannerCallback(ledger::mojom::PublisherBannerPtr banner);

  void GetShareURLCallback(const std::string& url);

  void FetchBalanceCallback(FetchBalanceResult result);

  void GetRewardsParametersCallback(
      ledger::mojom::RewardsParametersPtr parameters);

  RewardsService* rewards_service_ = nullptr;     // NOT OWNED
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
      "dialogReady", base::BindRepeating(&TipMessageHandler::DialogReady,
                                         base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getPublisherBanner",
      base::BindRepeating(&TipMessageHandler::GetPublisherBanner,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getUserType", base::BindRepeating(&TipMessageHandler::GetUserType,
                                         base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getRewardsParameters",
      base::BindRepeating(&TipMessageHandler::GetRewardsParameters,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "onTip",
      base::BindRepeating(&TipMessageHandler::OnTip, base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getRecurringTips",
      base::BindRepeating(&TipMessageHandler::GetRecurringTips,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getReconcileStamp",
      base::BindRepeating(&TipMessageHandler::GetReconcileStamp,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "tweetTip", base::BindRepeating(&TipMessageHandler::TweetTip,
                                      base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "fetchBalance", base::BindRepeating(&TipMessageHandler::FetchBalance,
                                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getExternalWallet",
      base::BindRepeating(&TipMessageHandler::GetExternalWallet,
                          base::Unretained(this)));
}

void TipMessageHandler::OnRecurringTipRemoved(RewardsService* rewards_service,
                                              bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  FireWebUIListener("recurringTipRemoved", base::Value(success));
}

void TipMessageHandler::OnRecurringTipSaved(RewardsService* rewards_service,
                                            bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  FireWebUIListener("recurringTipSaved", base::Value(success));
}

void TipMessageHandler::OnPendingContributionSaved(
    RewardsService* rewards_service,
    ledger::mojom::Result result) {
  FireWebUIListener("pendingContributionSaved",
                    base::Value(static_cast<int>(result)));
}

void TipMessageHandler::OnReconcileComplete(
    RewardsService* rewards_service,
    const ledger::mojom::Result result,
    const std::string& contribution_id,
    const double amount,
    const ledger::mojom::RewardsType type,
    const ledger::mojom::ContributionProcessor processor) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict data;
  data.Set("result", static_cast<int>(result));
  data.Set("type", static_cast<int>(type));

  FireWebUIListener("reconcileCompleted", data);
}

void TipMessageHandler::OnUnblindedTokensReady(
    RewardsService* rewards_service) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  FireWebUIListener("unblindedTokensReady");
}

void TipMessageHandler::DialogReady(const base::Value::List& args) {
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

void TipMessageHandler::GetPublisherBanner(const base::Value::List& args) {
  CHECK_EQ(1U, args.size());
  const std::string publisher_key = args[0].GetString();

  if (publisher_key.empty() || !rewards_service_) {
    return;
  }

  rewards_service_->GetPublisherBanner(
      publisher_key,
      base::BindOnce(&TipMessageHandler::GetPublisherBannerCallback,
                     weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::GetUserType(const base::Value::List& args) {
  if (!IsJavascriptAllowed() || !rewards_service_) {
    return;
  }
  rewards_service_->GetUserType(base::BindOnce(
      &TipMessageHandler::GetUserTypeCallback, weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::GetUserTypeCallback(ledger::mojom::UserType user_type) {
  FireWebUIListener("userTypeUpdated",
                    base::Value(static_cast<int>(user_type)));
}

void TipMessageHandler::GetRewardsParameters(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->GetRewardsParameters(
      base::BindOnce(&TipMessageHandler::GetRewardsParametersCallback,
                     weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::OnTip(const base::Value::List& args) {
  CHECK_EQ(3U, args.size());
  const std::string publisher_key = args[0].GetString();
  const double amount = args[1].GetDouble();
  const bool recurring = args[2].GetBool();

  if (publisher_key.empty() || !rewards_service_) {
    return;
  }

  if (recurring && amount <= 0) {
    rewards_service_->RemoveRecurringTip(publisher_key);
    OnTipCallback(0, ledger::mojom::Result::LEDGER_OK);
  } else if (amount > 0) {
    rewards_service_->OnTip(publisher_key, amount, recurring,
                            base::BindOnce(&TipMessageHandler::OnTipCallback,
                                           weak_factory_.GetWeakPtr(), amount));
  }
}

void TipMessageHandler::GetReconcileStamp(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->GetReconcileStamp(
      base::BindOnce(&TipMessageHandler::GetReconcileStampCallback,
                     weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::TweetTip(const base::Value::List& args) {
  CHECK_EQ(args.size(), 3U);
  const std::string name = args[0].GetString();
  const std::string tweet_id = args[1].GetString();
  const ledger::mojom::PublisherStatus status =
      static_cast<ledger::mojom::PublisherStatus>(args[2].GetInt());

  if (name.empty() || !rewards_service_) {
    return;
  }

  std::string comment;
  if (status == ledger::mojom::PublisherStatus::NOT_VERIFIED) {
    const std::u16string date =
        base::TimeFormatShortDate(base::Time::Now() + base::Days(90));
    const std::u16string hashtag = u"%23";
    comment = l10n_util::GetStringFUTF8(
        IDS_BRAVE_REWARDS_LOCAL_COMPLIMENT_TWEET_UNVERIFIED_PUBLISHER, hashtag,
        base::UTF8ToUTF16(name), hashtag, date);
  } else {
    comment = l10n_util::GetStringFUTF8(
        IDS_BRAVE_REWARDS_LOCAL_COMPLIMENT_TWEET, base::UTF8ToUTF16(name));
  }

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

void TipMessageHandler::FetchBalance(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }
  rewards_service_->FetchBalance(base::BindOnce(
      &TipMessageHandler::FetchBalanceCallback, weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::GetExternalWallet(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }
  rewards_service_->GetExternalWallet(base::BindOnce(
      &TipMessageHandler::OnGetExternalWallet, weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::GetRecurringTips(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }
  rewards_service_->GetRecurringTips(
      base::BindOnce(&TipMessageHandler::GetRecurringTipsCallback,
                     weak_factory_.GetWeakPtr()));
}

void TipMessageHandler::GetRewardsParametersCallback(
    ledger::mojom::RewardsParametersPtr parameters) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict data;
  if (parameters) {
    base::Value::List tip_choices;
    for (const auto& item : parameters->tip_choices) {
      tip_choices.Append(item);
    }

    base::Value::List monthly_choices;
    for (const auto& item : parameters->monthly_tip_choices) {
      monthly_choices.Append(item);
    }

    base::Value::List ac_choices;
    for (const auto& item : parameters->auto_contribute_choices) {
      ac_choices.Append(item);
    }

    base::Value::Dict payout_status;
    for (const auto& item : parameters->payout_status) {
      payout_status.Set(item.first, item.second);
    }

    data.Set("rate", parameters->rate);
    data.Set("tipChoices", std::move(tip_choices));
    data.Set("monthlyTipChoices", std::move(monthly_choices));
    data.Set("payoutStatus", std::move(payout_status));
  }

  FireWebUIListener("rewardsParametersUpdated", data);
}

void TipMessageHandler::GetRecurringTipsCallback(
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::List publishers;
  for (const auto& item : list) {
    base::Value::Dict publisher;
    publisher.Set("publisherKey", item->id);
    publisher.Set("amount", item->weight);
    publishers.Append(std::move(publisher));
  }

  FireWebUIListener("recurringTipsUpdated", publishers);
}

void TipMessageHandler::GetPublisherBannerCallback(
    ledger::mojom::PublisherBannerPtr banner) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict result;

  if (banner) {
    result.Set("publisherKey", banner->publisher_key);
    result.Set("title", banner->title);
    result.Set("name", banner->name);
    result.Set("description", banner->description);
    result.Set("background", banner->background);
    result.Set("logo", banner->logo);
    result.Set("provider", banner->provider);
    result.Set("status", static_cast<int>(banner->status));

    base::Value::Dict links;
    for (const auto& item : banner->links) {
      links.Set(item.first, item.second);
    }
    result.Set("links", std::move(links));

    FireWebUIListener("publisherBannerUpdated", result);
  }
}

void TipMessageHandler::OnTipCallback(double amount,
                                      ledger::mojom::Result result) {
  if (IsJavascriptAllowed()) {
    result == ledger::mojom::Result::LEDGER_OK
        ? FireWebUIListener("tipProcessed", base::Value(amount))
        : FireWebUIListener("tipFailed");
  }
}

void TipMessageHandler::GetReconcileStampCallback(uint64_t reconcile_stamp) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  const std::string stamp = base::NumberToString(reconcile_stamp);
  FireWebUIListener("reconcileStampUpdated", base::Value(stamp));
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
      gurl, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false));
}

void TipMessageHandler::FetchBalanceCallback(FetchBalanceResult result) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict data;
  if (result.has_value()) {
    const auto balance = std::move(result.value());

    base::Value::Dict value_balance;
    value_balance.Set("total", balance->total);
    value_balance.Set(
        "wallets",
        base::Value::Dict(std::move_iterator(balance->wallets.begin()),
                          std::move_iterator(balance->wallets.end())));
    data.SetByDottedPath("value.balance", std::move(value_balance));
  } else {
    data.Set("error", static_cast<int>(result.error()));
  }

  FireWebUIListener("balanceUpdated", std::move(data));
}

void TipMessageHandler::OnGetExternalWallet(GetExternalWalletResult result) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict data;
  if (auto wallet = std::move(result).value_or(nullptr)) {
    data.Set("type", wallet->type);
    data.Set("status", static_cast<int>(wallet->status));
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
