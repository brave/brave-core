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
#include "url/gurl.h"

namespace content {
class RenderFrame;
}

namespace ai_chat {

void DistillPageText(
    content::RenderFrame* render_frame,
    int32_t global_world_id,
    int32_t isolated_world_id,
    base::OnceCallback<void(const std::optional<std::string>&)>);

bool LoadSiteScriptForHost(std::string* host,
                           std::string* script_content,
                           bool* needs_main_world);

void DistillPageTextViaSiteScript(
    content::RenderFrame* render_frame,
    const std::string& script_content,
    int32_t world_id,
    base::OnceCallback<void(const std::optional<std::string>&)> callback);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_TEXT_DISTILLING_H_
