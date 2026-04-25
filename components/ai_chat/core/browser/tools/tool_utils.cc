// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

std::vector<mojom::ContentBlockPtr> CreateContentBlocksForText(
    const std::string& text) {
  std::vector<mojom::ContentBlockPtr> content_blocks;
  content_blocks.push_back(mojom::ContentBlock::NewTextContentBlock(
      mojom::TextContentBlock::New(text)));
  return content_blocks;
}

std::vector<mojom::ContentBlockPtr> CreateContentBlocksForImage(
    const GURL& image_url) {
  std::vector<mojom::ContentBlockPtr> content_blocks;
  content_blocks.push_back(mojom::ContentBlock::NewImageContentBlock(
      mojom::ImageContentBlock::New(image_url)));
  return content_blocks;
}

}  // namespace ai_chat
