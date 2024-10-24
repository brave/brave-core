// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_archive_content.h"

#include <ios>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <utility>

#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/utf_ostream_operators.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"

namespace ai_chat {

AssociatedArchiveContent::AssociatedArchiveContent(
    GURL url,
    std::string text_content,
    std::vector<std::string> screenshots,
    std::u16string title,
    bool is_video)
    : url_(url),
      text_content_(text_content),
      screenshots_(screenshots),
      title_(title),
      is_video_(is_video) {
  DVLOG(1) << "Made archive for content at: " << url.spec() << "\n"
           << "title: " << title << "text: " << text_content;
}

AssociatedArchiveContent::~AssociatedArchiveContent() = default;

void AssociatedArchiveContent::SetMetadata(GURL url,
                                           std::u16string title,
                                           bool is_video) {
  url_ = url;
  title_ = title;
  is_video_ = is_video;
}

void AssociatedArchiveContent::SetContent(std::string text_content) {
  text_content_ = text_content;
}

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
  std::move(callback).Run(text_content_, is_video_, "", screenshots_);
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
