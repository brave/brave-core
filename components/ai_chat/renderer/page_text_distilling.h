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

#include "base/containers/fixed_flat_map.h"
#include "base/functional/callback_forward.h"
#include "brave/components/ai_chat/resources/custom_site_distiller_scripts/grit/custom_site_distiller_scripts_generated.h"

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

// A map of hostnames to the corresponding custom site distiller script.
// The value is a pair consisting of the resource ID of the script, and a
// boolean indicating if the script is intended for the main world or not.
inline constexpr auto kHostToScriptResource =
    base::MakeFixedFlatMap<std::string_view, std::pair<int, bool>>({
        {"github.com",
         {IDR_CUSTOM_SITE_DISTILLER_SCRIPTS_GITHUB_COM_BUNDLE_JS, false}},
        {"x.com", {IDR_CUSTOM_SITE_DISTILLER_SCRIPTS_X_COM_BUNDLE_JS, true}},
    });

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
