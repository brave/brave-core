// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"

namespace ai_chat {

// Base class for Tools that are exposed to the Assistant
class Tool {
 public:
  using ToolResult = std::vector<mojom::ContentBlockPtr>;
  using UseToolCallback = base::OnceCallback<void(ToolResult output)>;

  Tool();
  virtual ~Tool();

  Tool(const Tool&) = delete;
  Tool& operator=(const Tool&) = delete;

  // Should be a unique name for the tool
  virtual std::string_view Name() const = 0;

  // Description for the Assistant to understand the purpose of the tool
  virtual std::string_view Description() const = 0;

  // Type of the tool, usually left as default "function"
  virtual std::string_view Type() const;

  // If the tool accepts parameters, they should be defined in JSON Schema
  // format, e.g. `{ "location": { "type": "string", "description": "location
  // for weather" } }`
  // Implementors are encouraged to use the helpers at tool_input_properties.h
  // in order to create valid JSON Schema properties, e.g.
  // `StringProperty("Location for weather")`
  // or
  // `ArrayProperty("List of locations", StringProperty("A city name"))`
  // or
  // `ObjectProperty("Coordinates in the world", {{"lattitude",
  // StringProperty()},
  //  "longitude", StringProperty()}})`
  virtual std::optional<base::Value::Dict> InputProperties() const;

  // A list of properties contained within GetInputSchemaJson that are required
  virtual std::optional<std::vector<std::string>> RequiredProperties() const;

  // Parameters for remote-defined tools that this client provides, e.g. screen
  // width, location, etc. This normally applies for non-function type tools,
  // since for function type tools, the description includes any information
  // needed, but for remote-defined tools, the description might need to be
  // built to include some extra parameters that only the client knows about,
  // e.g. location for a search tool, or screen size for a computer use tool.
  virtual std::optional<base::Value::Dict> ExtraParams() const;

  // If this tool is an agent tool, it will only be available to
  // conversations using the agent mode instead of the chat mode.
  virtual bool IsAgentTool() const;

  // Implementor can check features of the model to determine if the tool is
  // supported.
  virtual bool IsSupportedByModel(const mojom::Model& model) const;

  // Check if this tool requires user interaction before handling
  // Returns:
  //  - bool(false): No interaction needed, tool can execute immediately
  //  - bool(true): Needs user to provide output (e.g. UserChoiceTool)
  //  - mojom::PermissionChallengePtr: Needs user permission with the given
  //    challenge.
  virtual std::variant<bool, mojom::PermissionChallengePtr>
  RequiresUserInteractionBeforeHandling(
      const mojom::ToolUseEvent& tool_use) const;

  // Called after user grants permission when a tool requires a
  // PermissionChallenge. Tools can override to perform any setup needed
  // before UseTool is called.
  virtual void UserPermissionGranted(const std::string& tool_use_id);

  // Whether this tool supports the given conversation. Can be used to filter
  // tools based on conversation properties like temporary status.
  virtual bool SupportsConversation(
      bool is_temporary,
      bool has_untrusted_content,
      mojom::ConversationCapability conversation_capability) const;

  // Implementers should handle tool execution unless it is a built-in
  // tool handled directly by the ConversationHandler.
  virtual void UseTool(const std::string& input_json, UseToolCallback callback);

  base::WeakPtr<Tool> GetWeakPtr() { return weak_ptr_factory_.GetWeakPtr(); }

 protected:
  base::WeakPtrFactory<Tool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_H_
