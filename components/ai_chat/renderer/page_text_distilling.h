// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_TEXT_DISTILLING_H_
#define BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_TEXT_DISTILLING_H_

#include <cstdint>
#include <optional>
#include <string>

#include "base/functional/callback_forward.h"

namespace content {
class RenderFrame;
}

namespace ai_chat {

void DistillPageText(
    content::RenderFrame* render_frame,
    int32_t isolated_world_id,
    base::OnceCallback<void(const std::optional<std::string>&)>);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_TEXT_DISTILLING_H_
