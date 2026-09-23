// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_ASSOCIATED_URL_CONTENT_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_ASSOCIATED_URL_CONTENT_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/one_shot_event.h"
#include "base/timer/timer.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "ios/web/public/web_state_observer.h"

@protocol AIChatAssociatedURLContentContext;

namespace web {
class WebState;
class NavigationContext;
}  // namespace web

namespace ai_chat {

// Represents a link that has been attached to a conversation.
// The link will be loaded asynchronously in a background WebContents when
// |GetContent| is called.
class AssociatedURLContent : public AssociatedContentDelegate,
                             public web::WebStateObserver {
 public:
  AssociatedURLContent(GURL url,
                       std::u16string title,
                       id<AIChatAssociatedURLContentContext> context);
  ~AssociatedURLContent() override;
  AssociatedURLContent(const AssociatedURLContent&) = delete;
  AssociatedURLContent& operator=(const AssociatedURLContent&) = delete;

  // AssociatedContentDelegate
  void GetContent(GetPageContentCallback callback) override;

 private:
  // web::WebStateObserver
  void WebStateDestroyed(web::WebState* web_state) override;
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void PageLoaded(
      web::WebState* web_state,
      web::PageLoadCompletionStatus load_completion_status) override;

  void FetchPageContent();
  void OnTimeout();
  void OnContentExtractionComplete(NSString* content, bool is_video);
  void CompleteWithError(const std::string& error);

  raw_ptr<web::WebState> web_state_;
  id<AIChatAssociatedURLContentContext> context_;

  std::unique_ptr<base::OneShotEvent> content_loaded_event_;
  base::OneShotTimer timeout_timer_;

  base::WeakPtrFactory<AssociatedURLContent> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_ASSOCIATED_URL_CONTENT_H_
