// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_ARCHIVE_CONTENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_ARCHIVE_CONTENT_H_

#include <string>
#include <string_view>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "url/gurl.h"

namespace ai_chat {

// Used in place of real content, which is normally provided by
// AssociatedContentDriver (via AIChatTabHelper and WebContents on desktop).
// When the actual associated content is no longer available (e.g. it's been
// navigated away from), this class is used to provide the archive of that
// content.
// Similarly, if a conversation is loaded from storage, and the conversation
// was associated with content, this class is used to represent that content.
//
// If this class is used to represent archive content that can be shared by
// multiple conversations, consider changing owner to the AIChatService and
// having it subclass AssociatedContentDriver for related conversation
// management.
class AssociatedArchiveContent
    : public ConversationHandler::AssociatedContentDelegate {
 public:
  AssociatedArchiveContent(GURL url,
                           std::string text_content,
                           std::vector<std::string> screenshots,
                           std::u16string title,
                           bool is_video);
  ~AssociatedArchiveContent() override;
  AssociatedArchiveContent(const AssociatedArchiveContent&) = delete;
  AssociatedArchiveContent& operator=(const AssociatedArchiveContent&) = delete;

  // Occassionally even an archive is updated, such as when content is deleted
  // for privacy reasons.
  void SetMetadata(GURL url, std::u16string title, bool is_video);
  void SetContent(std::string text_content);

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
  std::vector<std::string> screenshots_;
  std::u16string title_;
  bool is_video_;

  base::WeakPtrFactory<ConversationHandler::AssociatedContentDelegate>
      weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_ARCHIVE_CONTENT_H_
