// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_RENDERER_YT_UTIL_H_
#define BRAVE_COMPONENTS_AI_CHAT_RENDERER_YT_UTIL_H_

#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_tree.h"
#include "base/values.h"

namespace ai_chat {

inline constexpr auto kYouTubeHosts =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "m.youtube.com",
                                                 "www.youtube.com",
                                             });

// Extract a caption url from an array of YT caption tracks, from the YT page
// API.
std::optional<std::string> ChooseCaptionTrackUrl(
    const base::Value::List* caption_tracks);

// Parse YT metadata json string and choose the most appropriate caption track
// url.
std::optional<std::string> ParseAndChooseCaptionTrackUrl(std::string_view body);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_RENDERER_YT_UTIL_H_
