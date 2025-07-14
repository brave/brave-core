// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"

namespace ai_chat {

// Base class for Tools that are exposed to the Assistant
class Tool {
 public:
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

  // If this tool requires content associated, it won't be provided if
  // used in a conversation without content association.
  virtual bool IsContentAssociationRequired() const;

  // If this tool is an agent tool, it will only be available to
  // conversations using the agent mode instead of the chat mode.
  virtual bool IsAgentTool() const;

  // Implementor can check features of the model to determine if the tool is
  // supported.
  virtual bool IsSupportedByModel(const mojom::Model& model) const;

  // If this tool requires a user to interact with it before a response will
  // be sent to the Assistant. This can be for permission or because
  // the tool requires the user to take some action to provide the result.
  virtual bool RequiresUserInteractionBeforeHandling() const;

  base::WeakPtr<Tool> GetWeakPtr() { return weak_ptr_factory_.GetWeakPtr(); }

 protected:
  base::WeakPtrFactory<Tool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TOOL_H_
