/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/webui/brave_account/brave_account_ui_ios.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/password_strength_meter/password_strength_meter.mojom.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "url/gurl.h"

BraveAccountUIIOS::BraveAccountUIIOS(web::WebUIIOS* web_ui, const GURL& url)
    : BraveAccountUIBase(ProfileIOS::FromWebUIIOS(web_ui), url),
      web::WebUIIOSController(web_ui, url.GetHost()) {
  AddInterface<brave_account::mojom::Authentication>();
  AddInterface<brave_account::mojom::DialogController>();
  AddInterface<password_strength_meter::mojom::PasswordStrengthMeter>();
}

BraveAccountUIIOS::~BraveAccountUIIOS() {
  RemoveInterface<brave_account::mojom::Authentication>();
  RemoveInterface<brave_account::mojom::DialogController>();
  RemoveInterface<password_strength_meter::mojom::PasswordStrengthMeter>();
}

void BraveAccountUIIOS::CloseDialog() {
  web_ui()->GetWebState()->CloseWebState();
}

void BraveAccountUIIOS::BindInterface(
    mojo::PendingReceiver<brave_account::mojom::DialogController>
        pending_receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(pending_receiver));
}

template <typename Interface>
void BraveAccountUIIOS::AddInterface() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(static_cast<void (BraveAccountUIIOS::*)(
                              mojo::PendingReceiver<Interface>)>(
                              &BraveAccountUIIOS::BindInterface),
                          base::Unretained(this)));
}

template <typename Interface>
void BraveAccountUIIOS::RemoveInterface() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      Interface::Name_);
}
