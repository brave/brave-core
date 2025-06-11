// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEST_MOCK_ASSOCIATED_CONTENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEST_MOCK_ASSOCIATED_CONTENT_H_

#include <string>
#include <string_view>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

namespace ai_chat {

class MockAssociatedContent : public AssociatedContentDelegate {
 public:
  MockAssociatedContent();
  ~MockAssociatedContent() override;

  int GetContentId() const override { return content_id_; }

  void SetContentId(int id) { content_id_ = id; }

  void GetContent(GetPageContentCallback callback) override;

  const PageContent& GetCachedPageContent() const override {
    return cached_page_content_;
  }

  MOCK_METHOD(GURL, GetURL, (), (const, override));
  MOCK_METHOD(std::u16string, GetTitle, (), (const, override));

  MOCK_METHOD(bool, GetIsVideo, (), (const));
  MOCK_METHOD(std::string, GetTextContent, (), (const));

  MOCK_METHOD(void,
              GetStagedEntriesFromContent,
              (GetStagedEntriesCallback),
              (override));
  MOCK_METHOD(bool, HasOpenAIChatPermission, (), (const, override));

  base::WeakPtr<AssociatedContentDelegate> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  int content_id_ = 0;
  PageContent cached_page_content_;
  base::WeakPtrFactory<AssociatedContentDelegate> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TEST_MOCK_ASSOCIATED_CONTENT_H_
