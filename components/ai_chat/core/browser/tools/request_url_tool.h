// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_REQUEST_URL_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_REQUEST_URL_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "base/functional/callback.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"
#include "url/gurl.h"

namespace ai_chat {

// Tool that allows the AI assistant to request a URL be attached to the
// conversation as associated content. Once attached, the page content will be
// available for subsequent turns.
//
// Each invocation requires explicit user confirmation before the fetch
// proceeds.
class RequestURLTool : public Tool {
 public:
  // Invoked when the AI requests a URL attachment. The implementation should
  // load the page and add it via AssociatedContentManager, then call
  // |on_complete| with the fetched page text once it is available (empty
  // string on failure).
  using AttachURLCallback = base::RepeatingCallback<void(
      GURL url,
      std::string title,
      base::OnceCallback<void(std::string)> on_complete)>;

  explicit RequestURLTool(AttachURLCallback attach_url_callback);
  ~RequestURLTool() override;

  RequestURLTool(const RequestURLTool&) = delete;
  RequestURLTool& operator=(const RequestURLTool&) = delete;

  // Tool overrides
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
  AttachURLCallback attach_url_callback_;
  // The tool_use_id for which the user most recently granted permission.
  // Cleared after UseTool consumes it so each URL requires fresh approval.
  std::string approved_tool_use_id_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_REQUEST_URL_TOOL_H_
