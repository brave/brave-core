// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/mcp_tool.h"

#include <algorithm>
#include <tuple>
#include <utility>
#include <variant>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/hash/hash.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/bind_post_task.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_client.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/mcp.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "third_party/abseil-cpp/absl/functional/overload.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

constexpr char kToolNamePrefix[] = "mcp_";
// Hex digits of the hash that keeps a shortened or altered name distinct.
constexpr size_t kNameHashLength = 8;

constexpr char kPropertiesKey[] = "properties";
constexpr char kRequiredKey[] = "required";

// Model APIs reject property names that aren't `[a-zA-Z0-9_.-]{1,64}`.
constexpr size_t kMaxPropertyNameLength = 64;

// Never followed, and removed so the model's API doesn't try to either: the
// definitions they'd point at aren't sent.
constexpr auto kReferenceKeywords = base::MakeFixedFlatSet<std::string_view>(
    {"$ref", "$dynamicRef", "$recursiveRef"});

constexpr char kStoppedMessage[] = "The user stopped this tool call.";
constexpr char kNoContentMessage[] = "The tool returned no content.";
constexpr char kToolErrorMessage[] = "The tool reported an error:";
constexpr char kUnsupportedImageMessage[] =
    "The tool returned an image in a format that can't be shown.";
constexpr char kImageTooLargeMessage[] =
    "The tool returned an image that's too large to be shown.";
constexpr char kInvalidArgumentsMessage[] =
    "The tool's arguments must be a JSON object.";
constexpr char kTruncatedMessage[] =
    "The rest of the tool's output was left out because it's too long.";

// `mcp_<slug>_<tool name>`, with each character a model's API doesn't allow
// replaced by `_`. If that alters the name, or it's too long, it's shortened to
// end in a hash of the tool's name, so that it stays distinct from the names of
// the server's other tools. The slug has no `_`, so it keeps the names of
// different servers' tools apart.
std::string MakeToolName(std::string_view slug, std::string_view tool_name) {
  std::string name = base::StrCat({kToolNamePrefix, slug, "_", tool_name});
  bool altered = false;
  for (char& c : name) {
    if (!base::IsAsciiAlphaNumeric(c) && c != '_') {
      c = '_';
      altered = true;
    }
  }
  if (!altered && name.size() <= kMaxMCPToolNameLength) {
    return name;
  }
  name.resize(
      std::min(name.size(), kMaxMCPToolNameLength - kNameHashLength - 1));
  base::StrAppend(
      &name, {"_", absl::StrFormat("%08x", base::PersistentHash(tool_name))});
  return name;
}

bool IsValidPropertyName(std::string_view name) {
  return !name.empty() && name.size() <= kMaxPropertyNameLength &&
         std::ranges::all_of(name, [](char c) {
           return base::IsAsciiAlphaNumeric(c) || c == '_' || c == '.' ||
                  c == '-';
         });
}

struct SchemaCounts {
  size_t objects = 0;
  size_t properties = 0;
};

// Copies `value`, part of an input schema, without reference keywords.
// `is_properties` is whether it's a map of property names to schemas, rather
// than a schema. Returns nullopt if the schema exceeds the bounds, or has a
// property name that a model's API would reject.
std::optional<base::Value> CopySchema(const base::Value& value,
                                      size_t depth,
                                      bool is_properties,
                                      SchemaCounts& counts) {
  if ((value.is_dict() || value.is_list()) && depth > kMaxMCPToolSchemaDepth) {
    return std::nullopt;
  }
  if (const base::DictValue* dict = value.GetIfDict()) {
    if (++counts.objects > kMaxMCPToolSchemaObjects) {
      return std::nullopt;
    }
    if (is_properties) {
      counts.properties += dict->size();
      if (counts.properties > kMaxMCPToolSchemaProperties) {
        return std::nullopt;
      }
    }
    base::DictValue copy;
    for (const auto [key, child] : *dict) {
      if (is_properties && !IsValidPropertyName(key)) {
        return std::nullopt;
      }
      if (!is_properties && kReferenceKeywords.contains(key)) {
        continue;
      }
      std::optional<base::Value> child_copy = CopySchema(
          child, depth + 1,
          /*is_properties=*/!is_properties && key == kPropertiesKey, counts);
      if (!child_copy) {
        return std::nullopt;
      }
      copy.Set(key, std::move(*child_copy));
    }
    return base::Value(std::move(copy));
  }
  if (const base::ListValue* list = value.GetIfList()) {
    base::ListValue copy;
    for (const base::Value& item : *list) {
      std::optional<base::Value> item_copy =
          CopySchema(item, depth + 1, /*is_properties=*/false, counts);
      if (!item_copy) {
        return std::nullopt;
      }
      copy.Append(std::move(*item_copy));
    }
    return base::Value(std::move(copy));
  }
  return value.Clone();
}

// Truncated at a character boundary.
std::string Truncate(std::string_view text, size_t max_bytes) {
  std::string truncated;
  base::TruncateUTF8ToByteSize(text, max_bytes, &truncated);
  return truncated;
}

void AppendText(Tool::ToolResult& blocks, std::string text) {
  blocks.push_back(mojom::ContentBlock::NewTextContentBlock(
      mojom::TextContentBlock::New(std::move(text))));
}

// Appends as much of the server's `text` as `budget` has room for, and sets
// `truncated` if that isn't all of it.
void AppendServerText(Tool::ToolResult& blocks,
                      std::string text,
                      size_t& budget,
                      bool& truncated) {
  if (text.size() > budget) {
    text = Truncate(text, budget);
    truncated = true;
  }
  budget -= text.size();
  if (!text.empty()) {
    AppendText(blocks, std::move(text));
  }
}

void AppendImage(Tool::ToolResult& blocks, const MCPImageContent& image) {
  std::string url =
      base::StrCat({"data:", image.mime_type, ";base64,", image.data});
  // Longer URLs can't be passed to the UI.
  if (url.size() > url::kMaxURLChars) {
    AppendText(blocks, kImageTooLargeMessage);
    return;
  }
  blocks.push_back(mojom::ContentBlock::NewImageContentBlock(
      mojom::ImageContentBlock::New(GURL(url))));
}

// What the model is told the tool returned.
Tool::ToolResult ToToolResult(
    base::expected<MCPCallToolResult, MCPError> result) {
  if (!result.has_value()) {
    return CreateContentBlocksForText(result.error().message);
  }
  Tool::ToolResult blocks;
  if (result->is_error) {
    AppendText(blocks, kToolErrorMessage);
  }
  // `kMCPMaxResponseBytes` allows far more text than a model should be sent.
  size_t text_budget = kMaxMCPToolOutputTextBytes;
  bool truncated = false;
  bool has_text = false;
  for (MCPContent& content : result->content) {
    std::visit(
        absl::Overload{
            [&](MCPTextContent& text) {
              AppendServerText(blocks, std::move(text.text), text_budget,
                               truncated);
              has_text = true;
            },
            [&](const MCPImageContent& image) { AppendImage(blocks, image); },
            [&](const MCPUnsupportedImageContent&) {
              AppendText(blocks, kUnsupportedImageMessage);
            },
        },
        content);
  }
  // Servers are asked to also return structured content as text, so it's only
  // used when they don't.
  if (result->structured_content && !has_text) {
    if (std::optional<std::string> json =
            base::WriteJson(*result->structured_content)) {
      AppendServerText(blocks, std::move(*json), text_budget, truncated);
    }
  }
  if (truncated) {
    AppendText(blocks, kTruncatedMessage);
  }
  if (blocks.empty() || (result->is_error && blocks.size() == 1)) {
    AppendText(blocks, kNoContentMessage);
  }
  return blocks;
}

}  // namespace

MCPTool::Call::Call() = default;
MCPTool::Call::Call(Call&&) noexcept = default;
MCPTool::Call& MCPTool::Call::operator=(Call&&) noexcept = default;
MCPTool::Call::~Call() = default;

// static
std::unique_ptr<MCPTool> MCPTool::Create(const mojom::MCPServer& server,
                                         const MCPToolDefinition& definition,
                                         base::WeakPtr<MCPClient> client) {
  base::DictValue input_properties;
  std::vector<std::string> required_properties;
  if (const base::Value* properties =
          definition.input_schema.Find(kPropertiesKey)) {
    SchemaCounts counts;
    std::optional<base::Value> copy =
        properties->is_dict() ? CopySchema(*properties, /*depth=*/1,
                                           /*is_properties=*/true, counts)
                              : std::nullopt;
    std::optional<std::string> json =
        copy ? base::WriteJson(*copy) : std::nullopt;
    if (!json || json->size() > kMaxMCPToolSchemaBytes) {
      return nullptr;
    }
    input_properties = std::move(copy->GetDict());
  }
  if (const base::ListValue* required =
          definition.input_schema.FindList(kRequiredKey)) {
    base::flat_set<std::string_view> added;
    for (const base::Value& item : *required) {
      const std::string* property = item.GetIfString();
      if (property && input_properties.contains(*property) &&
          added.insert(*property).second) {
        required_properties.push_back(*property);
      }
    }
  }
  return base::WrapUnique(
      new MCPTool(server, definition, std::move(input_properties),
                  std::move(required_properties), std::move(client)));
}

// static
std::vector<std::unique_ptr<MCPTool>> MCPTool::CreateAll(
    const mojom::MCPServer& server,
    base::span<const MCPToolDefinition> definitions,
    base::WeakPtr<MCPClient> client) {
  std::vector<std::unique_ptr<MCPTool>> tools;
  base::flat_set<std::string> names;
  for (const MCPToolDefinition& definition : definitions) {
    std::unique_ptr<MCPTool> tool = Create(server, definition, client);
    if (tool && names.insert(std::string(tool->Name())).second) {
      tools.push_back(std::move(tool));
    }
  }
  return tools;
}

MCPTool::MCPTool(const mojom::MCPServer& server,
                 const MCPToolDefinition& definition,
                 base::DictValue input_properties,
                 std::vector<std::string> required_properties,
                 base::WeakPtr<MCPClient> client)
    : tool_name_(definition.name),
      tool_description_(Truncate(definition.description.value_or(""),
                                 kMaxMCPToolDescriptionBytes)),
      name_(MakeToolName(server.slug, definition.name)),
      required_properties_(std::move(required_properties)),
      server_name_(server.name),
      server_origin_(url::Origin::Create(server.endpoint)),
      client_(std::move(client)) {
  if (!input_properties.empty()) {
    input_properties_ = std::move(input_properties);
  }
  // Makes it clear to the model where the tool comes from. Everything but the
  // origin is free text, which mustn't be able to pass itself off as part of
  // the prompt.
  description_ = base::StrCat({"Tool provided by the MCP server ",
                               EscapeMarkdownText(server_name_), " at ",
                               EscapeMarkdown(server_origin_.Serialize()),
                               ". Name: ", EscapedToolName(), "."});
  if (definition.title) {
    base::StrAppend(
        &description_,
        {" Title: ",
         EscapeMarkdownText(Truncate(*definition.title, kMaxMCPToolTitleBytes)),
         "."});
  }
  if (!tool_description_.empty()) {
    base::StrAppend(&description_,
                    {" Server-provided description: ",
                     EscapeMarkdownText(tool_description_), "."});
  }
  base::StrAppend(&description_,
                  {" Only use this tool when it is relevant to the user's "
                   "request."});
}

MCPTool::~MCPTool() {
  // A dropped connection doesn't stop the server's work.
  for (auto& [id, call] : calls_) {
    std::move(call.cancel).Run();
  }
}

std::string_view MCPTool::Name() const {
  return name_;
}

std::string_view MCPTool::Description() const {
  return description_;
}

std::string_view MCPTool::DisplayName() const {
  return tool_name_;
}

std::string_view MCPTool::DisplayDescription() const {
  return tool_description_;
}

std::optional<base::DictValue> MCPTool::InputProperties() const {
  if (!input_properties_) {
    return std::nullopt;
  }
  return input_properties_->Clone();
}

std::optional<std::vector<std::string>> MCPTool::RequiredProperties() const {
  if (required_properties_.empty()) {
    return std::nullopt;
  }
  return required_properties_;
}

std::variant<bool, mojom::PermissionChallengePtr>
MCPTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  if (user_permission_strategy_ == mojom::ToolPermission::kAllowSession ||
      granted_tool_use_id_ == tool_use.id) {
    return false;
  }
  auto challenge = mojom::PermissionChallenge::New();
  challenge->title = l10n_util::GetStringUTF8(
      IDS_CHAT_UI_PERMISSION_CHALLENGE_WEB_TOOL_HEADER);
  challenge->description = GetPermissionChallengeDescription(tool_use);
  challenge->supports_allow_session = true;
  return challenge;
}

void MCPTool::UserPermissionGranted(
    const std::string& tool_use_id,
    const mojom::PermissionChallenge& challenge) {
  granted_tool_use_id_ = tool_use_id;
}

void MCPTool::SetUserPermissionStrategy(mojom::ToolPermission permission) {
  user_permission_strategy_ = permission;
}

std::optional<std::string> MCPTool::GetPermissionChallengeDescription(
    const mojom::ToolUseEvent& tool_use) const {
  return l10n_util::GetStringFUTF8(
      IDS_CHAT_UI_PERMISSION_CHALLENGE_MCP_TOOL_SUMMARY,
      base::UTF8ToUTF16(EscapedToolName()),
      base::UTF8ToUTF16(EscapeMarkdownText(server_name_)),
      base::UTF8ToUTF16(EscapeMarkdown(server_origin_.Serialize())));
}

void MCPTool::UseTool(const std::string& input_json, UseToolCallback callback) {
  // Models may send an empty string for a tool that takes no arguments.
  std::optional<base::DictValue> arguments =
      input_json.empty()
          ? std::make_optional<base::DictValue>()
          : base::JSONReader::ReadDict(input_json, base::JSON_PARSE_RFC);
  if (!arguments || !client_) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  CreateContentBlocksForText(
                                      arguments ? ServerUnavailableMessage()
                                                : kInvalidArgumentsMessage),
                                  ToolArtifacts()));
    return;
  }
  const int call_id = ++last_call_id_;
  // A client that's destroyed drops its callbacks, and this with them.
  base::ScopedClosureRunner on_dropped(
      base::BindPostTaskToCurrentDefault(base::BindOnce(
          &MCPTool::OnCallDropped, weak_ptr_factory_.GetWeakPtr(), call_id)));
  Call& call = calls_[call_id];
  call.callback = std::move(callback);
  call.cancel = client_->CallTool(
      tool_name_, std::move(*arguments),
      base::BindOnce(&MCPTool::OnCallToolResult, weak_ptr_factory_.GetWeakPtr(),
                     call_id, std::move(on_dropped)));
}

void MCPTool::StopToolUse() {
  base::flat_map<int, Call> calls = std::exchange(calls_, {});
  for (auto& [id, call] : calls) {
    std::move(call.cancel).Run();
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(call.callback),
                                  CreateContentBlocksForText(kStoppedMessage),
                                  ToolArtifacts()));
  }
}

void MCPTool::OnCallToolResult(
    int call_id,
    base::ScopedClosureRunner on_dropped,
    base::expected<MCPCallToolResult, MCPError> result) {
  std::ignore = on_dropped.Release();
  CompleteCall(call_id, ToToolResult(std::move(result)));
}

void MCPTool::OnCallDropped(int call_id) {
  CompleteCall(call_id, CreateContentBlocksForText(ServerUnavailableMessage()));
}

void MCPTool::CompleteCall(int call_id, ToolResult result) {
  auto it = calls_.find(call_id);
  if (it == calls_.end()) {
    return;
  }
  UseToolCallback callback = std::move(it->second.callback);
  calls_.erase(it);
  std::move(callback).Run(std::move(result), ToolArtifacts());
}

std::string MCPTool::EscapedToolName() const {
  // The tool's name is the server's to choose, so it's reduced to characters
  // that can't spell out a message of its own. If that leaves nothing, the
  // name the model uses is the next best thing to show.
  std::string escaped = EscapeMarkdown(tool_name_);
  return escaped.empty() ? EscapeMarkdown(name_) : escaped;
}

std::string MCPTool::ServerUnavailableMessage() const {
  return base::StrCat({server_name_, " is no longer available."});
}

}  // namespace ai_chat
