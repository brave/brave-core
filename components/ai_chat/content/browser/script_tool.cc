// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/script_tool.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/mojom/content_extraction/script_tools.mojom.h"

namespace ai_chat {

ScriptTool::ScriptTool(const blink::mojom::ScriptTool& script_tool,
                       content::WeakDocumentPtr rfh)
    : rfh_(std::move(rfh)),
      name_(script_tool.name),
      description_(script_tool.description) {
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

ScriptTool::~ScriptTool() = default;

std::string_view ScriptTool::Name() const {
  return name_;
}

std::string_view ScriptTool::Description() const {
  return description_;
}

std::optional<base::DictValue> ScriptTool::InputProperties() const {
  if (!input_properties_) {
    return std::nullopt;
  }
  return input_properties_->Clone();
}

std::optional<std::vector<std::string>> ScriptTool::RequiredProperties() const {
  if (required_properties_.empty()) {
    return std::nullopt;
  }
  return required_properties_;
}

void ScriptTool::UseTool(const std::string& input_json,
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
  extractor_ptr->ExecuteScriptTool(
      name_, input_json,
      base::BindOnce(
          [](UseToolCallback cb, mojo::Remote<mojom::PageContentExtractor>,
             const std::optional<std::string>& result) {
            std::move(cb).Run(CreateContentBlocksForText(result.value_or("")),
                              {});
          },
          std::move(callback), std::move(extractor)));
}

}  // namespace ai_chat
