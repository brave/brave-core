/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_ui.h"

#include "brave/browser/payments/payments_service.h"
#include "brave/browser/payments/wallet_properties.h"
#include "brave/browser/payments/payments_service_factory.h"
#include "brave/browser/payments/payments_service_observer.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
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
                          public payments::PaymentsServiceObserver {
 public:
  RewardsDOMHandler() {};
  ~RewardsDOMHandler() override;

  void Init();

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleCreateWalletRequested(const base::ListValue* args);
  void OnWalletCreated();
  void OnWalletCreateFailed();
  void GetWalletProperties(const base::ListValue* args);
  void GetPromotion(const base::ListValue* args);
  void OnWalletProperties(payments::WalletProperties result);
  void OnPromotion(payments::Promotion result);

  // PaymentServiceObserver implementation
  void OnWalletCreated(payments::PaymentsService* payment_service,
                       int error_code) override;
  void OnWalletProperties(payments::PaymentsService* payment_service,
                       payments::WalletProperties result) override;
  void OnPromotion(payments::PaymentsService* payment_service,
                       payments::Promotion result) override;

  payments::PaymentsService* payments_service_;  // NOT OWNED

  DISALLOW_COPY_AND_ASSIGN(RewardsDOMHandler);
};

RewardsDOMHandler::~RewardsDOMHandler() {
  if (payments_service_)
    payments_service_->RemoveObserver(this);
}

void RewardsDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("createWalletRequested",
      base::BindRepeating(&RewardsDOMHandler::HandleCreateWalletRequested,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback("getWalletProperties",
      base::BindRepeating(&RewardsDOMHandler::GetWalletProperties,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback("getPromotion",
                                    base::BindRepeating(&RewardsDOMHandler::GetPromotion,
                                                        base::Unretained(this)));
}

void RewardsDOMHandler::Init() {
  Profile* profile = Profile::FromWebUI(web_ui());
  payments_service_ = PaymentsServiceFactory::GetForProfile(profile);
  if (payments_service_)
    payments_service_->AddObserver(this);
}

void RewardsDOMHandler::HandleCreateWalletRequested(const base::ListValue* args) {
  if (payments_service_) {
    payments_service_->CreateWallet();
  } else {
    OnWalletCreateFailed();
  }
}

void RewardsDOMHandler::GetWalletProperties(const base::ListValue* args) {
  if (payments_service_) {
    payments_service_->GetWalletProperties();
  }
}

void RewardsDOMHandler::OnWalletCreated(
    payments::PaymentsService* payment_service,
    int error_code) {
  if (error_code == 0)
    OnWalletCreated();
  else
    OnWalletCreateFailed();
}

void RewardsDOMHandler::OnWalletCreated() {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletCreated");
  }
}

void RewardsDOMHandler::OnWalletCreateFailed() {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletCreateFailed");
  }
}

void RewardsDOMHandler::OnWalletProperties(
    payments::PaymentsService* payment_service,
    payments::WalletProperties result) {
  OnWalletProperties(result);
}

void RewardsDOMHandler::OnWalletProperties(payments::WalletProperties result) {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    base::DictionaryValue* walletInfo = new base::DictionaryValue();
    walletInfo->SetDouble("balance", result.balance);
    walletInfo->SetString("probi", result.probi);

    base::DictionaryValue* rates = new base::DictionaryValue();
    for (auto const& rate : result.rates) {
      rates->SetDouble(rate.first, rate.second);
    }
    walletInfo->SetDictionary("rates", std::unique_ptr<base::DictionaryValue>(rates));

    auto choices (std::make_unique<base::ListValue>());
    for (double const& choice : result.parameters_choices) {
      choices->AppendDouble(choice);
    }
    walletInfo->SetList("choices", std::move(choices));

    auto range (std::make_unique<base::ListValue>());
    for (double const& value : result.parameters_range) {
      range->AppendDouble(value);
    }
    walletInfo->SetList("range", std::move(range));

    // TODO add grants

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletProperties", *walletInfo);
  }
}

void RewardsDOMHandler::OnPromotion(
    payments::PaymentsService* payment_service,
    payments::Promotion result) {
  OnPromotion(result);
}

void RewardsDOMHandler::GetPromotion(const base::ListValue* args) {
  if (payments_service_) {
    std::string lang;
    std::string paymentId;
    args->GetString(0, &lang);
    args->GetString(1, &paymentId);
    payments_service_->GetPromotion(lang, paymentId);
  }
}

void RewardsDOMHandler::OnPromotion(payments::Promotion result) {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    base::DictionaryValue* promotion = new base::DictionaryValue();
    promotion->SetString("promotionId", result.promotionId);
    promotion->SetDouble("amount", result.amount);

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.promotion", *promotion);
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
