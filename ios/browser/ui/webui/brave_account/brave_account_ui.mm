/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/ui/webui/brave_account/brave_account_ui.h"

#include "base/functional/bind.h"
#include "brave/components/password_strength_meter/password_strength_meter.mojom.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "url/gurl.h"

BraveAccountUI::BraveAccountUI(web::WebUIIOS* web_ui, const GURL& url)
    : BraveAccountUIBase(ProfileIOS::FromWebUIIOS(web_ui)),
      web::WebUIIOSController(web_ui, url.host()) {
  web_ui->GetWebState()->GetInterfaceBinderForMainFrame()->AddInterface(
      base::BindRepeating(&BraveAccountUIBase::BindInterface,
                          base::Unretained(this)));
}

BraveAccountUI::~BraveAccountUI() {
  web_ui()->GetWebState()->GetInterfaceBinderForMainFrame()->RemoveInterface(
      password_strength_meter::mojom::PasswordStrengthMeter::Name_);
}
