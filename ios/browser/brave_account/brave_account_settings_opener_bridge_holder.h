/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_BRIDGE_HOLDER_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_BRIDGE_HOLDER_H_

#include "ios/web/public/web_state_user_data.h"

@protocol BraveAccountSettingsOpenerBridge;

namespace brave_account {

// WebStateUserData that holds onto the bridge for opening a settings page in
// place of the WebUI page. Set by the host surface before the page is loaded;
// used when the page asks via `mojom::SettingsOpener::OpenSettings()`.
class BraveAccountSettingsOpenerBridgeHolder
    : public web::WebStateUserData<BraveAccountSettingsOpenerBridgeHolder> {
 public:
  void SetBridge(id<BraveAccountSettingsOpenerBridge> bridge) {
    bridge_ = bridge;
  }
  id<BraveAccountSettingsOpenerBridge> bridge() { return bridge_; }

 private:
  explicit BraveAccountSettingsOpenerBridgeHolder(web::WebState*);
  friend class web::WebStateUserData<BraveAccountSettingsOpenerBridgeHolder>;
  __weak id<BraveAccountSettingsOpenerBridge> bridge_ = nullptr;
};

}  // namespace brave_account

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SETTINGS_OPENER_BRIDGE_HOLDER_H_
