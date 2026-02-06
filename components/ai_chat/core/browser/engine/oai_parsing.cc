// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {

constexpr char kAllowedWebSourceFaviconHost[] = "imgs.search.brave.com";

std::vector<mojom::ToolUseEventPtr> ToolUseEventFromToolCallsResponse(
    const base::ListValue* tool_calls_api_response) {
  // https://platform.openai.com/docs/api-reference/chat/create#chat-create-tools
  // https://platform.openai.com/docs/api-reference/chat/object
  // choices -> message -> tool_calls
  std::vector<mojom::ToolUseEventPtr> tool_use_events;
  for (auto& tool_call_raw : *tool_calls_api_response) {
    if (!tool_call_raw.is_dict()) {
      DVLOG(1) << "Tool call is not a dictionary.";
      continue;
    }
    const auto& tool_call = tool_call_raw.GetDict();

    auto tool_use_event = ParseToolCallRequest(tool_call);
    if (!tool_use_event) {
      continue;
    }

    tool_use_events.push_back(std::move(*tool_use_event));
  }

  return tool_use_events;
}

std::optional<mojom::ToolUseEventPtr> ParseToolCallRequest(
    const base::DictValue& tool_call) {
  const base::DictValue* function = tool_call.FindDict("function");
  if (!function) {
    DVLOG(1) << "No function info found in tool call.";
    return std::nullopt;
  }

  // Most APIs that have partial chunk responses seem to initially always have
  // the tool name and id, and only chunk the arguments json. So whilst id and
  // name are required for the completed event, we can't rely on them being
  // present for parsing.
  const std::string* id = tool_call.FindString("id");
  const std::string* name = function->FindString("name");

  mojom::ToolUseEventPtr tool_use_event = mojom::ToolUseEvent::New(
      name ? *name : "", id ? *id : "", "", std::nullopt, nullptr, false);

  const std::string* arguments_raw = function->FindString("arguments");
  if (arguments_raw) {
    tool_use_event->arguments_json = *arguments_raw;
  }

  // Check for alignment_check within this tool call
  if (const base::DictValue* alignment_dict =
          tool_call.FindDict("alignment_check")) {
    if (!alignment_dict->FindBool("allowed").value_or(true)) {
      const std::string* assessment = alignment_dict->FindString("reasoning");
      tool_use_event->permission_challenge = mojom::PermissionChallenge::New(
          assessment ? std::make_optional(*assessment) : std::nullopt,
          std::nullopt);
    }
  }

  return tool_use_event;
}

std::optional<mojom::ContentBlockPtr> ParseContentBlockFromDict(
    const base::DictValue& dict) {
  const std::string* type = dict.FindString("type");
  if (!type) {
    return std::nullopt;
  }

  if (*type == "text") {
    const std::string* text = dict.FindString("text");
    if (!text) {
      return std::nullopt;
    }
    return mojom::ContentBlock::NewTextContentBlock(
        mojom::TextContentBlock::New(*text));
  }

  if (*type == "brave-chat.webSources") {
    std::vector<mojom::WebSourcePtr> sources;
    const base::ListValue* sources_list = dict.FindList("sources");
    if (sources_list) {
      for (const auto& source_value : *sources_list) {
        if (!source_value.is_dict()) {
          continue;
        }
        const auto& source_dict = source_value.GetDict();
        const std::string* title = source_dict.FindString("title");
        const std::string* url_str = source_dict.FindString("url");
        const std::string* favicon_str = source_dict.FindString("favicon");

        if (!title || !url_str) {
          DVLOG(1) << "Missing required fields in webSource";
          continue;
        }

        GURL url(*url_str);
        GURL favicon_url = favicon_str ? GURL(*favicon_str)
                                       : GURL(
                                             "chrome-untrusted://resources/"
                                             "brave-icons/globe.svg");

        if (!url.is_valid() || !favicon_url.is_valid()) {
          DVLOG(2) << "Invalid URL in webSource";
          continue;
        }

        // Validate favicon is from allowed source
        if (favicon_str &&
            (!favicon_url.SchemeIs(url::kHttpsScheme) ||
             base::CompareCaseInsensitiveASCII(
                 favicon_url.host(), kAllowedWebSourceFaviconHost) != 0)) {
          DVLOG(2) << "favicon contained disallowed host or scheme";
          continue;
        }

        sources.push_back(mojom::WebSource::New(*title, std::move(url),
                                                std::move(favicon_url)));
      }
    }

    std::optional<std::string> query;
    const std::string* query_str = dict.FindString("query");
    if (query_str && !query_str->empty()) {
      query = *query_str;
    }

    // Return nullopt if nothing useful was parsed
    if (sources.empty() && !query.has_value()) {
      return std::nullopt;
    }

    return mojom::ContentBlock::NewWebSourcesContentBlock(
        mojom::WebSourcesContentBlock::New(std::move(sources),
                                           std::move(query)));
  }

  return std::nullopt;
}

std::optional<mojom::ToolUseEventPtr> ParseToolCallResult(
    const base::DictValue& tool_call) {
  const base::ListValue* output_content = tool_call.FindList("output_content");
  if (!output_content) {
    DVLOG(1) << "No output_content found in tool result.";
    return std::nullopt;
  }

  const std::string* id = tool_call.FindString("id");
  if (!id) {
    DVLOG(1) << "Tool result missing required id field.";
    return std::nullopt;
  }

  mojom::ToolUseEventPtr tool_use_event =
      mojom::ToolUseEvent::New("", *id, "", std::nullopt, nullptr, true);

  tool_use_event->output = std::vector<mojom::ContentBlockPtr>();
  for (const auto& output_item : *output_content) {
    if (!output_item.is_dict()) {
      continue;
    }

    auto content_block = ParseContentBlockFromDict(output_item.GetDict());
    if (content_block.has_value()) {
      tool_use_event->output->push_back(std::move(*content_block));
    } else {
      // Unsupported type - store as TextContentBlock with JSON serialization
      std::string item_json;
      if (base::JSONWriter::Write(output_item, &item_json)) {
        tool_use_event->output->push_back(
            mojom::ContentBlock::NewTextContentBlock(
                mojom::TextContentBlock::New(item_json)));
      }
    }
  }

  return tool_use_event;
}

std::optional<base::ListValue> ToolApiDefinitionsFromTools(
    const std::vector<base::WeakPtr<Tool>>& tools) {
  if (tools.empty()) {
    return std::nullopt;
  }
  base::ListValue tools_list;
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

    base::DictValue tool_dict;

    bool type_is_funcion = tool->Type().empty() || tool->Type() == "function";
    tool_dict.Set("type", type_is_funcion ? "function" : tool->Type());

    if (type_is_funcion) {
      base::DictValue function_dict;
      function_dict.Set("name", tool->Name());

      if (!tool->Description().empty()) {
        function_dict.Set("description", tool->Description());
      }
      auto input_schema = tool->InputProperties();
      if (input_schema) {
        // input_schema contains the properties dict from the tool.
        // Wrap it in a proper JSON Schema object format.
        base::DictValue parameters;
        parameters.Set("type", "object");
        parameters.Set("properties", std::move(input_schema.value()));

        // We don't have any validation on parameters and required objects
        // as enforcing to JSON Schema is done by the remote and is non
        // fatal for the client.
        if (tool->RequiredProperties().has_value() &&
            !tool->RequiredProperties()->empty()) {
          base::ListValue required_properties;
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
      // and any "extra_param". The use case for this is remote-defined
      // tools that have different parameters to creat the tool description,
      // e.g. for screen size or user's locale.
      tool_dict.Set("name", tool->Name());
      if (tool->ExtraParams().has_value()) {
        tool_dict.Merge(tool->ExtraParams().value());
      }
    }
    tools_list.Append(std::move(tool_dict));
  }
  return tools_list;
}

const base::DictValue* GetOAIContentContainer(const base::DictValue& response) {
  const base::ListValue* choices = response.FindList("choices");
  if (!choices || choices->empty() || !choices->front().is_dict()) {
    VLOG(2) << "No choices list found in response, or it is empty.";
    return nullptr;
  }

  const base::DictValue& choice = choices->front().GetDict();

  // Response can have either "delta" or "message" field
  const base::DictValue* content_container = choice.FindDict("delta");
  if (!content_container) {
    content_container = choice.FindDict("message");
  }

  return content_container;
}

std::optional<EngineConsumer::GenerationResultData> ParseOAICompletionResponse(
    const base::DictValue& response,
    std::optional<std::string> model_key) {
  const base::DictValue* content_container = GetOAIContentContainer(response);
  if (!content_container) {
    VLOG(2) << "No delta or message info found in first completion choice.";
    return std::nullopt;
  }

  const std::string* content = content_container->FindString("content");
  if (!content || content->empty()) {
    return std::nullopt;
  }

  auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New(*content));
  return EngineConsumer::GenerationResultData(std::move(event),
                                              std::move(model_key));
}

}  // namespace ai_chat
