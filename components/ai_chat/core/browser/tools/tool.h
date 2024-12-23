// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/values.h"

namespace ai_chat {

class ConversationHandler;

// Base class for Tools that are exposed to the Assistant
class Tool {
 public:

  // output_json, delay in milliseconds
  using UseToolCallback =
      base::OnceCallback<void(std::optional<std::string_view> out, int delay)>;

  Tool() = default;
  virtual ~Tool() = default;

  Tool(const Tool&) = delete;
  Tool& operator=(const Tool&) = delete;

  // Should be a unique name for the tool
  virtual std::string_view name() const = 0;

  // Description for the Assistant to understand the purpose of the tool
  virtual std::string_view description() const = 0;

  // Type of the tool, usually left as default "function"
  virtual std::string_view type() const;

  // If the tool accepts parameters, they should be defined in JSON Schema
  // format.
  virtual std::optional<std::string> GetInputSchemaJson() const;

  // A list of properties contained within GetInputSchemaJson that are required
  virtual std::optional<std::vector<std::string>> required_properties() const;

  // If this tool requires content associated, it won't be provided if
  // used in a conversation without content association.
  virtual bool IsContentAssociationRequired() const;

  virtual bool IsAgentTool() const;

  // If this tool requires a user to interact with it before a response will
  // be sent to the Assistant.
  virtual bool RequiresUserInteractionBeforeHandling() const;

  // Parameters this tool provides when being defined for the Assistant
  virtual std::optional<base::Value> extra_params() const;

  // Implementers should handle tool execution unless it is a built-in
  // tool handled directly by the ConversationHandler.
  virtual void UseTool(const std::string& input_json, UseToolCallback callback);
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_H_
