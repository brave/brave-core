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
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/mojom/content_extraction/script_tools.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace ai_chat {

namespace {

// Backslash-escapes ASCII punctuation so `text` can be embedded in a
// markdown string without affecting its formatting. Used for
// site-controlled values (e.g. WebMCP tool names) shown in the permission
// prompt.
std::string EscapeMarkdown(const std::string& text) {
  std::string escaped;
  escaped.reserve(text.size());
  for (char c : text) {
    if (absl::ascii_ispunct(c)) {
      escaped.push_back('\\');
    }
    escaped.push_back(c);
  }
  return escaped;
}

}  // namespace

ContentTool::ContentTool(const blink::mojom::ScriptTool& script_tool,
                         content::WeakDocumentPtr rfh)
    : rfh_(std::move(rfh)), internal_tool_name_(script_tool.name) {
  const GURL& url = rfh_.AsRenderFrameHostIfValid()->GetLastCommittedURL();

  // Name of the ContentTool is web_{host}_tool_name. Only the host is used
  // (not the full path) to keep the name within Bedrock's 64-char tool-name
  // limit.
  name_ = base::StrCat({"web_", url.host(), "_", script_tool.name});

  // Toolnames only allow alphanumeric characters and underscores.
  std::replace_if(
      name_.begin(), name_.end(),
      [](char c) { return !absl::ascii_isalnum(c) && c != '_'; }, '_');

  // We add some additional information to the description of the content tool
  // to make it obvious the tool is coming from a website.
  description_ = base::StrCat(
      {"Website-provided tool for the current page at ", url.spec(), ".",
       "Name: ", script_tool.name, ".",
       "Website-provided description: ", script_tool.description, ".",
       "Only use this tool when it is relevant to the user's request on this "
       "page."});

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

  auto challenge = mojom::PermissionChallenge::New();
  challenge->description = GetPermissionChallengeDescription(tool_use);
  return challenge;
}

void ContentTool::UserPermissionGranted(const std::string& tool_use_id) {
  user_permission_granted_ = true;
}

std::optional<std::string> ContentTool::GetPermissionChallengeDescription(
    const mojom::ToolUseEvent& tool_use) const {
  // Provide a human-readable, markdown-formatted description naming the
  // site-registered tool and the site's origin, instead of the mangled
  // model-facing tool name. The tool name is site-controlled, so escape it
  // to prevent the site injecting markdown into the prompt.
  content::RenderFrameHost* rfh = rfh_.AsRenderFrameHostIfValid();
  if (!rfh) {
    return std::nullopt;
  }
  const url::Origin& origin = rfh->GetLastCommittedOrigin();
  // An opaque origin (e.g. a data: URL, or a document sandboxed via CSP) has
  // no serialization to fall back on, so the full URL is used instead - and
  // unlike an origin's serialization (scheme://host:port), a URL's
  // path/query is site-controlled and can legally contain markdown
  // metacharacters, so it must be escaped just like the tool name to
  // prevent the site injecting formatting or a link into the prompt.
  const std::string site_display =
      origin.opaque() ? EscapeMarkdown(rfh->GetLastCommittedURL().spec())
                      : origin.Serialize();
  return l10n_util::GetStringFUTF8(
      IDS_CHAT_UI_PERMISSION_CHALLENGE_WEB_TOOL_SUMMARY,
      base::UTF8ToUTF16(EscapeMarkdown(internal_tool_name_)),
      base::UTF8ToUTF16(site_display));
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
