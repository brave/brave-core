// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_SNAPSHOT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_SNAPSHOT_H_

#include <string>
#include <string_view>

#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
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
class AssociatedContentSnapshot : public AssociatedContentDelegate {
 public:
  AssociatedContentSnapshot(GURL url,
                            std::string text_content,
                            std::u16string title,
                            bool is_video,
                            std::string uuid);
  ~AssociatedContentSnapshot() override;
  AssociatedContentSnapshot(const AssociatedContentSnapshot&) = delete;
  AssociatedContentSnapshot& operator=(const AssociatedContentSnapshot&) =
      delete;

  // Occassionally even an archive is updated, such as when content is deleted
  // for privacy reasons.
  void SetMetadata(GURL url, std::u16string title, bool is_video);
  void SetContent(std::string text_content);

  int GetContentId() const override;
  GURL GetURL() const override;
  std::u16string GetTitle() const override;

  void GetContent(GetPageContentCallback callback) override;
  const PageContent& GetCachedPageContent() const override;

  base::WeakPtr<AssociatedContentDelegate> GetWeakPtr();

 private:
  GURL url_;
  std::u16string title_;
  PageContent cached_page_content_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_CONTENT_SNAPSHOT_H_
