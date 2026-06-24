// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTION_RULE_H_
#define BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTION_RULE_H_

#include <string_view>

#include "base/containers/span.h"

namespace ai_chat {

// A single WebMCP tool that Brave injects into pages whose URL matches
// `url_pattern`. The injected tool is registered on the page via
// `navigator.modelContext.registerTool(...)` and is then discovered by the
// existing ContentTool / AssociatedWebContentsContent pipeline, so Brave's AI
// Chat can use it like any page-registered tool.
//
// This is a proof-of-concept: the table is hardcoded (see
// GetWebMcpInjectionRules()) rather than loaded from a runtime config.
struct WebMcpInjectionRule {
  // Glob matched against the full URL spec via base::MatchPattern ('*' and '?'
  // wildcards), e.g. "*://mail.google.com/*".
  std::string_view url_pattern;

  // The tool `name` passed to registerTool().
  std::string_view tool_name;

  // The tool `description` passed to registerTool().
  std::string_view description;

  // JSON string for the tool `inputSchema` (a JSON Schema object). Must be a
  // valid JS/JSON object literal; it is embedded verbatim into the injected
  // script. Use "{}" for tools that take no input.
  std::string_view input_schema;

  // The body of the async `execute(input) { ... }` callback. Runs in the page's
  // main world and may read/manipulate the DOM. Should `return` a string (or a
  // value coercible to one) that becomes the tool result.
  std::string_view execute_body;
};

// Returns the hardcoded set of WebMCP injection rules for this PoC.
base::span<const WebMcpInjectionRule> GetWebMcpInjectionRules();

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_WEB_MCP_INJECTION_WEB_MCP_INJECTION_RULE_H_
