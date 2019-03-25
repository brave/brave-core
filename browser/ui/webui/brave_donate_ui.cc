/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_donate_ui.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ui/webui/basic_ui.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/brave_donate_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/publisher_banner.h"

using content::WebUIMessageHandler;

namespace {

// The handler for Javascript messages for Brave about: pages
class RewardsDonateDOMHandler : public WebUIMessageHandler,
                                public brave_rewards::RewardsServiceObserver {
 public:
  RewardsDonateDOMHandler();
  ~RewardsDonateDOMHandler() override;

  void Init();

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void GetPublisherDonateData(const base::ListValue* args);
  void GetWalletProperties(const base::ListValue* args);
  void OnDonate(const base::ListValue* args);
  void GetRecurringDonations(const base::ListValue* args);
  void GetReconcileStamp(const base::ListValue* args);
  void OnReconcileStamp(uint64_t reconcile_stamp);
  void OnPublisherBanner(
      std::unique_ptr<brave_rewards::PublisherBanner> banner);

  // RewardsServiceObserver implementation
  void OnWalletProperties(
      brave_rewards::RewardsService* rewards_service,
      int error_code,
      std::unique_ptr<brave_rewards::WalletProperties> wallet_properties)
      override;
  void OnRecurringDonationUpdated(
      brave_rewards::RewardsService* rewards_service,
      brave_rewards::ContentSiteList) override;

  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED
  base::WeakPtrFactory<RewardsDonateDOMHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RewardsDonateDOMHandler);
};

RewardsDonateDOMHandler::RewardsDonateDOMHandler() : weak_factory_(this) {}

RewardsDonateDOMHandler::~RewardsDonateDOMHandler() {
  if (rewards_service_)
    rewards_service_->RemoveObserver(this);
}

void RewardsDonateDOMHandler::Init() {
  Profile* profile = Profile::FromWebUI(web_ui());
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_)
    rewards_service_->AddObserver(this);
}

void RewardsDonateDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "brave_rewards_donate.getPublisherBanner",
      base::BindRepeating(&RewardsDonateDOMHandler::GetPublisherDonateData,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_donate.getWalletProperties",
      base::BindRepeating(&RewardsDonateDOMHandler::GetWalletProperties,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_donate.onDonate",
       base::BindRepeating(&RewardsDonateDOMHandler::OnDonate,
                           base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_donate.getRecurringDonations",
      base::BindRepeating(&RewardsDonateDOMHandler::GetRecurringDonations,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_donate.getReconcileStamp",
      base::BindRepeating(&RewardsDonateDOMHandler::GetReconcileStamp,
                          base::Unretained(this)));
}

void RewardsDonateDOMHandler::GetPublisherDonateData(
    const base::ListValue* args) {
  std::string publisher_key;
  args->GetString(0, &publisher_key);
  rewards_service_->GetPublisherBanner(
      publisher_key,
      base::Bind(&RewardsDonateDOMHandler::OnPublisherBanner,
                 weak_factory_.GetWeakPtr()));
}

void RewardsDonateDOMHandler::GetWalletProperties(const base::ListValue* args) {
  if (!rewards_service_)
    return;

  rewards_service_->FetchWalletProperties();
}

void RewardsDonateDOMHandler::OnWalletProperties(
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

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_donate.walletProperties", result);
}

void RewardsDonateDOMHandler::OnDonate(const base::ListValue* args) {
  if (!rewards_service_ || !args)
    return;

  std::string publisher_key;
  int amount;
  bool recurring;
  args->GetString(0, &publisher_key);
  args->GetInteger(1, &amount);
  args->GetBoolean(2, &recurring);

  if (publisher_key.empty() || amount < 1) {
    // TODO(nejczdovc) add error
    return;
  }

  rewards_service_->OnDonate(publisher_key, amount, recurring);
}

void RewardsDonateDOMHandler::GetRecurringDonations(
    const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->UpdateRecurringDonationsList();
  }
}

void RewardsDonateDOMHandler::OnRecurringDonationUpdated(
    brave_rewards::RewardsService* rewards_service,
    const brave_rewards::ContentSiteList list) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  auto publishers = std::make_unique<base::ListValue>();
  for (auto const& item : list) {
    auto publisher = std::make_unique<base::DictionaryValue>();
    publisher->SetString("publisherKey", item.id);
    publisher->SetInteger("monthlyDate", item.reconcile_stamp);
    publishers->Append(std::move(publisher));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_donate.recurringDonations", *publishers);
}

void RewardsDonateDOMHandler::OnPublisherBanner(
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
    result.SetBoolean("verified", banner->verified);

    auto amounts = std::make_unique<base::ListValue>();
    for (int const& value : banner->amounts) {
      amounts->AppendInteger(value);
    }
    result.SetList("amounts", std::move(amounts));

    auto social = std::make_unique<base::DictionaryValue>();
    for (auto const& item : banner->social) {
      social->SetString(item.first, item.second);
    }
    result.SetDictionary("social", std::move(social));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_donate.publisherBanner", result);
}

}  // namespace

BraveDonateUI::BraveDonateUI(content::WebUI* web_ui, const std::string& name)
    : ConstrainedWebDialogUI(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);
  // Show error for non-supported contexts
  if (profile->IsOffTheRecord()) {
    return;
  }
  content::WebUIDataSource* data_source = CreateBasicUIHTMLSource(profile,
                                              name,
                                              kBraveDonateGenerated,
                                              kBraveDonateGeneratedSize,
                                              IDR_BRAVE_DONATE_HTML);
  content::WebUIDataSource::Add(profile, data_source);

  auto handler_owner = std::make_unique<RewardsDonateDOMHandler>();
  RewardsDonateDOMHandler * handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

void RewardsDonateDOMHandler::GetReconcileStamp(const base::ListValue *args) {
  if (rewards_service_) {
    rewards_service_->GetReconcileStamp(base::Bind(
          &RewardsDonateDOMHandler::OnReconcileStamp,
          weak_factory_.GetWeakPtr()));
  }
}

void RewardsDonateDOMHandler::OnReconcileStamp(uint64_t reconcile_stamp) {
  if (!web_ui()->CanCallJavascript()) {
     return;
  }

  const std::string stamp = std::to_string(reconcile_stamp);
  web_ui()->CallJavascriptFunctionUnsafe("brave_rewards_donate.reconcileStamp",
      base::Value(stamp));
}

BraveDonateUI::~BraveDonateUI() {
}
