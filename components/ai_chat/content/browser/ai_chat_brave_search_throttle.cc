/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/content/browser/ai_chat_brave_search_throttle.h"

#include <memory>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/content/browser/page_content_fetcher.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/utils.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/permission_request_description.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

// static
std::unique_ptr<AIChatBraveSearchThrottle>
AIChatBraveSearchThrottle::MaybeCreateThrottleFor(
    std::unique_ptr<Delegate> delegate,
    content::NavigationHandle* navigation_handle,
    AIChatService* ai_chat_service) {
  auto* web_contents = navigation_handle->GetWebContents();
  if (!web_contents) {
    return nullptr;
  }

  if (!delegate || !ai_chat_service ||
      !ai_chat::IsAIChatEnabled(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())) ||
      !::ai_chat::IsOpenLeoButtonFromBraveSearchURL(
          navigation_handle->GetURL())) {
    return nullptr;
  }

  return std::make_unique<AIChatBraveSearchThrottle>(
      std::move(delegate), navigation_handle, ai_chat_service);
}

AIChatBraveSearchThrottle::AIChatBraveSearchThrottle(
    std::unique_ptr<Delegate> delegate,
    content::NavigationHandle* handle,
    AIChatService* ai_chat_service)
    : content::NavigationThrottle(handle),
      delegate_(std::move(delegate)),
      ai_chat_service_(ai_chat_service) {
  CHECK(delegate_);
  CHECK(ai_chat_service_);
}

AIChatBraveSearchThrottle::~AIChatBraveSearchThrottle() = default;

AIChatBraveSearchThrottle::ThrottleCheckResult
AIChatBraveSearchThrottle::WillStartRequest() {
  content::WebContents* web_contents = navigation_handle()->GetWebContents();
  if (!web_contents || !navigation_handle()->IsInPrimaryMainFrame() ||
      !IsOpenLeoButtonFromBraveSearchURL(navigation_handle()->GetURL()) ||
      !IsBraveSearchURL(web_contents->GetLastCommittedURL())) {
    return content::NavigationThrottle::PROCEED;
  }

  // Check if nonce in HTML tag matches the one in the URL.
  AIChatTabHelper* helper =
      ai_chat::AIChatTabHelper::FromWebContents(web_contents);
  if (!helper) {
    return content::NavigationThrottle::PROCEED;
  }

  helper->ValidateOpenLeoButtonNonce(
      base::BindOnce(&AIChatBraveSearchThrottle::OnNonceValidationResult,
                     weak_factory_.GetWeakPtr()));
  return content::NavigationThrottle::DEFER;
}

void AIChatBraveSearchThrottle::OpenLeoWithStagedConversions() {
  content::WebContents* web_contents = navigation_handle()->GetWebContents();
  if (!web_contents) {
    return;
  }

  ai_chat::AIChatTabHelper* helper =
      ai_chat::AIChatTabHelper::FromWebContents(web_contents);
  if (!helper) {
    return;
  }

  ConversationHandler* conversation =
      ai_chat_service_->GetOrCreateConversationHandlerForContent(
          helper->GetContentId(), helper->GetWeakPtr());
  if (!conversation) {
    return;
  }

  delegate_->OpenLeo(web_contents);
  // Trigger the fetch of staged conversations from Brave Search.
  conversation->MaybeFetchOrClearContentStagedConversation();
}

void AIChatBraveSearchThrottle::OnNonceValidationResult(bool valid) {
  if (!valid) {
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
          blink::PermissionType::BRAVE_AI_CHAT, rfh);

  if (permission_status.status == content::PermissionStatus::DENIED) {
    CancelDeferredNavigation(content::NavigationThrottle::CANCEL);
  } else if (permission_status.status == content::PermissionStatus::GRANTED) {
    OpenLeoWithStagedConversions();
    CancelDeferredNavigation(content::NavigationThrottle::CANCEL);
  } else {  // ask
    permission_controller->RequestPermissionFromCurrentDocument(
        rfh,
        content::PermissionRequestDescription(
            blink::PermissionType::BRAVE_AI_CHAT, /*user_gesture=*/true),
        base::BindOnce(&AIChatBraveSearchThrottle::OnPermissionPromptResult,
                       weak_factory_.GetWeakPtr()));
  }
}

void AIChatBraveSearchThrottle::OnPermissionPromptResult(
    content::PermissionStatus status) {
  if (status == content::PermissionStatus::GRANTED) {
    OpenLeoWithStagedConversions();
  }

  CancelDeferredNavigation(content::NavigationThrottle::CANCEL);
}

const char* AIChatBraveSearchThrottle::GetNameForLogging() {
  return "AIChatBraveSearchThrottle";
}

}  // namespace ai_chat
