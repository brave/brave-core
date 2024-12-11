// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_TEXT_DISTILLING_H_
#define BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_TEXT_DISTILLING_H_

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/callback_forward.h"
#include "url/gurl.h"

namespace content {
class RenderFrame;
}

namespace ai_chat {

// Distills the text content of a page. If possible, it will use a custom site
// distiller script. Otherwise, it will fall back to a more general approach.
void DistillPageText(
    content::RenderFrame* render_frame,
    int32_t global_world_id,
    int32_t isolated_world_id,
    base::OnceCallback<void(const std::optional<std::string>&)>);

// Attempts to retrieve a a custom site distiller script for the given host.
// Returns a pair consisting of the script content, and a boolean indicating if
// it is intended for the main world or not
std::optional<std::pair<std::string, bool>> LoadSiteScriptForHost(
    std::string_view host);

// Attempts to distill a page based on the retrieval of a host-specific script.
void DistillPageTextViaSiteScript(
    content::RenderFrame* render_frame,
    std::string_view script_content,
    int32_t world_id,
    base::OnceCallback<void(const std::optional<std::string>&)>);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_TEXT_DISTILLING_H_
