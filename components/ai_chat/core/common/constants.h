/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_CONSTANTS_H_

#include <string_view>

#include "base/containers/fixed_flat_set.h"
#include "url/url_constants.h"
namespace ai_chat {

inline constexpr char kBraveSearchURLPrefix[] = "search";

inline constexpr char kBraveUntrustedContentOpenTag[] =
    "<brave_untrusted_content>";
inline constexpr char kBraveUntrustedContentCloseTag[] =
    "</brave_untrusted_content>";
inline constexpr char kBraveUntrustedContentTagName[] =
    "brave_untrusted_content";

inline constexpr auto kAllowedContentSchemes =
    base::MakeFixedFlatSet<std::string_view>(
        {url::kHttpsScheme, url::kHttpScheme, url::kFileScheme,
         url::kDataScheme});

// Model key for the automatic model.
inline constexpr char kChatAutomaticModelKey[] = "chat-automatic";
// Model key for Claude Sonnet model.
inline constexpr char kClaudeSonnetModelKey[] = "chat-claude-sonnet";

// Keys for custom model prefs
inline constexpr char kCustomModelItemModelKey[] = "model_request_name";
inline constexpr char kCustomModelItemEndpointUrlKey[] = "endpoint_url";

// Size of the favicon images which mojom::AIChatUIHandler::GetFaviconDataURL
// provides. They are displayed at icon size, and are embedded in data which we
// want to keep small, so this only needs to be large enough for a high density
// display.
inline constexpr int kFaviconDataURLSizeInPixels = 64;

// The chrome-untrusted WebUI that executes untrusted AI-generated code in a
// sandboxed iframe which runs with an opaque origin. CSP cannot block WebRTC
// (ICE uses UDP), so RTCPeerConnection is blocked for the page's entire frame
// tree by checking the top-level frame's origin (see
// chromium_src/third_party/blink/renderer/modules/peerconnection/
// rtc_peer_connection.cc).
inline constexpr char kAIChatCodeSandboxUIHost[] = "aichat-code-sandbox";
inline constexpr char kAIChatCodeSandboxUIURL[] =
    "chrome-untrusted://aichat-code-sandbox/";

// The chrome-untrusted WebUI that hosts Leo's local "workspace" file tools.
// Each workspace is served from its own subdomain of this host
// (chrome-untrusted://<uuid>.leo-workspace) rather than from a path under it,
// so that every workspace is its own origin and therefore gets its own storage
// and its own File System Access grants. LeoWorkspaceUIConfig opts into this by
// overriding WebUIConfig::ShouldHandleSubdomains().
inline constexpr char kAIChatLeoWorkspaceUIHost[] = "leo-workspace";

// The suffix every workspace host ends with, for IsAIChatLeoWorkspaceHost().
inline constexpr char kAIChatLeoWorkspaceUIHostSuffix[] = ".leo-workspace";
static_assert(
    std::string_view(kAIChatLeoWorkspaceUIHostSuffix).substr(1) ==
        std::string_view(kAIChatLeoWorkspaceUIHost),
    "The workspace host suffix must be the workspace host, preceded by a dot.");

// Prefixed to a workspace's host to get the host of that workspace's viewer
// document (chrome-untrusted://view.<uuid>.leo-workspace), which is a separate
// origin from the workspace that frames it.
inline constexpr char kAIChatLeoWorkspaceViewUIHostPrefix[] = "view.";

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_CONSTANTS_H_
