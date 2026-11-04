// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/strings/string_util.h"
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

std::string EscapeMarkdown(std::string_view text) {
  std::string escaped;
  escaped.reserve(text.size());
  for (char c : text) {
    // Ignore whitespace characters in our escaping.
    if (base::IsAsciiWhitespace(c)) {
      continue;
    }

    // Only certain characters are allowed so as to avoid markdown injection.
    if (!base::IsAsciiAlphaNumeric(c) && c != '/' && c != '_' && c != '-' &&
        c != '.' && c != ':') {
      continue;
    }

    if (base::IsAsciiPunctuation(c)) {
      escaped.push_back('\\');
    }
    escaped.push_back(c);
  }
  return escaped;
}

std::string EscapeMarkdownText(std::string_view text) {
  std::string escaped;
  escaped.reserve(text.size());
  bool pending_space = false;
  for (char c : text) {
    if (base::IsAsciiWhitespace(c)) {
      pending_space = !escaped.empty();
      continue;
    }
    if (base::IsAsciiControl(c)) {
      continue;
    }
    if (pending_space) {
      escaped.push_back(' ');
      pending_space = false;
    }
    // Markdown lets any ASCII punctuation be escaped, which leaves it literal.
    if (base::IsAsciiPunctuation(c)) {
      escaped.push_back('\\');
    }
    escaped.push_back(c);
  }
  return escaped;
}

}  // namespace ai_chat
