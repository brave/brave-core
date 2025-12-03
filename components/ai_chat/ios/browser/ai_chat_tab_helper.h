// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_TAB_HELPER_H_
#define BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_TAB_HELPER_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "ios/web/public/lazy_web_state_user_data.h"
#include "ios/web/public/web_state_observer.h"

@protocol AIChatAssociatedContentPageFetcher;

namespace ai_chat {

class AIChatTabHelper : public web::WebStateObserver,
                        public web::LazyWebStateUserData<AIChatTabHelper>,
                        public AssociatedContentDriver {
 public:
  ~AIChatTabHelper() override;

  id<AIChatAssociatedContentPageFetcher> page_fetcher() {
    return page_fetcher_;
  }
  void SetPageFetcher(id<AIChatAssociatedContentPageFetcher> page_fetcher) {
    page_fetcher_ = page_fetcher;
  }

 protected:
  // ai_chat::AssociatedContentDriver
  void GetPageContent(FetchPageContentCallback callback,
                      std::string_view invalidation_token) override;
  void GetSearchSummarizerKey(GetSearchSummarizerKeyCallback callback) override;
  void OnNewPage(int64_t navigation_id) override;

 private:
  explicit AIChatTabHelper(web::WebState* web_state);
  friend class web::LazyWebStateUserData<AIChatTabHelper>;
  __weak id<AIChatAssociatedContentPageFetcher> page_fetcher_ = nullptr;

  raw_ptr<web::WebState> web_state_ = nullptr;

  bool is_same_document_navigation_ = false;
  int pending_navigation_id_ = 0;
  std::u16string previous_page_title_;
  bool is_page_loaded_ = false;

  // TODO(petemill): Use signal to allow for multiple callbacks
  FetchPageContentCallback pending_get_page_content_callback_;

  void OnFetchPageContentComplete(FetchPageContentCallback callback,
                                  std::string invalidation_token,
                                  NSString* content,
                                  BOOL is_video);
  void MaybeSameDocumentIsNewPage();
  void SetPendingGetContentCallback(FetchPageContentCallback callback);

  base::WeakPtrFactory<AIChatTabHelper> weak_ptr_factory_{this};

  // web::WebStateObserver
  void DidFinishNavigation(web::WebState* web_state,
                           web::NavigationContext* navigation_context) override;
  void PageLoaded(
      web::WebState* web_state,
      web::PageLoadCompletionStatus load_completion_status) override;
  void TitleWasSet(web::WebState* web_state) override;
  void WebStateDestroyed(web::WebState* web_state) override;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_AI_CHAT_TAB_HELPER_H_
