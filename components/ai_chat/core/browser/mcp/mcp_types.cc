/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_types.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include "base/base64.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_util.h"
#include "base/types/optional_util.h"

namespace ai_chat {

namespace {

// Tool definition keys.
constexpr char kNameKey[] = "name";
constexpr char kTitleKey[] = "title";
constexpr char kDescriptionKey[] = "description";
constexpr char kInputSchemaKey[] = "inputSchema";

// CallToolResult keys.
constexpr char kContentKey[] = "content";
constexpr char kStructuredContentKey[] = "structuredContent";
constexpr char kIsErrorKey[] = "isError";
constexpr char kTypeKey[] = "type";
constexpr char kTextKey[] = "text";
constexpr char kDataKey[] = "data";
constexpr char kMimeTypeKey[] = "mimeType";

constexpr char kTextContentType[] = "text";
constexpr char kImageContentType[] = "image";

// The MCP spec's limit on tool names.
constexpr size_t kMaxToolNameLength = 128;

// Image types the model can be given.
constexpr auto kSupportedImageMimeTypes =
    base::MakeFixedFlatSet<std::string_view>(
        {"image/gif", "image/jpeg", "image/png", "image/webp"});

bool IsAbsentOrString(const base::DictValue& dict, std::string_view key) {
  const base::Value* value = dict.Find(key);
  return !value || value->is_string();
}

bool IsValidToolName(std::string_view name) {
  return !name.empty() && name.size() <= kMaxToolNameLength &&
         std::ranges::none_of(name,
                              [](char c) { return base::IsAsciiControl(c); });
}

std::optional<MCPContent> ParseContent(const base::Value& item) {
  if (!item.is_dict()) {
    return std::nullopt;
  }
  const base::DictValue& dict = item.GetDict();
  const std::string* type = dict.FindString(kTypeKey);
  if (!type) {
    return std::nullopt;
  }
  if (*type == kTextContentType) {
    const std::string* text = dict.FindString(kTextKey);
    if (!text) {
      return std::nullopt;
    }
    return MCPTextContent{*text};
  }
  if (*type == kImageContentType) {
    const std::string* data = dict.FindString(kDataKey);
    const std::string* mime_type = dict.FindString(kMimeTypeKey);
    // Only strict base64 is accepted, so `data` can be used without
    // re-encoding.
    if (!data || !mime_type || !kSupportedImageMimeTypes.contains(*mime_type) ||
        !base::Base64Decode(*data)) {
      return std::nullopt;
    }
    return MCPImageContent{*data, *mime_type};
  }
  return std::nullopt;
}

}  // namespace

MCPServerConfig::MCPServerConfig() = default;
MCPServerConfig::MCPServerConfig(GURL endpoint,
                                 std::optional<std::string> bearer_token,
                                 std::string display_name)
    : endpoint(std::move(endpoint)),
      bearer_token(std::move(bearer_token)),
      display_name(std::move(display_name)) {}
MCPServerConfig::MCPServerConfig(const MCPServerConfig&) = default;
MCPServerConfig& MCPServerConfig::operator=(const MCPServerConfig&) = default;
MCPServerConfig::MCPServerConfig(MCPServerConfig&&) noexcept = default;
MCPServerConfig& MCPServerConfig::operator=(MCPServerConfig&&) noexcept =
    default;
MCPServerConfig::~MCPServerConfig() = default;

bool MCPServerConfig::operator==(const MCPServerConfig&) const = default;

MCPToolDefinition::MCPToolDefinition() = default;
MCPToolDefinition::MCPToolDefinition(MCPToolDefinition&&) noexcept = default;
MCPToolDefinition& MCPToolDefinition::operator=(MCPToolDefinition&&) noexcept =
    default;
MCPToolDefinition::~MCPToolDefinition() = default;

MCPToolDefinition MCPToolDefinition::Clone() const {
  MCPToolDefinition clone;
  clone.name = name;
  clone.title = title;
  clone.description = description;
  clone.input_schema = input_schema.Clone();
  return clone;
}

MCPCallToolResult::MCPCallToolResult() = default;
MCPCallToolResult::MCPCallToolResult(MCPCallToolResult&&) noexcept = default;
MCPCallToolResult& MCPCallToolResult::operator=(MCPCallToolResult&&) noexcept =
    default;
MCPCallToolResult::~MCPCallToolResult() = default;

std::optional<MCPToolDefinition> ParseToolDefinition(
    const base::DictValue& tool) {
  const std::string* name = tool.FindString(kNameKey);
  const base::DictValue* input_schema = tool.FindDict(kInputSchemaKey);
  if (!name || !IsValidToolName(*name) || !input_schema) {
    return std::nullopt;
  }
  if (!IsAbsentOrString(tool, kTitleKey) ||
      !IsAbsentOrString(tool, kDescriptionKey)) {
    return std::nullopt;
  }
  MCPToolDefinition definition;
  definition.name = *name;
  definition.title = base::OptionalFromPtr(tool.FindString(kTitleKey));
  definition.description =
      base::OptionalFromPtr(tool.FindString(kDescriptionKey));
  definition.input_schema = input_schema->Clone();
  return definition;
}

std::optional<MCPCallToolResult> ParseCallToolResult(base::DictValue result) {
  MCPCallToolResult call_result;
  if (const base::Value* content = result.Find(kContentKey)) {
    if (!content->is_list()) {
      return std::nullopt;
    }
    for (const base::Value& item : content->GetList()) {
      if (std::optional<MCPContent> parsed = ParseContent(item)) {
        call_result.content.push_back(std::move(*parsed));
      }
    }
  }
  if (base::Value* structured = result.Find(kStructuredContentKey)) {
    if (!structured->is_dict()) {
      return std::nullopt;
    }
    call_result.structured_content = std::move(structured->GetDict());
  }
  if (const base::Value* is_error = result.Find(kIsErrorKey)) {
    if (!is_error->is_bool()) {
      return std::nullopt;
    }
    call_result.is_error = is_error->GetBool();
  }
  return call_result;
}

}  // namespace ai_chat
