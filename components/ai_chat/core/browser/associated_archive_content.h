// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_ARCHIVE_CONTENT_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_ARCHIVE_CONTENT_H_

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
class AssociatedArchiveContent : public AssociatedContentDelegate {
 public:
  AssociatedArchiveContent(GURL url,
                           std::string text_content,
                           std::u16string title,
                           bool is_video,
                           std::string uuid);
  ~AssociatedArchiveContent() override;
  AssociatedArchiveContent(const AssociatedArchiveContent&) = delete;
  AssociatedArchiveContent& operator=(const AssociatedArchiveContent&) = delete;

  void GetContent(GetPageContentCallback callback) override;

  base::WeakPtr<AssociatedContentDelegate> GetWeakPtr();
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ASSOCIATED_ARCHIVE_CONTENT_H_
