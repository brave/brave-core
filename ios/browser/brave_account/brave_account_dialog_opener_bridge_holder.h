/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_OPENER_BRIDGE_HOLDER_H_
#define BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_OPENER_BRIDGE_HOLDER_H_

#include "ios/web/public/web_state_user_data.h"

@protocol BraveAccountDialogOpenerBridge;

namespace brave_account {

// WebStateUserData that holds onto the bridge for opening the Brave Account
// dialog over the WebUI page. Set by the host surface before the page is
// loaded; used when the page asks via
// `mojom::DialogOpener::OpenDialog()`.
class BraveAccountDialogOpenerBridgeHolder
    : public web::WebStateUserData<BraveAccountDialogOpenerBridgeHolder> {
 public:
  void SetBridge(id<BraveAccountDialogOpenerBridge> bridge) {
    bridge_ = bridge;
  }
  id<BraveAccountDialogOpenerBridge> bridge() { return bridge_; }

 private:
  explicit BraveAccountDialogOpenerBridgeHolder(web::WebState*);
  friend class web::WebStateUserData<BraveAccountDialogOpenerBridgeHolder>;
  __weak id<BraveAccountDialogOpenerBridge> bridge_ = nullptr;
};

}  // namespace brave_account

#endif  // BRAVE_IOS_BROWSER_BRAVE_ACCOUNT_BRAVE_ACCOUNT_DIALOG_OPENER_BRIDGE_HOLDER_H_
