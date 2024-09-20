// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_ARCHIVE_CONTENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_ARCHIVE_CONTENT_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "url/gurl.h"

namespace ai_chat {

class AssociatedArchiveContent
    : public ConversationHandler::AssociatedContentDelegate {
 public:
  AssociatedArchiveContent(GURL url,
                           std::string text_content,
                           std::u16string title,
                           bool is_video);
  ~AssociatedArchiveContent() override;
  AssociatedArchiveContent(const AssociatedArchiveContent&) = delete;
  AssociatedArchiveContent& operator=(const AssociatedArchiveContent&) = delete;

  int GetContentId() const override;
  GURL GetURL() const override;
  std::u16string GetTitle() const override;

  void GetContent(
      ConversationHandler::GetPageContentCallback callback) override;
  std::string_view GetCachedTextContent() override;
  bool GetCachedIsVideo() override;

  base::WeakPtr<AssociatedContentDelegate> GetWeakPtr();

 private:
  GURL url_;
  std::string text_content_;
  std::u16string title_;
  bool is_video_;

  base::WeakPtrFactory<ConversationHandler::AssociatedContentDelegate>
      weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_ARCHIVE_CONTENT_H_
