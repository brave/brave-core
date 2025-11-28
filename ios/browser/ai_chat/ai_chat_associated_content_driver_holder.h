// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_ASSOCIATED_CONTENT_DRIVER_HOLDER_H_
#define BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_ASSOCIATED_CONTENT_DRIVER_HOLDER_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "ios/web/public/lazy_web_state_user_data.h"

@protocol AIChatAssociatedContentDriverBridge;

namespace ai_chat {

class AssociatedContentDriverBridgeHolder
    : public web::LazyWebStateUserData<AssociatedContentDriverBridgeHolder>,
      public AssociatedContentDriver {
 public:
  ~AssociatedContentDriverBridgeHolder() override;

  id<AIChatAssociatedContentDriverBridge> bridge() { return bridge_; }
  void SetBridge(id<AIChatAssociatedContentDriverBridge> bridge) {
    bridge_ = bridge;
    SetTitle(GetPageTitle());
    set_url(GetPageURL());
  }

 protected:
  std::u16string GetPageTitle() const;
  GURL GetPageURL() const;
  void GetPageContent(FetchPageContentCallback callback,
                      std::string_view invalidation_token) override;
  void GetSearchSummarizerKey(GetSearchSummarizerKeyCallback callback) override;

 private:
  explicit AssociatedContentDriverBridgeHolder(web::WebState* web_state);
  friend class web::LazyWebStateUserData<AssociatedContentDriverBridgeHolder>;
  __weak id<AIChatAssociatedContentDriverBridge> bridge_ = nullptr;

  void OnFinishedGetPageContent(FetchPageContentCallback callback,
                                std::string invalidation_token,
                                NSString* content,
                                BOOL is_video);

  base::WeakPtrFactory<AssociatedContentDriverBridgeHolder> weak_ptr_factory_{
      this};
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_AI_CHAT_AI_CHAT_ASSOCIATED_CONTENT_DRIVER_HOLDER_H_
