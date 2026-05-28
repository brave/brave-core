// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_CONTENT_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_CONTENT_TOOL_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "content/public/browser/weak_document_ptr.h"
#include "third_party/blink/public/mojom/content_extraction/script_tools.mojom-forward.h"

namespace ai_chat {

// A page-defined content tool registered via the Model Context web API.
// Executes via the renderer's PageContentExtractor mojo interface.
class ContentTool : public Tool {
 public:
  ContentTool(const blink::mojom::ScriptTool& script_tool,
              content::WeakDocumentPtr rfh);
  ~ContentTool() override;

  ContentTool(const ContentTool&) = delete;
  ContentTool& operator=(const ContentTool&) = delete;

  // Tool overrides:
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::DictValue> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  std::variant<bool, mojom::PermissionChallengePtr>
  RequiresUserInteractionBeforeHandling(
      const mojom::ToolUseEvent& tool_use) const override;
  void UserPermissionGranted(const std::string& tool_use_id) override;

  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  bool user_permission_granted_ = false;

  content::WeakDocumentPtr rfh_;
  std::string internal_tool_name_;
  std::string name_;
  std::string description_;
  std::optional<base::DictValue> input_properties_;
  std::vector<std::string> required_properties_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_CONTENT_TOOL_H_
