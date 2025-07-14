// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"

#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

namespace ai_chat {

std::vector<mojom::ToolUseEventPtr> ToolUseEventFromToolCallsResponse(
    const base::Value::List* tool_calls_api_response) {
  // https://platform.openai.com/docs/api-reference/chat/create#chat-create-tools
  // https://platform.openai.com/docs/api-reference/chat/object
  // choices -> message -> tool_calls
  std::vector<mojom::ToolUseEventPtr> tool_use_events;
  for (auto& tool_call_raw : *tool_calls_api_response) {
    if (!tool_call_raw.is_dict()) {
      DLOG(ERROR) << "Tool call is not a dictionary.";
      continue;
    }
    const auto& tool_call = tool_call_raw.GetDict();

    // Most APIs that have partial chunk responses seem to initially always have
    // the tool name and id, and only chunk the arguments json. So, for now, we
    // can continue to have validation here and ignore calls that don't have
    // required fields.

    const base::Value::Dict* function = tool_call.FindDict("function");
    if (!function) {
      DLOG(ERROR) << "No function info found in tool call.";
      continue;
    }
    // Tool call results can be partial and should be added to the previous
    // event by the event handler.
    const std::string* id = tool_call.FindString("id");
    if (!id) {
      DLOG(ERROR) << "No id found in tool call.";
      continue;
    }

    const std::string* name = function->FindString("name");
    if (!name) {
      DLOG(ERROR) << "No name found in tool call.";
      continue;
    }

    mojom::ToolUseEventPtr tool_use_event =
        mojom::ToolUseEvent::New(*name, *id, "", std::nullopt, std::nullopt);

    // Arguments are provided as a JSON string. We parse it here so that the
    // rest of the code doesn't have to parse it potentially multiple times
    // (performance), and we can add some protections.
    const std::string* arguments_raw = function->FindString("arguments");
    if (arguments_raw) {
      tool_use_event->arguments_json = *arguments_raw;
    }

    tool_use_events.push_back(std::move(tool_use_event));
  }

  return tool_use_events;
}

std::optional<base::Value::List> ToolApiDefinitionsFromTools(
    const std::vector<base::WeakPtr<Tool>>& tools) {
  if (tools.empty()) {
    return std::nullopt;
  }
  base::Value::List tools_list;
  for (const base::WeakPtr<Tool> tool : tools) {
    if (!tool) {
      DLOG(ERROR) << "Tool is null, skipping tool.";
      continue;
    }
    // Every tool needs a name otherwise it isn't useable
    if (tool->Name().empty()) {
      DLOG(ERROR) << "Tool name is empty, skipping tool.";
      continue;
    }

    base::Value::Dict tool_dict;

    bool type_is_funcion = tool->Type().empty() || tool->Type() == "function";
    tool_dict.Set("type", type_is_funcion ? "function" : tool->Type());

    if (type_is_funcion) {
      base::Value::Dict function_dict;
      function_dict.Set("name", tool->Name());

      if (!tool->Description().empty()) {
        function_dict.Set("description", tool->Description());
      }
      auto input_schema = tool->InputProperties();
      if (input_schema) {
        // input_schema contains the properties dict from the tool.
        // Wrap it in a proper JSON Schema object format.
        base::Value::Dict parameters;
        parameters.Set("type", "object");
        parameters.Set("properties", std::move(input_schema.value()));

        // We don't have any validation on parameters and required objects
        // as enforcing to JSON Schema is done by the remote and is non
        // fatal for the client.
        if (tool->RequiredProperties().has_value() &&
            !tool->RequiredProperties()->empty()) {
          base::Value::List required_properties;
          const auto properties = tool->RequiredProperties().value();
          for (const auto& property : properties) {
            required_properties.Append(property);
          }

          parameters.Set("required", std::move(required_properties));
        }

        function_dict.Set("parameters", std::move(parameters));
      }
      tool_dict.Set("function", std::move(function_dict));
    } else {
      // For non-known types (anything not "function"), we send name, type
      // and any "extra_param". The use case for this is custom anthropic
      // tools that have different parameters each time it's defined, e.g.
      // for screen size.
      tool_dict.Set("name", tool->Name());
      if (tool->ExtraParams().has_value()) {
        tool_dict.Merge(tool->ExtraParams().value());
      }
    }
    tools_list.Append(std::move(tool_dict));
  }
  return tools_list;
}

}  // namespace ai_chat
