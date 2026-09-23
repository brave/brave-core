// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/web_mcp/core/browser/web_mcp_injection_rule.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "base/strings/pattern.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "url/gurl.h"

namespace web_mcp {

namespace {

// Userscript-style fences that delimit the metadata block at the top of a
// script file.
constexpr std::string_view kBlockStart = "==WebMCP==";
constexpr std::string_view kBlockEnd = "==/WebMCP==";

// Tools that take no input still need a JSON Schema object with an (empty)
// `properties` key. Without `properties`, ContentTool leaves its input schema
// unset and the tool is sent to the server with no `parameters`, which makes
// the model emit invalid tool-call arguments.
constexpr std::string_view kDefaultInputSchema =
    R"({"type":"object","properties":{}})";

// Strips a leading `//` line comment marker (and surrounding whitespace) from
// `line`, returning the remaining content.
std::string_view StripCommentMarker(std::string_view line) {
  line = base::TrimWhitespaceASCII(line, base::TRIM_ALL);
  if (line.starts_with("//")) {
    line = base::TrimWhitespaceASCII(line.substr(2), base::TRIM_ALL);
  }
  return line;
}

// Applies a single `@key value` metadata entry to `rule`. Unknown keys and
// empty values are ignored.
void ApplyMetadataEntry(WebMcpInjectionRule& rule,
                        std::string_view key,
                        std::string_view value) {
  if (value.empty()) {
    return;
  }
  if (key == "name") {
    rule.tool_name = std::string(value);
  } else if (key == "match") {
    rule.url_patterns.emplace_back(value);
  } else if (key == "description") {
    // Multiple `@description` lines are joined with a single space.
    rule.description = rule.description.empty()
                           ? std::string(value)
                           : base::StrCat({rule.description, " ", value});
  } else if (key == "schema") {
    rule.input_schema = std::string(value);
  }
}

// Parses an in-block metadata line (already stripped of its `//` marker) of the
// form `@key value`, applying it to `rule`. Non-`@` lines are ignored.
void ParseMetadataLine(WebMcpInjectionRule& rule, std::string_view entry) {
  if (!entry.starts_with("@")) {
    return;
  }
  entry.remove_prefix(1);
  const size_t sep = entry.find_first_of(" \t");
  const std::string_view key =
      sep == std::string_view::npos ? entry : entry.substr(0, sep);
  const std::string_view value =
      sep == std::string_view::npos
          ? std::string_view()
          : base::TrimWhitespaceASCII(entry.substr(sep + 1), base::TRIM_ALL);
  ApplyMetadataEntry(rule, key, value);
}

}  // namespace

WebMcpInjectionRule::WebMcpInjectionRule() = default;
WebMcpInjectionRule::WebMcpInjectionRule(const WebMcpInjectionRule&) = default;
WebMcpInjectionRule& WebMcpInjectionRule::operator=(
    const WebMcpInjectionRule&) = default;
WebMcpInjectionRule::WebMcpInjectionRule(WebMcpInjectionRule&&) noexcept =
    default;
WebMcpInjectionRule& WebMcpInjectionRule::operator=(
    WebMcpInjectionRule&&) noexcept = default;
WebMcpInjectionRule::~WebMcpInjectionRule() = default;

// static
std::optional<WebMcpInjectionRule> WebMcpInjectionRule::ParseScript(
    std::string_view contents) {
  const std::vector<std::string_view> lines = base::SplitStringPiece(
      contents, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  // Locate the `==WebMCP==` / `==/WebMCP==` fences.
  std::optional<size_t> block_start;
  std::optional<size_t> block_end;
  for (size_t i = 0; i < lines.size(); ++i) {
    const std::string_view content = StripCommentMarker(lines[i]);
    if (!block_start && content == kBlockStart) {
      block_start = i;
    } else if (block_start && content == kBlockEnd) {
      block_end = i;
      break;
    }
  }
  if (!block_start || !block_end) {
    return std::nullopt;
  }

  WebMcpInjectionRule rule;
  for (size_t i = *block_start + 1; i < *block_end; ++i) {
    ParseMetadataLine(rule, StripCommentMarker(lines[i]));
  }

  // The body is everything after the closing fence, with leading and trailing
  // blank lines trimmed but interior formatting preserved.
  const std::vector<std::string_view> body_lines(lines.begin() + *block_end + 1,
                                                 lines.end());
  rule.execute_body = base::TrimWhitespaceASCII(
      base::JoinString(body_lines, "\n"), base::TRIM_ALL);

  if (rule.input_schema.empty()) {
    rule.input_schema = std::string(kDefaultInputSchema);
  }

  // A usable rule needs a name, at least one URL pattern, and a body.
  if (rule.tool_name.empty() || rule.url_patterns.empty() ||
      rule.execute_body.empty()) {
    return std::nullopt;
  }

  return rule;
}

bool WebMcpInjectionRule::Matches(const GURL& url) const {
  return std::ranges::any_of(url_patterns, [&url](const std::string& pattern) {
    return base::MatchPattern(url.spec(), pattern);
  });
}

}  // namespace web_mcp
