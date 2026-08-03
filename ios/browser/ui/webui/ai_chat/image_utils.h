// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_IMAGE_UTILS_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_IMAGE_UTILS_H_

#include <cstdint>
#include <optional>
#include <vector>

namespace ai_chat {

// Decodes the image in `image_data`, scales it down to the bounds shared with
// the other platforms via screenshot::ScaleDownBitmap(), and re-encodes it as a
// PNG. Returns std::nullopt if `image_data` holds no decodable image or cannot
// be re-encoded. Blocks, so call this from a sequence that allows blocking.
std::optional<std::vector<uint8_t>> DecodeAndScaleImageData(
    std::vector<uint8_t> image_data);

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_IMAGE_UTILS_H_
