// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/associated_archive_content.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/utf_ostream_operators.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"

namespace ai_chat {

AssociatedArchiveContent::AssociatedArchiveContent(GURL url,
                                                   std::string text_content,
                                                   std::u16string title,
                                                   bool is_video,
                                                   std::string uuid)
    : url_(url), title_(title), cached_page_content_(text_content, is_video) {
  DVLOG(1) << "Made archive for content at: " << url.spec() << "\n"
           << "title: " << title << "text: " << text_content;
  set_uuid(std::move(uuid));
}

AssociatedArchiveContent::~AssociatedArchiveContent() = default;

void AssociatedArchiveContent::SetMetadata(GURL url,
                                           std::u16string title,
                                           bool is_video) {
  url_ = url;
  title_ = title;
  cached_page_content_.is_video = is_video;
}

void AssociatedArchiveContent::SetContent(std::string text_content) {
  cached_page_content_.content = text_content;
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

void AssociatedArchiveContent::GetContent(GetPageContentCallback callback) {
  std::move(callback).Run(cached_page_content_);
}

const PageContent& AssociatedArchiveContent::GetCachedPageContent() const {
  return cached_page_content_;
}

}  // namespace ai_chat
