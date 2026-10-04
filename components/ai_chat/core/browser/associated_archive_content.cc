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
                                                   std::string uuid) {
  DVLOG(1) << "Made archive for content at: " << url.spec() << "\n"
           << "title: " << title << "text: " << text_content;
  set_uuid(std::move(uuid));
  set_url(std::move(url));
  set_cached_page_content(PageContent(std::move(text_content), is_video));
  SetTitle(std::move(title));
}

AssociatedArchiveContent::~AssociatedArchiveContent() = default;

void AssociatedArchiveContent::GetContent(GetPageContentCallback callback) {
  std::move(callback).Run(cached_page_content());
}

}  // namespace ai_chat
