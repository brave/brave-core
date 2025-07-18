// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEST_MOCK_ASSOCIATED_CONTENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEST_MOCK_ASSOCIATED_CONTENT_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

namespace ai_chat {

class MockAssociatedContent : public AssociatedContentDelegate {
 public:
  MockAssociatedContent();
  ~MockAssociatedContent() override;

  void SetContentId(int id) { content_id_ = id; }

  void SetUrl(GURL url) { url_ = std::move(url); }

  void SetCachedPageContent(PageContent page_content) {
    cached_page_content_ = std::move(page_content);
  }

  void SetTitle(std::u16string title);

  // AssociatedContentDelegate:
  void GetContent(GetPageContentCallback callback) override;
  void OnNewPage(int64_t navigation_id) override;

  MOCK_METHOD(void,
              GetStagedEntriesFromContent,
              (GetStagedEntriesCallback),
              (override));
  MOCK_METHOD(bool, HasOpenAIChatPermission, (), (const, override));
  MOCK_METHOD(void,
              GetScreenshots,
              (mojom::ConversationHandler::GetScreenshotsCallback),
              (override));

  base::WeakPtr<AssociatedContentDelegate> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<AssociatedContentDelegate> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEST_MOCK_ASSOCIATED_CONTENT_H_
