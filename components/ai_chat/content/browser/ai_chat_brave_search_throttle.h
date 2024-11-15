/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_BRAVE_SEARCH_THROTTLE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_BRAVE_SEARCH_THROTTLE_H_

#include <memory>
#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/permission_result.h"

namespace blink {
namespace mojom {
enum class PermissionStatus : int32_t;
}  // namespace mojom
}  // namespace blink

namespace content {
class WebContents;
class NavigationHandle;
}

class PrefService;

namespace ai_chat {
class AIChatService;

// A network throttle which intercepts Brave Search requests.
// Currently the only use case is to intercept requests to open Leo AI chat, so
// it is only created when navigating to open Leo button URL from Brave Search.
// It could be extended to other Brave Search URLs in the future.
//
// For Open Leo feature, we check:
// 1) If AI chat is enabled.
// 2) If the request is from Brave Search and is trying to navigate to open Leo
// button URL.
// 3) If the nonce property in the a tag element is equal to the one in url ref.
// 4) If the user has granted permission to open Leo.
// The navigation to the specific Open Leo URL will be cancelled, and Leo AI
// chat will be opened only if all the above conditions are met.
class AIChatBraveSearchThrottle : public content::NavigationThrottle {
 public:
  AIChatBraveSearchThrottle(
      base::OnceCallback<void(content::WebContents*)> open_leo_delegate,
      content::NavigationHandle* handle,
      AIChatService* ai_chat_service);
  ~AIChatBraveSearchThrottle() override;

  static std::unique_ptr<AIChatBraveSearchThrottle> MaybeCreateThrottleFor(
      base::OnceCallback<void(content::WebContents*)> open_leo_delegate,
      content::NavigationHandle* navigation_handle,
      AIChatService* ai_chat_service,
      PrefService* pref_service);

  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  void OnGetOpenAIChatButtonNonce(const std::optional<std::string>& nonce);
  void OnPermissionPromptResult(blink::mojom::PermissionStatus status);
  void OnOpenAIChat();

  void OpenAIChatWithStagedEntries();

  base::OnceCallback<void(content::WebContents*)> open_ai_chat_delegate_;
  const raw_ptr<AIChatService> ai_chat_service_ = nullptr;

  base::WeakPtrFactory<AIChatBraveSearchThrottle> weak_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_BRAVE_SEARCH_THROTTLE_H_
