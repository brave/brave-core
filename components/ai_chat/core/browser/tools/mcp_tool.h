// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MCP_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MCP_TOOL_H_

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/mcp/mcp_types.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/mcp.mojom-forward.h"
#include "url/origin.h"

namespace ai_chat {

class MCPClient;

// Bedrock rejects tool names longer than this.
inline constexpr size_t kMaxMCPToolNameLength = 64;

// Bounds on a tool's input schema, which is sent to the model's API with every
// request, and may be used there to check the model's arguments. A tool whose
// schema exceeds them isn't offered.
// How deeply JSON objects and arrays may nest.
inline constexpr size_t kMaxMCPToolSchemaDepth = 16;
// How many JSON objects the schema may have, which bounds its subschemas.
inline constexpr size_t kMaxMCPToolSchemaObjects = 256;
// How many properties all of the schema's `properties` may have together.
inline constexpr size_t kMaxMCPToolSchemaProperties = 128;
// How long the schema may be once serialized.
inline constexpr size_t kMaxMCPToolSchemaBytes = 16 * 1024;

// How much of a tool's title and description the model is given.
inline constexpr size_t kMaxMCPToolTitleBytes = 256;
inline constexpr size_t kMaxMCPToolDescriptionBytes = 4 * 1024;
// How much of the text a tool returns, across its content, the model is given.
inline constexpr size_t kMaxMCPToolOutputTextBytes = 64 * 1024;

// A tool offered by a remote MCP server, called through the server's client.
// Everything the server says about the tool is untrusted.
class MCPTool : public Tool {
 public:
  // Returns null if the tool's input schema exceeds the bounds, has
  // `properties` that aren't an object, or has a property name that a model's
  // API would reject.
  static std::unique_ptr<MCPTool> Create(const mojom::MCPServer& server,
                                         const MCPToolDefinition& definition,
                                         base::WeakPtr<MCPClient> client);

  // The tools of `server`, leaving out those that Create() rejects and any
  // with the Name() of an earlier one.
  static std::vector<std::unique_ptr<MCPTool>> CreateAll(
      const mojom::MCPServer& server,
      base::span<const MCPToolDefinition> definitions,
      base::WeakPtr<MCPClient> client);

  MCPTool(const MCPTool&) = delete;
  MCPTool& operator=(const MCPTool&) = delete;
  // Tells the server to stop work on calls in progress.
  ~MCPTool() override;

  // Tool:
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::string_view DisplayName() const override;
  std::string_view DisplayDescription() const override;
  std::optional<base::DictValue> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  std::variant<bool, mojom::PermissionChallengePtr>
  RequiresUserInteractionBeforeHandling(
      const mojom::ToolUseEvent& tool_use) const override;
  void UserPermissionGranted(
      const std::string& tool_use_id,
      const mojom::PermissionChallenge& challenge) override;
  void SetUserPermissionStrategy(mojom::ToolPermission permission) override;
  std::optional<std::string> GetPermissionChallengeDescription(
      const mojom::ToolUseEvent& tool_use) const override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;
  void StopToolUse() override;

 private:
  struct Call {
    Call();
    Call(Call&&) noexcept;
    Call& operator=(Call&&) noexcept;
    ~Call();

    UseToolCallback callback;
    // MCPClient::CallTool() returns this, to cancel the call.
    base::OnceClosure cancel;
  };

  MCPTool(const mojom::MCPServer& server,
          const MCPToolDefinition& definition,
          base::DictValue input_properties,
          std::vector<std::string> required_properties,
          base::WeakPtr<MCPClient> client);

  void OnCallToolResult(int call_id,
                        base::ScopedClosureRunner on_dropped,
                        base::expected<MCPCallToolResult, MCPError> result);
  // The client was destroyed with the call in progress.
  void OnCallDropped(int call_id);
  // Runs and forgets the callback of call `call_id`, if it's still waiting.
  void CompleteCall(int call_id, ToolResult result);
  // The server's name for the tool, escaped for markdown.
  std::string EscapedToolName() const;
  std::string ServerUnavailableMessage() const;

  // The tool's name and description as the server gives them, which the user
  // is shown.
  std::string tool_name_;
  std::string tool_description_;
  // The tool's name and description as the model is given them.
  std::string name_;
  std::string description_;
  std::optional<base::DictValue> input_properties_;
  std::vector<std::string> required_properties_;

  std::string server_name_;
  url::Origin server_origin_;
  base::WeakPtr<MCPClient> client_;

  // The tool use the user allowed once by answering a permission challenge.
  // Other uses of this tool, even with the same arguments, are asked again.
  std::optional<std::string> granted_tool_use_id_;
  // A kNeverAllow tool isn't offered, so it doesn't need handling here.
  mojom::ToolPermission user_permission_strategy_ = mojom::ToolPermission::kAsk;

  int last_call_id_ = 0;
  base::flat_map<int, Call> calls_;

  base::WeakPtrFactory<MCPTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MCP_TOOL_H_
