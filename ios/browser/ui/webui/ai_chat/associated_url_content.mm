// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/associated_url_content.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "base/uuid.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/ios/browser/ai_chat_associated_content_page_fetcher.h"
#include "brave/ios/browser/ai_chat/ai_chat_ui_handler_bridge.h"
#include "brave/ios/browser/api/web_view/brave_web_view.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web_view/internal/cwv_web_view_internal.h"

namespace ai_chat {

AssociatedURLContent::AssociatedURLContent(
    GURL url,
    std::u16string title,
    id<AIChatAssociatedURLContentContext> context)
    : web_state_(context.webView.webState), context_(context) {
  set_uuid(base::Uuid::GenerateRandomV4().AsLowercaseString());
  set_url(std::move(url));
  SetTitle(std::move(title));

  web_state_->AddObserver(this);
}

AssociatedURLContent::~AssociatedURLContent() {
  if (web_state_) {
    web_state_->RemoveObserver(this);
    web_state_ = nullptr;
  }
}

void AssociatedURLContent::GetContent(GetPageContentCallback callback) {
  // As we're just loading a link there's no point fetching it again if we have
  // content.
  // Note: If we change this in future we'll need to consider reloading the
  // page, as currently the DOMContentLoaded event won't fire again for
  // navigating to the same url.
  if (!cached_page_content().content.empty()) {
    std::move(callback).Run(cached_page_content());
    return;
  }

  if (!content_loaded_event_) {
    DVLOG(2) << __func__ << "Loading link content for: " << url().spec();

    content_loaded_event_ = std::make_unique<base::OneShotEvent>();

    // Note: We setup a 30 second timeout to avoid trying to load the page
    // forever.
    timeout_timer_.Start(FROM_HERE, base::Seconds(30),
                         base::BindOnce(&AssociatedURLContent::OnTimeout,
                                        weak_ptr_factory_.GetWeakPtr()));
    web::NavigationManager::WebLoadParams load_params(url());
    load_params.transition_type = ui::PAGE_TRANSITION_LINK;
    web_state_->GetNavigationManager()->LoadURLWithParams(load_params);
  }

  // Register callback with the OneShotEvent - it will be called when content is
  // loaded
  content_loaded_event_->Post(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<AssociatedURLContent> self,
                        GetPageContentCallback callback) {
                       if (!self) {
                         return;
                       }
                       std::move(callback).Run(self->cached_page_content());
                     },
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AssociatedURLContent::DidFinishNavigation(
    web::WebState* web_state,
    web::NavigationContext* navigation_context) {
  if (!navigation_context->HasCommitted()) {
    return;
  }
  if (NSError* error = navigation_context->GetError()) {
    CompleteWithError("Navigation failed: " +
                      base::SysNSStringToUTF8(error.localizedDescription) +
                      " (" + base::NumberToString(error.code) + ")");
  }
}

void AssociatedURLContent::PageLoaded(
    web::WebState* web_state,
    web::PageLoadCompletionStatus load_completion_status) {
  DVLOG(2) << __func__ << "Page fully loaded for URL: " << url().spec();
  SetTitle(web_state_->GetTitle());

  FetchPageContent();
}

void AssociatedURLContent::WebStateDestroyed(web::WebState* web_state) {
  web_state_->RemoveObserver(this);
  web_state_ = nullptr;
}

void AssociatedURLContent::OnTimeout() {
  DVLOG(2) << __func__
           << "Background content loading timed out for URL: " << url().spec();

  // On timeout, try and fetch the page content anyway in case we managed to
  // partially load the page.
  SetTitle(web_state_->GetTitle());
  FetchPageContent();
}

void AssociatedURLContent::FetchPageContent() {
  auto completion_handler = base::CallbackToBlock(
      base::BindOnce(&AssociatedURLContent::OnContentExtractionComplete,
                     weak_ptr_factory_.GetWeakPtr()));
  [context_.pageFetcher fetchPageContent:completion_handler];
}

void AssociatedURLContent::OnContentExtractionComplete(NSString* ns_content,
                                                       bool is_video) {
  std::string content = base::SysNSStringToUTF8(ns_content);
  DVLOG(2) << __func__
           << "Content extraction completed for URL: " << url().spec()
           << ", content length: " << content.length()
           << ", is video: " << is_video;

  timeout_timer_.Stop();

  // Update our cached content with the loaded content
  set_cached_page_content(PageContent(std::move(content), is_video));

  // Notify pending callbacks
  if (content_loaded_event_) {
    content_loaded_event_->Signal();
    content_loaded_event_ = nullptr;
  }
}

void AssociatedURLContent::CompleteWithError(const std::string& error) {
  // Note: We don't actually do anything with the error, just log it in debug
  // mode.
  DVLOG(2) << __func__ << "Background content loading failed: " << error;

  timeout_timer_.Stop();

  // Clear cached content
  set_cached_page_content(PageContent());

  // Notify pending callbacks
  if (content_loaded_event_) {
    content_loaded_event_->Signal();
    content_loaded_event_ = nullptr;
  }
}

}  // namespace ai_chat
