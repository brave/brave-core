// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_PAGE_HANDLER_BRIDGE_HOLDER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_PAGE_HANDLER_BRIDGE_HOLDER_H_

#include "ios/web/public/web_state_user_data.h"

@protocol TransferPageHandlerBridge;

namespace brave_transfer {

// WebState user data that holds the Swift TransferPageHandlerBridge for a
// tab. Set through -[BraveWebView setTransferPageHandler:].
class PageHandlerBridgeHolder
    : public web::WebStateUserData<PageHandlerBridgeHolder> {
 public:
  void SetBridge(id<TransferPageHandlerBridge> bridge) { bridge_ = bridge; }
  id<TransferPageHandlerBridge> bridge() { return bridge_; }

 private:
  explicit PageHandlerBridgeHolder(web::WebState*);
  friend class web::WebStateUserData<PageHandlerBridgeHolder>;
  __weak id<TransferPageHandlerBridge> bridge_ = nullptr;
};

}  // namespace brave_transfer

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_BRAVE_TRANSFER_BRAVE_TRANSFER_PAGE_HANDLER_BRIDGE_HOLDER_H_
