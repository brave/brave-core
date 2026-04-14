// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/request_url_tool.h"

#include <string>
#include <utility>
#include <variant>

#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

RequestURLTool::RequestURLTool(AttachURLCallback attach_url_callback)
    : attach_url_callback_(std::move(attach_url_callback)) {
  CHECK(attach_url_callback_);
}

RequestURLTool::~RequestURLTool() = default;

std::string_view RequestURLTool::Name() const {
  return mojom::kRequestURLToolName;
}

std::string_view RequestURLTool::Description() const {
  return "Attach a web page to the conversation so its content can be read. "
         "Use when the user references a specific URL or when reading a "
         "particular page would help answer the question. The page content "
         "will be available in subsequent turns. Only http and https URLs are "
         "supported.";
}

std::optional<base::DictValue> RequestURLTool::InputProperties() const {
  return CreateInputProperties(
      {{"url",
        StringProperty("The http or https URL of the web page to load and "
                       "attach to the conversation")},
       {"title",
        StringProperty("A short title describing what you expect to find at "
                       "this URL, shown to the user while loading")}});
}

std::optional<std::vector<std::string>> RequestURLTool::RequiredProperties()
    const {
  return std::vector<std::string>{"url"};
}

std::variant<bool, mojom::PermissionChallengePtr>
RequestURLTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  // Already approved for this specific invocation.
  if (!approved_tool_use_id_.empty() && tool_use.id == approved_tool_use_id_) {
    return false;
  }

  auto input_dict = base::JSONReader::ReadDict(
      tool_use.arguments_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input_dict) {
    // Invalid JSON — let UseTool handle and report the error.
    return false;
  }

  const std::string* url_str = input_dict->FindString("url");
  if (!url_str || url_str->empty()) {
    // Missing URL — let UseTool handle and report the error.
    return false;
  }

  GURL url(*url_str);
  if (!url.is_valid() || (!url.SchemeIs("https") && !url.SchemeIs("http"))) {
    // Bad URL — let UseTool handle and report the error.
    return false;
  }

  auto challenge = mojom::PermissionChallenge::New();
  challenge->plan = base::StrCat({"Allow Leo to fetch ", url.spec()});
  return challenge;
}

void RequestURLTool::UserPermissionGranted(const std::string& tool_use_id) {
  approved_tool_use_id_ = tool_use_id;
}

void RequestURLTool::UseTool(const std::string& input_json,
                             UseToolCallback callback) {
  if (approved_tool_use_id_.empty()) {
    // UseTool should only be called after UserPermissionGranted.
    std::move(callback).Run(CreateContentBlocksForText("Unknown error"), {});
    return;
  }
  // Consume the approval so the next URL requires fresh confirmation.
  approved_tool_use_id_.clear();

  auto input_dict = base::JSONReader::ReadDict(
      input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input_dict) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Invalid JSON input"), {});
    return;
  }

  const std::string* url_str = input_dict->FindString("url");
  if (!url_str || url_str->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: Missing or empty 'url' field"), {});
    return;
  }

  GURL url(*url_str);
  if (!url.is_valid() || (!url.SchemeIs("https") && !url.SchemeIs("http"))) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Error: Invalid URL or unsupported scheme. Only http and https "
            "are allowed."),
        {});
    return;
  }

  const std::string* title_str = input_dict->FindString("title");
  std::string title =
      (title_str && !title_str->empty()) ? *title_str : std::string(url.host());

  // Defer the tool result until the page content is ready and include the
  // fetched text directly in the tool response so the assistant can read the
  // article in the same turn.
  auto on_complete = base::BindOnce(
      [](UseToolCallback cb, std::string content) {
        std::move(cb).Run(CreateContentBlocksForText(std::move(content)), {});
      },
      std::move(callback));

  attach_url_callback_.Run(url, std::move(title), std::move(on_complete));
}

}  // namespace ai_chat
