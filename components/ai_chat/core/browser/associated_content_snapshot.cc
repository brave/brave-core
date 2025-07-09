// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_content_snapshot.h"

#include <string_view>
#include <utility>

#include "base/logging.h"
#include "base/strings/utf_ostream_operators.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"

namespace ai_chat {

AssociatedContentSnapshot::AssociatedContentSnapshot(GURL url,
                                                     std::string text_content,
                                                     std::u16string title,
                                                     bool is_video,
                                                     std::string uuid)
    : url_(url), title_(title), cached_page_content_(text_content, is_video) {
  DVLOG(1) << "Made archive for content at: " << url.spec() << "\n"
           << "title: " << title << "text: " << text_content;
  set_uuid(std::move(uuid));
}

AssociatedContentSnapshot::~AssociatedContentSnapshot() = default;

void AssociatedContentSnapshot::SetMetadata(GURL url,
                                            std::u16string title,
                                            bool is_video) {
  url_ = url;
  title_ = title;
  cached_page_content_.is_video = is_video;
}

void AssociatedContentSnapshot::SetContent(std::string text_content) {
  cached_page_content_.content = text_content;
}

int AssociatedContentSnapshot::GetContentId() const {
  return -1;
}

GURL AssociatedContentSnapshot::GetURL() const {
  return url_;
}

std::u16string AssociatedContentSnapshot::GetTitle() const {
  return title_;
}

void AssociatedContentSnapshot::GetContent(GetPageContentCallback callback) {
  std::move(callback).Run(cached_page_content_);
}

const PageContent& AssociatedContentSnapshot::GetCachedPageContent() const {
  return cached_page_content_;
}

}  // namespace ai_chat
