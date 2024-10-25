/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_THROTTLE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_THROTTLE_H_

#include <memory>

#include "content/public/browser/navigation_throttle.h"

namespace ai_chat {

// Prevents navigation to certain AI Chat URLs
class AiChatThrottle : public content::NavigationThrottle {
 public:
  explicit AiChatThrottle(content::NavigationHandle* handle);
  ~AiChatThrottle() override;

  static std::unique_ptr<AiChatThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* navigation_handle);

  // content::NavigationThrottle:
  // ThrottleCheckResult WillProcessResponse() override;
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_THROTTLE_H_
