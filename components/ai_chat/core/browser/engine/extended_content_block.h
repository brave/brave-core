// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_EXTENDED_CONTENT_BLOCK_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_EXTENDED_CONTENT_BLOCK_H_

#include <string>
#include <variant>

#include "base/values.h"

namespace ai_chat {

enum class ExtendedContentBlockType {
  // Standard OpenAI content block types
  kText,
  kImage,
  // Customized Brave content block types
  kPageExcerpt,
  kChangeTone,
  kParaphrase,
  kImprove,
  kShorten,
  kExpand,
  kMaxValue = kExpand,
};

// https://platform.openai.com/docs/api-reference/chat/create#chat_create-messages-user_message-content-array_of_content_parts-text_content_part
struct TextContent {
  std::string text;
};

struct ImageUrl {
  ImageUrl();
  ImageUrl(const ImageUrl&) = delete;
  ImageUrl& operator=(const ImageUrl&) = delete;
  ImageUrl(ImageUrl&&);
  ImageUrl& operator=(ImageUrl&&);
  ~ImageUrl();

  // Either a URL of the image or the base64 encoded image data.
  std::string url;
  // Specifies the detail level of the image.
  std::optional<std::string> detail;
};

// https://platform.openai.com/docs/api-reference/chat/create#chat_create-messages-user_message-content-array_of_content_parts-image_content_part
struct ImageContent {
  ImageUrl image_url;
};

struct ChangeToneContent {
  std::string tone;
};

using ContentData = std::variant<TextContent, ImageContent, ChangeToneContent>;

struct ExtendedContentBlock {
  ExtendedContentBlock();
  ExtendedContentBlock(ExtendedContentBlockType type, ContentData data);
  ExtendedContentBlock(const ExtendedContentBlock&) = delete;
  ExtendedContentBlock& operator=(const ExtendedContentBlock&) = delete;
  ExtendedContentBlock(ExtendedContentBlock&&);
  ExtendedContentBlock& operator=(ExtendedContentBlock&&);
  ~ExtendedContentBlock();

  ExtendedContentBlockType type;
  ContentData data;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_EXTENDED_CONTENT_BLOCK_H_
