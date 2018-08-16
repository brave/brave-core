/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_ui.h"

#include "base/base64.h"

#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
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
                          public brave_rewards::RewardsServiceObserver {
 public:
  RewardsDOMHandler() {};
  ~RewardsDOMHandler() override;

  void Init();

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleCreateWalletRequested(const base::ListValue* args);
  void GetWalletProperties(const base::ListValue* args);
  void GetPromotion(const base::ListValue* args);
  void GetPromotionCaptcha(const base::ListValue* args);
  void GetWalletPassphrase(const base::ListValue* args);
  void RecoverWallet(const base::ListValue* args);
  void SolvePromotionCaptcha(const base::ListValue* args);
  void GetReconcileStamp(const base::ListValue* args);

  // PaymentServiceObserver implementation
  void OnWalletInitialized(brave_rewards::RewardsService* payment_service,
                       int error_code) override;
  void OnWalletProperties(brave_rewards::RewardsService* payment_service,
      int error_code,
      std::unique_ptr<brave_rewards::WalletProperties> wallet_properties) override;
  void OnPromotion(brave_rewards::RewardsService* payment_service,
                   brave_rewards::Promotion result) override;
  void OnPromotionCaptcha(brave_rewards::RewardsService* payment_service,
                          std::string image) override;
  void OnRecoverWallet(brave_rewards::RewardsService* payment_service,
                       unsigned int result,
                       double balance) override;
  void OnPromotionFinish(brave_rewards::RewardsService* payment_service,
                       unsigned int result,
                       unsigned int statusCode,
                       uint64_t expirationDate) override;

  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED

  DISALLOW_COPY_AND_ASSIGN(RewardsDOMHandler);
};

RewardsDOMHandler::~RewardsDOMHandler() {
  if (rewards_service_)
    rewards_service_->RemoveObserver(this);
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
  web_ui()->RegisterMessageCallback("getPromotionCaptcha",
                                    base::BindRepeating(&RewardsDOMHandler::GetPromotionCaptcha,
                                                        base::Unretained(this)));
  web_ui()->RegisterMessageCallback("getWalletPassphrase",
                                    base::BindRepeating(&RewardsDOMHandler::GetWalletPassphrase,
                                                        base::Unretained(this)));
  web_ui()->RegisterMessageCallback("recoverWallet",
                                    base::BindRepeating(&RewardsDOMHandler::RecoverWallet,
                                                        base::Unretained(this)));
  web_ui()->RegisterMessageCallback("solvePromotionCaptcha",
                                    base::BindRepeating(&RewardsDOMHandler::SolvePromotionCaptcha,
                                                        base::Unretained(this)));
  web_ui()->RegisterMessageCallback("getReconcileStamp",
                                    base::BindRepeating(&RewardsDOMHandler::GetReconcileStamp,
                                                        base::Unretained(this)));
}

void RewardsDOMHandler::Init() {
  Profile* profile = Profile::FromWebUI(web_ui());
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_)
    rewards_service_->AddObserver(this);
}

void RewardsDOMHandler::HandleCreateWalletRequested(const base::ListValue* args) {
  if (!rewards_service_)
    return;

  rewards_service_->CreateWallet();
}

void RewardsDOMHandler::GetWalletProperties(const base::ListValue* args) {
  if (!rewards_service_)
    return;

  rewards_service_->GetWalletProperties();
}

void RewardsDOMHandler::OnWalletInitialized(
    brave_rewards::RewardsService* payment_service,
    int error_code) {
  if (0 == (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI))
    return;

  if (error_code == 0)
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletCreated");
  else
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletCreateFailed");
}

void RewardsDOMHandler::OnWalletProperties(
    brave_rewards::RewardsService* payment_service,
    int error_code,
    std::unique_ptr<brave_rewards::WalletProperties> wallet_properties) {

  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
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

      // TODO add grants
    }

    result.SetDictionary("wallet", std::move(walletInfo));

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.walletProperties", result);
  }
}

void RewardsDOMHandler::OnPromotion(
    brave_rewards::RewardsService* payment_service,
    brave_rewards::Promotion result) {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    base::DictionaryValue* promotion = new base::DictionaryValue();
    promotion->SetString("promotionId", result.promotionId);
    promotion->SetDouble("amount", result.amount);

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.promotion", *promotion);
  }
}

void RewardsDOMHandler::GetPromotion(const base::ListValue* args) {
  if (rewards_service_) {
    std::string lang;
    std::string paymentId;
    args->GetString(0, &lang);
    args->GetString(1, &paymentId);
    rewards_service_->GetPromotion(lang, paymentId);
  }
}

void RewardsDOMHandler::OnPromotionCaptcha(
    brave_rewards::RewardsService* payment_service,
    std::string image) {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    std::string encoded_string;
    base::Value chunkValue;
    base::Base64Encode(image, &encoded_string);
    chunkValue = base::Value(std::move(encoded_string));
    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.promotionCaptcha", chunkValue);
  }
}

void RewardsDOMHandler::GetPromotionCaptcha(const base::ListValue* args) {
  if (rewards_service_) {
    rewards_service_->GetPromotionCaptcha();
  }
}

void RewardsDOMHandler::GetWalletPassphrase(const base::ListValue* args) {
  if (rewards_service_ && 0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
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
    brave_rewards::RewardsService* payment_service,
    unsigned int result,
    double balance) {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    base::DictionaryValue* recover = new base::DictionaryValue();
    recover->SetInteger("result", result);
    recover->SetDouble("balance", balance);

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.recoverWalletData", *recover);
  }
}

void RewardsDOMHandler::SolvePromotionCaptcha(const base::ListValue *args) {
  if (rewards_service_) {
    std::string solution;
    args->GetString(0, &solution);
    rewards_service_->SolvePromotionCaptcha(solution);
  }
}

void RewardsDOMHandler::OnPromotionFinish(
    brave_rewards::RewardsService* payment_service,
    unsigned int result,
    unsigned int statusCode,
    uint64_t expirationDate) {
  if (0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    base::DictionaryValue* finish = new base::DictionaryValue();
    finish->SetInteger("result", result);
    finish->SetInteger("statusCode", statusCode);
    finish->SetInteger("expirationDate", expirationDate);

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.promotionFinish", *finish);
  }
}

void RewardsDOMHandler::GetReconcileStamp(const base::ListValue* args) {
  if (rewards_service_ && 0 != (web_ui()->GetBindings() & content::BINDINGS_POLICY_WEB_UI)) {
    std::string stamp = std::to_string(rewards_service_->GetReconcileStamp());

    web_ui()->CallJavascriptFunctionUnsafe("brave_rewards.reconcileStamp", base::Value(stamp));
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
