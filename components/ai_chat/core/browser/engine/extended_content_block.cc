// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/extended_content_block.h"

namespace ai_chat {

ImageUrl::ImageUrl() = default;

ImageUrl::ImageUrl(ImageUrl&&) = default;

ImageUrl& ImageUrl::operator=(ImageUrl&&) = default;

ImageUrl::~ImageUrl() = default;

ExtendedContentBlock::ExtendedContentBlock() = default;

ExtendedContentBlock::ExtendedContentBlock(ExtendedContentBlockType type,
                                           ContentData data)
    : type(type), data(std::move(data)) {}

ExtendedContentBlock::ExtendedContentBlock(ExtendedContentBlock&&) = default;

ExtendedContentBlock& ExtendedContentBlock::operator=(ExtendedContentBlock&&) =
    default;

ExtendedContentBlock::~ExtendedContentBlock() = default;

}  // namespace ai_chat
