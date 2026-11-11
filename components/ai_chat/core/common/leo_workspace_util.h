/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_LEO_WORKSPACE_UTIL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_LEO_WORKSPACE_UTIL_H_

#include <string_view>

#include "brave/components/ai_chat/core/common/constants.h"

namespace ai_chat {

// Whether `host` is a workspace's own host: exactly one label before the
// suffix, so not a viewer host. Shared with blink, whose WebMCP gate has only
// an origin to go on.
constexpr bool IsAIChatLeoWorkspaceHost(std::string_view host) {
  if (!host.ends_with(kAIChatLeoWorkspaceUIHostSuffix)) {
    return false;
  }
  const std::string_view label = host.substr(
      0,
      host.size() - std::string_view(kAIChatLeoWorkspaceUIHostSuffix).size());
  return !label.empty() && label.find('.') == std::string_view::npos;
}

constexpr bool IsAIChatLeoWorkspaceViewHost(std::string_view host) {
  const std::string_view prefix(kAIChatLeoWorkspaceViewUIHostPrefix);
  return host.starts_with(prefix) &&
         IsAIChatLeoWorkspaceHost(host.substr(prefix.size()));
}

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_COMMON_LEO_WORKSPACE_UTIL_H_
