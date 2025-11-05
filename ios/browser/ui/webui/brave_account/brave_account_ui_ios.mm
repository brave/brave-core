/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/webui/brave_account/brave_account_ui_ios.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/password_strength_meter/password_strength_meter.mojom.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "ios/web/public/webui/web_ui_ios_message_handler.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "url/gurl.h"

namespace {

class BraveAccountUIMessageHandler : public web::WebUIIOSMessageHandler {
 public:
  BraveAccountUIMessageHandler() = default;

  BraveAccountUIMessageHandler(const BraveAccountUIMessageHandler&) = delete;
  BraveAccountUIMessageHandler& operator=(const BraveAccountUIMessageHandler&) =
      delete;

  ~BraveAccountUIMessageHandler() override = default;

 private:
  // WebUIIOSMessageHandler:
  void RegisterMessages() override;

  void OnDialogCloseMessage(const base::Value::List& args);
};

void BraveAccountUIMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "dialogClose",
      base::BindRepeating(&BraveAccountUIMessageHandler::OnDialogCloseMessage,
                          base::Unretained(this)));
}

void BraveAccountUIMessageHandler::OnDialogCloseMessage(
    const base::Value::List& args) {
  web_ui()->GetWebState()->CloseWebState();
}

}  // namespace

BraveAccountUIIOS::BraveAccountUIIOS(web::WebUIIOS* web_ui, const GURL& url)
    : BraveAccountUIBase(ProfileIOS::FromWebUIIOS(web_ui)),
      web::WebUIIOSController(web_ui, url.host()) {
  using Authentication = void (BraveAccountUIBase::*)(
      mojo::PendingReceiver<brave_account::mojom::Authentication>);
  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(
          static_cast<Authentication>(&BraveAccountUIBase::BindInterface),
          base::Unretained(this)));

  using PasswordStrengthMeter = void (BraveAccountUIBase::*)(
      mojo::PendingReceiver<
          password_strength_meter::mojom::PasswordStrengthMeter>);
  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(static_cast<PasswordStrengthMeter>(
                              &BraveAccountUIBase::BindInterface),
                          base::Unretained(this)));

  web_ui->AddMessageHandler(std::make_unique<BraveAccountUIMessageHandler>());
}

BraveAccountUIIOS::~BraveAccountUIIOS() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      brave_account::mojom::Authentication::Name_);

  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      password_strength_meter::mojom::PasswordStrengthMeter::Name_);
}
