// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/ios/browser/ai_chat_tab_helper.h"

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/ios/browser/ai_chat_associated_content_page_fetcher.h"
#include "ios/web/public/browser_state.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/navigation/navigation_item.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace ai_chat {

AIChatTabHelper::AIChatTabHelper(web::WebState* web_state)
    : AssociatedContentDriver(
          web_state->GetBrowserState()->GetSharedURLLoaderFactory()),
      web_state_(web_state) {
  web_state->AddObserver(this);

  previous_page_title_ = web_state->GetTitle();
}

AIChatTabHelper::~AIChatTabHelper() {
  if (web_state_) {
    web_state_->RemoveObserver(this);
    web_state_ = nullptr;
  }
}

void AIChatTabHelper::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  auto* item = web_state->GetNavigationManager()->GetLastCommittedItem();
  if (!navigation_context->HasCommitted() || !item) {
    return;
  }

  // UniqueID will provide a consistent value for the entry when navigating
  // through history, allowing us to re-join conversations and navigations.
  pending_navigation_id_ = item->GetUniqueID();

  // Allow same-document navigation, as content often changes as a result
  // of fragment / pushState / replaceState navigations.
  // Content won't be retrieved immediately and we don't have a similar
  // "DOM Content Loaded" event, so let's wait for something else such as
  // page title changing before committing to starting a new conversation
  // and treating it as a "fresh page".
  is_same_document_navigation_ = navigation_context->IsSameDocument();

  // Page loaded is only considered changing when full document changes
  if (!is_same_document_navigation_) {
    is_page_loaded_ = false;
  }

  // Experimentally only call |OnNewPage| for same-page navigations _if_
  // it results in a page title change (see |TitleWasSet|). Title detection
  // also done within the navigation entry so that back/forward navigations
  // are handled correctly.
  if (!is_same_document_navigation_ ||
      previous_page_title_ != web_state->GetTitle()) {
    OnNewPage(pending_navigation_id_);
  }

  previous_page_title_ = web_state->GetTitle();
}

void AIChatTabHelper::PageLoaded(
    web::WebState* web_state,
    web::PageLoadCompletionStatus load_completion_status) {
  if (!web_state->GetLastCommittedURL().is_valid()) {
    return;
  }
  is_page_loaded_ = true;
  if (pending_get_page_content_callback_) {
    GetPageContent(std::move(pending_get_page_content_callback_), "");
  }
}

void AIChatTabHelper::TitleWasSet(web::WebState* web_state) {
  MaybeSameDocumentIsNewPage();
  previous_page_title_ = web_state->GetTitle();
  SetTitle(web_state->GetTitle());
}

void AIChatTabHelper::WebStateDestroyed(web::WebState* web_state) {
  web_state->RemoveObserver(this);
  web_state_ = nullptr;
}

void AIChatTabHelper::GetPageContent(FetchPageContentCallback callback,
                                     std::string_view invalidation_token) {
  if (!page_fetcher_) {
    std::move(callback).Run("", false, "");
    return;
  }
  auto completion_handler = base::CallbackToBlock(
      base::BindOnce(&AIChatTabHelper::OnFetchPageContentComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     std::string(invalidation_token)));
  [page_fetcher_ fetchPageContent:completion_handler];
}

void AIChatTabHelper::OnFetchPageContentComplete(
    FetchPageContentCallback callback,
    std::string invalidation_token,
    NSString* page_content,
    BOOL is_video) {
  std::string content = base::SysNSStringToUTF8(page_content);
  base::TrimWhitespaceASCII(content, base::TRIM_ALL, &content);
  // If content is empty, and page was not loaded yet, wait for page load.
  // Once page load is complete, try again.
  if (content.empty() && !is_video && !is_page_loaded_) {
    DVLOG(1) << "page was not loaded yet, will try again after load";
    SetPendingGetContentCallback(std::move(callback));
    return;
  }
  std::move(callback).Run(content, is_video, invalidation_token);
}

void AIChatTabHelper::GetSearchSummarizerKey(
    GetSearchSummarizerKeyCallback callback) {
  std::move(callback).Run(std::nullopt);
}

void AIChatTabHelper::OnNewPage(int64_t navigation_id) {
  AssociatedContentDriver::OnNewPage(navigation_id);
  set_url(web_state_->GetLastCommittedURL());
  SetTitle(web_state_->GetTitle());
  if (pending_get_page_content_callback_) {
    std::move(pending_get_page_content_callback_).Run("", false, "");
  }
}

void AIChatTabHelper::SetPendingGetContentCallback(
    FetchPageContentCallback callback) {
  if (pending_get_page_content_callback_) {
    std::move(pending_get_page_content_callback_).Run("", false, "");
  }
  pending_get_page_content_callback_ = std::move(callback);
}

void AIChatTabHelper::MaybeSameDocumentIsNewPage() {
  if (is_same_document_navigation_) {
    DVLOG(2) << "Same document navigation detected new \"page\" - calling "
                "OnNewPage()";
    // Cancel knowledge that the current navigation should be associated
    // with any conversation that's associated with the previous navigation.
    // Tell any conversation that it shouldn't be associated with this
    // content anymore, as we've moved on.
    OnNewPage(pending_navigation_id_);
    // Don't respond to further TitleWasSet
    is_same_document_navigation_ = false;
  }
}

}  // namespace ai_chat
