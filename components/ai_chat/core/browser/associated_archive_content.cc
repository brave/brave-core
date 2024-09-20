// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_archive_content.h"

#include <utility>

#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"

namespace ai_chat {

AssociatedArchiveContent::AssociatedArchiveContent(GURL url,
                                                   std::string text_content,
                                                   std::u16string title,
                                                   bool is_video)
    : url_(url),
      text_content_(text_content),
      title_(title),
      is_video_(is_video) {
  DVLOG(1) << "Made archive for content at: " << url.spec() << "\n"
           << "title: " << title << "text: " << text_content;
}

AssociatedArchiveContent::~AssociatedArchiveContent() = default;

int AssociatedArchiveContent::GetContentId() const {
  return -1;
}

GURL AssociatedArchiveContent::GetURL() const {
  return url_;
}

std::u16string AssociatedArchiveContent::GetTitle() const {
  return title_;
}

void AssociatedArchiveContent::GetContent(
    ConversationHandler::GetPageContentCallback callback) {
  std::move(callback).Run(text_content_, is_video_, "");
}

std::string_view AssociatedArchiveContent::GetCachedTextContent() {
  return text_content_;
}

bool AssociatedArchiveContent::GetCachedIsVideo() {
  return is_video_;
}

base::WeakPtr<ConversationHandler::AssociatedContentDelegate>
AssociatedArchiveContent::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace ai_chat
