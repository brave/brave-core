/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_account/brave_account_settings_opener.h"

#include "brave/ios/browser/brave_account/brave_account_settings_opener_bridge.h"
#include "brave/ios/browser/brave_account/brave_account_settings_opener_bridge_holder.h"
#include "ios/web/public/web_state.h"

namespace brave_account {

void OpenSettingsPage(web::WebState& web_state, mojom::SettingsPage page) {
  auto* holder =
      BraveAccountSettingsOpenerBridgeHolder::FromWebState(&web_state);
  if (!holder) {
    return;
  }

  [holder->bridge()
      openSettingsPage:static_cast<BraveAccountSettingsPage>(page)];
}

}  // namespace brave_account
