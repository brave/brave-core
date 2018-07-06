/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_ui.h"

#include "brave/browser/payments/payments_service.h"
#include "brave/browser/payments/payments_service_factory.h"
#include "brave/common/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "content/public/common/bindings_policy.h"


using content::WebUIMessageHandler;

namespace {

// The handler for Javascript messages for Brave about: pages
class RewardsDOMHandler : public WebUIMessageHandler {
 public:
  RewardsDOMHandler() {
  }
  ~RewardsDOMHandler() override {}

  void Init();

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  void HandleCreateWalletRequested(const base::ListValue* args);
  void OnWalletCreated();
  void OnWalletCreateFailed();
  DISALLOW_COPY_AND_ASSIGN(RewardsDOMHandler);
};

void RewardsDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback("createWalletRequested",
      base::BindRepeating(&RewardsDOMHandler::HandleCreateWalletRequested,
                          base::Unretained(this)));
}

void RewardsDOMHandler::Init() {
}

void RewardsDOMHandler::HandleCreateWalletRequested(const base::ListValue* args) {
#if defined(BRAVE_PAYMENTS_ENABLED)
  Profile* profile = Profile::FromWebUI(web_ui());
  payments::PaymentsService* payments_service =
      PaymentsServiceFactory::GetForProfile(profile);

  if (payments_service) {
    payments_service->CreateWallet();
  }

  // TODO(bbondy): Use an observer or client override for when the wallet is actually
  // created once the native-ledger library supports it.
  OnWalletCreated();
#else
  OnWalletCreateFailed();
#endif
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
