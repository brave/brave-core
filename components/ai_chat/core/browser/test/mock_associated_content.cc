// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/test/mock_associated_content.h"

namespace ai_chat {

MockAssociatedContent::MockAssociatedContent() = default;

MockAssociatedContent::~MockAssociatedContent() {
  // Let observers know this content is gone.
  OnNewPage(-1);
}

void MockAssociatedContent::GetContent(GetPageContentCallback callback) {
  cached_text_content_ = GetTextContent();
  std::move(callback).Run(GetTextContent(), GetCachedIsVideo(), "");
}

}  // namespace ai_chat
