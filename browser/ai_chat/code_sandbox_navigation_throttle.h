// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_CODE_SANDBOX_NAVIGATION_THROTTLE_H_
#define BRAVE_BROWSER_AI_CHAT_CODE_SANDBOX_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationThrottleRegistry;
}

namespace ai_chat {

class CodeSandboxNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit CodeSandboxNavigationThrottle(
      content::NavigationThrottleRegistry& registry);
  ~CodeSandboxNavigationThrottle() override;

  CodeSandboxNavigationThrottle(const CodeSandboxNavigationThrottle&) = delete;
  CodeSandboxNavigationThrottle& operator=(
      const CodeSandboxNavigationThrottle&) = delete;

  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry);

 private:
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_CODE_SANDBOX_NAVIGATION_THROTTLE_H_
