// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_HANDLER_BRIDGE_HOLDER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_HANDLER_BRIDGE_HOLDER_H_

#include "ios/web/public/lazy_web_state_user_data.h"

@protocol WalletPageHandlerBridge;

namespace brave_wallet {

// Some WebState user data that holds onto an WalletPageHandlerBridge
class PageHandlerBridgeHolder
    : public web::LazyWebStateUserData<PageHandlerBridgeHolder> {
 public:
  void SetBridge(id<WalletPageHandlerBridge> bridge) { bridge_ = bridge; }
  id<WalletPageHandlerBridge> bridge() { return bridge_; }

 private:
  explicit PageHandlerBridgeHolder(web::WebState*);
  friend class web::LazyWebStateUserData<PageHandlerBridgeHolder>;
  __weak id<WalletPageHandlerBridge> bridge_ = nullptr;
};

}  // namespace brave_wallet

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_WALLET_WALLET_PAGE_HANDLER_BRIDGE_HOLDER_H_
