// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/content_tool.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/ascii.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/mojom/content_extraction/script_tools.mojom.h"
#include "url/gurl.h"

namespace ai_chat {

ContentTool::ContentTool(const blink::mojom::ScriptTool& script_tool,
                         content::WeakDocumentPtr rfh)
    : rfh_(std::move(rfh)), internal_tool_name_(script_tool.name) {
  const GURL& url = rfh_.AsRenderFrameHostIfValid()->GetLastCommittedURL();

  if (url.SchemeIs(content::kChromeUIUntrustedScheme)) {
    // First-party WebUI (e.g. the Leo workspace page at
    // chrome-untrusted://workspace) that registers tools via WebMCP. These are
    // system-owned, so surface the tool's own name and description verbatim: no
    // host/path prefix (the page is unique and controls its names) and no
    // "website-provided" framing.
    name_ = script_tool.name;
    description_ = script_tool.description;
  } else {
    // Name of the ContentTool is {host}{path}_tool_name. The path is included
    // (not just the host) so that tools with the same name registered on
    // different pages of the same host don't collapse to the same tool name.
    name_ = base::StrCat({url.host(), url.path(), "_", script_tool.name});

    // We add some additional information to the description of the content tool
    // to make it obvious the tool is coming from a website.
    description_ = base::StrCat(
        {"Website-provided tool for the current page at ", url.spec(), ".",
         "Name: ", script_tool.name, ".",
         "Website-provided description: ", script_tool.description, ".",
         "Only use this tool when it is relevant to the user's request on this "
         "page."});
  }

  // Toolnames only allow alphanumeric characters and underscores.
  std::replace_if(
      name_.begin(), name_.end(),
      [](char c) { return !absl::ascii_isalnum(c) && c != '_'; }, '_');

  if (!script_tool.input_schema) {
    return;
  }
  auto schema = base::JSONReader::ReadDict(
      *script_tool.input_schema, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!schema) {
    return;
  }
  if (auto* props = schema->FindDict("properties")) {
    input_properties_ = props->Clone();
  }
  if (auto* required = schema->FindList("required")) {
    for (const auto& item : *required) {
      if (item.is_string()) {
        required_properties_.push_back(item.GetString());
      }
    }
  }
}

ContentTool::~ContentTool() = default;

std::string_view ContentTool::Name() const {
  return name_;
}

std::string_view ContentTool::Description() const {
  return description_;
}

std::optional<base::DictValue> ContentTool::InputProperties() const {
  if (!input_properties_) {
    return std::nullopt;
  }
  return input_properties_->Clone();
}

std::optional<std::vector<std::string>> ContentTool::RequiredProperties()
    const {
  if (required_properties_.empty()) {
    return std::nullopt;
  }
  return required_properties_;
}

std::variant<bool, mojom::PermissionChallengePtr>
ContentTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  if (user_permission_granted_) {
    return false;
  }

  return mojom::PermissionChallenge::New();
}

void ContentTool::UserPermissionGranted(const std::string& tool_use_id) {
  user_permission_granted_ = true;
}

void ContentTool::UseTool(const std::string& input_json,
                          UseToolCallback callback) {
  content::RenderFrameHost* rfh = rfh_.AsRenderFrameHostIfValid();
  if (!rfh) {
    std::move(callback).Run({}, {});
    return;
  }
  mojo::Remote<mojom::PageContentExtractor> extractor;
  rfh->GetRemoteInterfaces()->GetInterface(
      extractor.BindNewPipeAndPassReceiver());
  auto* extractor_ptr = extractor.get();
  // Models may emit an empty string for a tool that takes no parameters. The
  // renderer parses this as JSON and fails ("Failed to parse input arguments"),
  // so normalize it to an empty object.
  const std::string& tool_input = input_json.empty() ? "{}" : input_json;
  extractor_ptr->ExecuteContentTool(
      internal_tool_name_, tool_input,
      base::BindOnce(
          [](UseToolCallback cb, mojo::Remote<mojom::PageContentExtractor>,
             const std::optional<std::string>& result) {
            std::move(cb).Run(CreateContentBlocksForText(result.value_or("")),
                              {});
          },
          std::move(callback), std::move(extractor)));
}

}  // namespace ai_chat
