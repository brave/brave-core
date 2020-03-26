/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_checkout_ui.h"

#include <memory>

#include "base/values.h"
#include "brave/browser/ui/webui/basic_ui.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_checkout_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace {

class CheckoutMessageHandler : public content::WebUIMessageHandler {
 public:
  CheckoutMessageHandler();
  ~CheckoutMessageHandler() override;

  void RegisterMessages() override;

  DISALLOW_COPY_AND_ASSIGN(CheckoutMessageHandler);

 private:
  void InitializeCallback(const base::ListValue* args);
};

CheckoutMessageHandler::CheckoutMessageHandler() {}

CheckoutMessageHandler::~CheckoutMessageHandler() {}

void CheckoutMessageHandler::RegisterMessages() {
  // TODO(zenparsing)
  web_ui()->RegisterMessageCallback("batPayments.initialize",
      base::BindRepeating(
          &CheckoutMessageHandler::InitializeCallback,
          base::Unretained(this)));
}

void CheckoutMessageHandler::InitializeCallback(
    const base::ListValue* args) {
  AllowJavascript();
  if (args->GetSize() == 0) {
    return;
  }
  base::Value result(true);
  ResolveJavascriptCallback(args->GetList()[0], result);
}


}  // namespace

BraveCheckoutUI::BraveCheckoutUI(
    content::WebUI* web_ui,
    const std::string& name)
    : ConstrainedWebDialogUI(web_ui) {
  Profile* profile = Profile::FromWebUI(web_ui);

  // TODO(zenparsing): Do we need this?
  /*
  // Show error for non-supported contexts
  if (profile->IsOffTheRecord()) {
    return;
  }
  */
  content::WebUIDataSource::Add(profile,
      CreateBasicUIHTMLSource(
          profile,
          name,
          kBraveRewardsCheckoutGenerated,
          kBraveRewardsCheckoutGeneratedSize,
          IDR_BRAVE_REWARDS_CHECKOUT_HTML));

  web_ui->AddMessageHandler(std::make_unique<CheckoutMessageHandler>());
}

BraveCheckoutUI::~BraveCheckoutUI() {}
