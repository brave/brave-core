/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SCREEN_OPENER_BRIDGE_HOLDER_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SCREEN_OPENER_BRIDGE_HOLDER_H_

#include "ios/web/public/web_state_user_data.h"

@protocol BraveAccountScreenOpenerBridge;

namespace brave_account {

// WebStateUserData that holds onto the bridge for opening the screens a WebUI
// page asks for. Set by the host surface before the page is loaded; used when
// the page asks via `mojom::DialogOpener` or `mojom::SettingsOpener`.
class BraveAccountScreenOpenerBridgeHolder
    : public web::WebStateUserData<BraveAccountScreenOpenerBridgeHolder> {
 public:
  void SetBridge(id<BraveAccountScreenOpenerBridge> bridge) {
    bridge_ = bridge;
  }
  id<BraveAccountScreenOpenerBridge> bridge() { return bridge_; }

 private:
  explicit BraveAccountScreenOpenerBridgeHolder(web::WebState*);
  friend class web::WebStateUserData<BraveAccountScreenOpenerBridgeHolder>;
  __weak id<BraveAccountScreenOpenerBridge> bridge_ = nullptr;
};

}  // namespace brave_account

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SCREEN_OPENER_BRIDGE_HOLDER_H_
