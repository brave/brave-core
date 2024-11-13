/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_brave_search_throttle.h"

#include <string>
#include <type_traits>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/permission_request_description.h"
#include "content/public/browser/permission_result.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom-shared.h"
#include "url/gurl.h"

namespace content {
class RenderFrameHost;
}  // namespace content

namespace ai_chat {

// static
std::unique_ptr<AIChatBraveSearchThrottle>
AIChatBraveSearchThrottle::MaybeCreateThrottleFor(
    base::OnceCallback<void(content::WebContents*)> open_leo_delegate,
    content::NavigationHandle* navigation_handle,
    AIChatService* ai_chat_service,
    PrefService* pref_service) {
  auto* web_contents = navigation_handle->GetWebContents();
  if (!web_contents) {
    return nullptr;
  }

  if (!open_leo_delegate || !ai_chat_service ||
      !IsAIChatEnabled(pref_service) ||
      !features::IsOpenAIChatFromBraveSearchEnabled() ||
      !IsOpenAIChatButtonFromBraveSearchURL(navigation_handle->GetURL())) {
    return nullptr;
  }

  return std::make_unique<AIChatBraveSearchThrottle>(
      std::move(open_leo_delegate), navigation_handle, ai_chat_service);
}

AIChatBraveSearchThrottle::AIChatBraveSearchThrottle(
    base::OnceCallback<void(content::WebContents*)> open_leo_delegate,
    content::NavigationHandle* handle,
    AIChatService* ai_chat_service)
    : content::NavigationThrottle(handle),
      open_ai_chat_delegate_(std::move(open_leo_delegate)),
      ai_chat_service_(ai_chat_service) {
  CHECK(open_ai_chat_delegate_);
  CHECK(ai_chat_service_);
}

AIChatBraveSearchThrottle::~AIChatBraveSearchThrottle() = default;

AIChatBraveSearchThrottle::ThrottleCheckResult
AIChatBraveSearchThrottle::WillStartRequest() {
  content::WebContents* web_contents = navigation_handle()->GetWebContents();
  if (!web_contents || !navigation_handle()->IsInPrimaryMainFrame() ||
      !IsOpenAIChatButtonFromBraveSearchURL(navigation_handle()->GetURL()) ||
      !IsBraveSearchURL(web_contents->GetLastCommittedURL())) {
    // Uninterested navigation for this throttle.
    return content::NavigationThrottle::PROCEED;
  }

  // Check if nonce in HTML tag matches the one in the URL.
  AIChatTabHelper::FromWebContents(web_contents)
      ->GetOpenAIChatButtonNonce(
          base::BindOnce(&AIChatBraveSearchThrottle::OnGetOpenAIChatButtonNonce,
                         weak_factory_.GetWeakPtr()));
  return content::NavigationThrottle::DEFER;
}

void AIChatBraveSearchThrottle::OpenAIChatWithStagedEntries() {
  content::WebContents* web_contents = navigation_handle()->GetWebContents();
  if (!web_contents) {
    return;
  }

  ai_chat_service_->OpenConversationWithStagedEntries(
      AIChatTabHelper::FromWebContents(web_contents)->GetWeakPtr(),
      base::BindOnce(&AIChatBraveSearchThrottle::OnOpenAIChat,
                     weak_factory_.GetWeakPtr()));
}

void AIChatBraveSearchThrottle::OnOpenAIChat() {
  std::move(open_ai_chat_delegate_).Run(navigation_handle()->GetWebContents());
}

void AIChatBraveSearchThrottle::OnGetOpenAIChatButtonNonce(
    const std::optional<std::string>& nonce) {
  if (!nonce || nonce->empty() ||
      *nonce != navigation_handle()->GetURL().ref()) {
    CancelDeferredNavigation(content::NavigationThrottle::CANCEL);
    return;
  }

  // Check if the user has granted permission to open AI Chat.
  content::WebContents* web_contents = navigation_handle()->GetWebContents();
  if (!web_contents) {
    CancelDeferredNavigation(content::NavigationThrottle::CANCEL);
    return;
  }

  content::RenderFrameHost* rfh = web_contents->GetPrimaryMainFrame();
  content::PermissionController* permission_controller =
      web_contents->GetBrowserContext()->GetPermissionController();
  content::PermissionResult permission_status =
      permission_controller->GetPermissionResultForCurrentDocument(
          blink::PermissionType::BRAVE_OPEN_AI_CHAT, rfh);

  if (permission_status.status == content::PermissionStatus::DENIED) {
    CancelDeferredNavigation(content::NavigationThrottle::CANCEL);
  } else if (permission_status.status == content::PermissionStatus::GRANTED) {
    OpenAIChatWithStagedEntries();
    CancelDeferredNavigation(content::NavigationThrottle::CANCEL);
  } else {  // ask
    permission_controller->RequestPermissionFromCurrentDocument(
        rfh,
        content::PermissionRequestDescription(
            blink::PermissionType::BRAVE_OPEN_AI_CHAT, /*user_gesture=*/true),
        base::BindOnce(&AIChatBraveSearchThrottle::OnPermissionPromptResult,
                       weak_factory_.GetWeakPtr()));
  }
}

void AIChatBraveSearchThrottle::OnPermissionPromptResult(
    content::PermissionStatus status) {
  if (status == content::PermissionStatus::GRANTED) {
    OpenAIChatWithStagedEntries();
  }

  CancelDeferredNavigation(content::NavigationThrottle::CANCEL);
}

const char* AIChatBraveSearchThrottle::GetNameForLogging() {
  return "AIChatBraveSearchThrottle";
}

}  // namespace ai_chat
