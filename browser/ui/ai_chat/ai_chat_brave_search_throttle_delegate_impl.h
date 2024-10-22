/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_AI_CHAT_AI_CHAT_BRAVE_SEARCH_THROTTLE_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_UI_AI_CHAT_AI_CHAT_BRAVE_SEARCH_THROTTLE_DELEGATE_IMPL_H_

#include "brave/components/ai_chat/content/browser/ai_chat_brave_search_throttle.h"

namespace content {
class WebContents;
}

namespace ai_chat {

class AIChatBraveSearchThrottleDelegateImpl
    : public AIChatBraveSearchThrottle::Delegate {
 public:
  AIChatBraveSearchThrottleDelegateImpl() = default;
  ~AIChatBraveSearchThrottleDelegateImpl() override = default;

  // AIChatBraveSearchThrottle::Delegate:
  void OpenLeo(content::WebContents* web_contents) override;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_AI_CHAT_AI_CHAT_BRAVE_SEARCH_THROTTLE_DELEGATE_IMPL_H_
