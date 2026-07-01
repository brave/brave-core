// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_INJECTION_RULE_H_
#define BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_INJECTION_RULE_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

class GURL;

namespace web_mcp {

// A single WebMCP tool that Brave injects into pages whose URL matches one of
// `url_patterns`. The injected tool is registered on the page via
// `navigator.modelContext.registerTool(...)` and is then discovered by the
// existing ContentTool pipeline, so Brave's AI Chat can use it like any
// page-registered tool.
//
// Each rule is parsed from one `.js` file shipped in the WebMCP component (see
// WebMcpRuleRegistry). A script file holds the tool metadata in a leading
// comment block delimited by `// ==WebMCP==` / `// ==/WebMCP==` (the
// Greasemonkey/userscript convention), followed by the body of the tool's
// `async execute(input)` callback. For example:
//
//   // ==WebMCP==
//   // @name        unread_count
//   // @match       https://mail.google.com/*
//   // @description Report the number of unread messages.
//   // @schema      {"type":"object","properties":{}}
//   // ==/WebMCP==
//
//   const inboxLink = document.querySelector('a[href*="#inbox"]');
//   return inboxLink ? inboxLink.getAttribute('aria-label') : 'not found';
struct WebMcpInjectionRule {
  WebMcpInjectionRule();
  WebMcpInjectionRule(const WebMcpInjectionRule&);
  WebMcpInjectionRule& operator=(const WebMcpInjectionRule&);
  WebMcpInjectionRule(WebMcpInjectionRule&&) noexcept;
  WebMcpInjectionRule& operator=(WebMcpInjectionRule&&) noexcept;
  ~WebMcpInjectionRule();

  // Parses one script file's `contents` into a rule. Returns nullopt when the
  // `==WebMCP==` metadata block is absent or a required field (`@name`,
  // `@match`, or a non-empty body) is missing.
  static std::optional<WebMcpInjectionRule> ParseScript(
      std::string_view contents);

  // True if `url`'s full spec matches any of `url_patterns` via
  // base::MatchPattern ('*' and '?' wildcards).
  bool Matches(const GURL& url) const;

  // The tool `name` passed to registerTool() (`@name`).
  std::string tool_name;

  // The tool `description` passed to registerTool() (`@description`, with
  // multiple lines joined by a single space).
  std::string description;

  // JSON string for the tool `inputSchema` (a JSON Schema object) (`@schema`).
  // Embedded verbatim into the injected script; defaults to an empty-input
  // object schema when the script omits `@schema`.
  std::string input_schema;

  // The body of the async `execute(input) { ... }` callback. Runs in the
  // page's main world and may read/manipulate the DOM. Should `return` a string
  // (or a value coercible to one) that becomes the tool result.
  std::string execute_body;

  // Globs (`@match`) matched against the full URL spec. A rule applies when any
  // pattern matches.
  std::vector<std::string> url_patterns;
};

}  // namespace web_mcp

#endif  // BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_INJECTION_RULE_H_
