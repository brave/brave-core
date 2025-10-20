// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_BRIDGE_HOLDER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_BRIDGE_HOLDER_H_

#include "ios/web/public/lazy_web_state_user_data.h"

@protocol AIChatUIHandlerBridge;

namespace ai_chat {

// Some WebState user data that holds onto an AIChatUIHandlerBridge
class UIHandlerBridgeHolder
    : public web::LazyWebStateUserData<UIHandlerBridgeHolder> {
 public:
  void SetBridge(id<AIChatUIHandlerBridge> bridge) { bridge_ = bridge; }
  id<AIChatUIHandlerBridge> bridge() { return bridge_; }

 private:
  explicit UIHandlerBridgeHolder(web::WebState*);
  friend class web::LazyWebStateUserData<UIHandlerBridgeHolder>;
  __weak id<AIChatUIHandlerBridge> bridge_ = nullptr;
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_BRIDGE_HOLDER_H_
