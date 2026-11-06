/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_account/brave_account_screen_opener.h"

#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/ios/browser/brave_account/brave_account_screen_opener_bridge.h"
#include "brave/ios/browser/brave_account/brave_account_screen_opener_bridge_holder.h"
#include "ios/web/public/web_state.h"

namespace brave_account {

namespace {

id<BraveAccountScreenOpenerBridge> GetBridge(web::WebState& web_state) {
  auto* holder = BraveAccountScreenOpenerBridgeHolder::FromWebState(&web_state);
  return holder ? holder->bridge() : nil;
}

}  // namespace

void OpenBraveAccountDialog(web::WebState& web_state,
                            mojom::DialogMode dialog_mode) {
  [GetBridge(web_state)
      openBraveAccountDialogWithDialogMode:static_cast<BraveAccountDialogMode>(
                                               dialog_mode)];
}

void OpenSettingsPage(web::WebState& web_state, mojom::SettingsPage page) {
  [GetBridge(web_state)
      openSettingsPage:static_cast<BraveAccountSettingsPage>(page)];
}

}  // namespace brave_account
