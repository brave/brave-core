// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/test/mock_associated_content.h"

namespace ai_chat {

MockAssociatedContent::MockAssociatedContent() = default;
MockAssociatedContent::~MockAssociatedContent() = default;

void MockAssociatedContent::SetTitle(std::u16string title) {
  AssociatedContentDelegate::SetTitle(std::move(title));
}

void MockAssociatedContent::OnNewPage(int64_t navigation_id) {
  AssociatedContentDelegate::OnNewPage(navigation_id);
}

void MockAssociatedContent::GetContent(GetPageContentCallback callback) {
  std::move(callback).Run(cached_page_content_);
}

}  // namespace ai_chat
