/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/webui/brave_account/brave_account_ui.h"

#include "base/functional/bind.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/password_strength_meter/password_strength_meter.mojom.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "url/gurl.h"

BraveAccountUI::BraveAccountUI(web::WebUIIOS* web_ui, const GURL& url)
    : BraveAccountUIBase(ProfileIOS::FromWebUIIOS(web_ui)),
      web::WebUIIOSController(web_ui, url.host()) {
  using PageHandler = void (BraveAccountUIBase::*)(
      mojo::PendingReceiver<brave_account::mojom::PageHandler>);
  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(
          static_cast<PageHandler>(&BraveAccountUIBase::BindInterface),
          base::Unretained(this)));

  using PasswordStrengthMeter = void (BraveAccountUIBase::*)(
      mojo::PendingReceiver<
          password_strength_meter::mojom::PasswordStrengthMeter>);
  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(static_cast<PasswordStrengthMeter>(
                              &BraveAccountUIBase::BindInterface),
                          base::Unretained(this)));
}

BraveAccountUI::~BraveAccountUI() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      brave_account::mojom::PageHandler::Name_);

  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      password_strength_meter::mojom::PasswordStrengthMeter::Name_);
}
