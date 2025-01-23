// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_AI_CHAT_ASSOCIATED_CONTENT_DRIVER_IOS_H_
#define BRAVE_IOS_BROWSER_API_AI_CHAT_ASSOCIATED_CONTENT_DRIVER_IOS_H_

#include <string>
#include <string_view>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"

@protocol AIChatDelegate;

namespace ai_chat {

class AIChatService;

class AssociatedContentDriverIOS : public AssociatedContentDriver {
 public:
  AssociatedContentDriverIOS(
      AIChatService* ai_chat_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      id<AIChatDelegate> delegate);
  ~AssociatedContentDriverIOS() override;

 protected:
  std::u16string GetPageTitle() const override;
  GURL GetPageURL() const override;
  void GetPageContent(ConversationHandler::GetPageContentCallback callback,
                      std::string_view invalidation_token) override;
  void GetSearchSummarizerKey(GetSearchSummarizerKeyCallback callback) override;

 private:
  __weak id<AIChatDelegate> bridge_;
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_API_AI_CHAT_ASSOCIATED_CONTENT_DRIVER_IOS_H_
