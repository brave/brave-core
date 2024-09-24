// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_AI_CHAT_ASSOCIATED_MULTI_TAB_CONTENT_H_
#define BRAVE_BROWSER_UI_AI_CHAT_ASSOCIATED_MULTI_TAB_CONTENT_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/browser.h"
#include "content/public/browser/web_contents.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"

namespace ai_chat {

class AssociatedMultiTabContent : public AssociatedContentDriver {
 public:
  explicit AssociatedMultiTabContent(Browser* browser);

  AssociatedMultiTabContent(const AssociatedMultiTabContent&) = delete;
  AssociatedMultiTabContent& operator=(const AssociatedMultiTabContent&) = delete;
  ~AssociatedMultiTabContent() override;

  // AssociatedContentDriver
  mojom::AssociatedContentType GetAssociatedContentType() const override;
  mojom::SiteInfoDetailPtr GetAssociatedContentDetail() const override;
  GURL GetPageURL() const override;
  std::u16string GetPageTitle() const override;
  void GetSearchSummarizerKey(
      GetSearchSummarizerKeyCallback callback) override;
  void GetPageContent(ConversationHandler::GetPageContentCallback callback,
      std::string_view invalidation_token) override;

 private:
  raw_ptr<Browser> browser_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_AI_CHAT_ASSOCIATED_MULTI_TAB_CONTENT_H_
