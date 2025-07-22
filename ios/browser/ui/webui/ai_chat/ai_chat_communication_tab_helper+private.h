// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_COMMUNICATION_TAB_HELPER_PRIVATE_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_COMMUNICATION_TAB_HELPER_PRIVATE_H_

#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_communication_tab_helper.h"

namespace web {
class WebState;
}  // namespace web

NS_ASSUME_NONNULL_BEGIN

@interface AIChatCommunicationController ()
+ (void)createForWebState:(web::WebState*)webState;
+ (nullable AIChatCommunicationController*)fromWebState:
    (web::WebState*)webState;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_COMMUNICATION_TAB_HELPER_PRIVATE_H_
