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
    return std::nullopt;
  }

  // Most APIs that have partial chunk responses seem to initially always have
  // the tool name and id, and only chunk the arguments json. So whilst id and
  // name are required for the completed event, we can't rely on them being
  // present for parsing.
  const std::string* id = tool_call.FindString("id");
  const std::string* name = function->FindString("name");

  mojom::ToolUseEventPtr tool_use_event =
      mojom::ToolUseEvent::New(name ? *name : "", id ? *id : "", "",
                               std::nullopt, std::nullopt, nullptr, false);

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
        GURL favicon_url = (favicon_str && !favicon_str->empty())
                               ? GURL(*favicon_str)
                               : GURL(
                                     "chrome-untrusted://resources/"
                                     "brave-icons/globe.svg");

        if (!url.is_valid() || !favicon_url.is_valid()) {
          DVLOG(2) << "Invalid URL in webSource";
          continue;
        }

        // Validate favicon is from allowed source
        if (favicon_str && !favicon_str->empty() &&
            (!favicon_url.SchemeIs(url::kHttpsScheme) ||
             base::CompareCaseInsensitiveASCII(
                 favicon_url.host(), kAllowedWebSourceFaviconHost) != 0)) {
          DVLOG(2) << "favicon contained disallowed host or scheme";
          continue;
        }

        std::optional<std::string> page_content;
        const auto* page_content_str = source_dict.FindString("page_content");
        if (page_content_str) {
          page_content = *page_content_str;
        }

        std::optional<std::vector<std::string>> extra_snippets;
        const auto* snippets_list = source_dict.FindList("extra_snippets");
        if (snippets_list) {
          std::vector<std::string> snippets;
          for (const auto& snippet : *snippets_list) {
            if (snippet.is_string()) {
              snippets.push_back(snippet.GetString());
            }
          }
          if (!snippets.empty()) {
            extra_snippets = std::move(snippets);
          }
        }

        sources.push_back(mojom::WebSource::New(
            *title, std::move(url), std::move(favicon_url),
            std::move(page_content), std::move(extra_snippets)));
      }
    }

    std::vector<std::string> queries;
    const base::Value* query_val = dict.Find("query");
    if (query_val) {
      if (query_val->is_string() && !query_val->GetString().empty()) {
        queries.push_back(query_val->GetString());
      } else if (query_val->is_list()) {
        for (const auto& item : query_val->GetList()) {
          if (item.is_string() && !item.GetString().empty()) {
            queries.push_back(item.GetString());
          }
        }
      }
    }

    std::vector<std::string> rich_results;
    const base::ListValue* rich_results_list = dict.FindList("rich_results");
    if (rich_results_list) {
      for (const auto& item : *rich_results_list) {
        if (!item.is_dict()) {
          continue;
        }
        std::string json;
        base::JSONWriter::Write(item, &json);
        rich_results.push_back(std::move(json));
      }
    }

    // Return nullopt if nothing useful was parsed
    if (sources.empty() && queries.empty()) {
      return std::nullopt;
    }

    return mojom::ContentBlock::NewWebSourcesContentBlock(
        mojom::WebSourcesContentBlock::New(
            std::move(sources), std::move(queries), std::move(rich_results)));
  }

  return std::nullopt;
}

std::vector<mojom::ConversationEntryEventPtr> ParseToolCallResult(
    const base::DictValue& tool_call) {
  const base::ListValue* output_content = tool_call.FindList("output_content");
  if (!output_content) {
    return {};
  }

  const std::string* id = tool_call.FindString("id");
  if (!id) {
    DVLOG(1) << "Tool result missing required id field.";
    return {};
  }

  mojom::ToolUseEventPtr tool_use_event = mojom::ToolUseEvent::New(
      "", *id, "", std::nullopt, std::nullopt, nullptr, true);

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

  // Build result: ToolUseEvent first, then any source/query
  // events extracted from the output content blocks.
  std::vector<mojom::ConversationEntryEventPtr> events;
  auto source_events = ExtractWebSourceEvents(*tool_use_event->output);
  events.push_back(mojom::ConversationEntryEvent::NewToolUseEvent(
      std::move(tool_use_event)));
  for (auto& source_event : source_events) {
    events.push_back(std::move(source_event));
  }
  return events;
}

std::vector<mojom::ConversationEntryEventPtr> ExtractWebSourceEvents(
    const std::vector<mojom::ContentBlockPtr>& content_blocks) {
  std::vector<mojom::WebSourcePtr> all_sources;
  std::vector<std::string> all_queries;
  std::vector<std::string> all_rich_results;

  for (const auto& block : content_blocks) {
    if (!block->is_web_sources_content_block()) {
      continue;
    }
    const auto& ws = block->get_web_sources_content_block();
    for (const auto& s : ws->sources) {
      all_sources.push_back(s.Clone());
    }
    for (const auto& q : ws->queries) {
      if (!q.empty()) {
        all_queries.push_back(q);
      }
    }
    for (const auto& r : ws->rich_results) {
      all_rich_results.push_back(r);
    }
  }

  std::vector<mojom::ConversationEntryEventPtr> events;
  if (!all_sources.empty()) {
    events.push_back(mojom::ConversationEntryEvent::NewSourcesEvent(
        mojom::WebSourcesEvent::New(std::move(all_sources),
                                    std::move(all_rich_results))));
  }
  if (!all_queries.empty()) {
    events.push_back(mojom::ConversationEntryEvent::NewSearchQueriesEvent(
        mojom::SearchQueriesEvent::New(std::move(all_queries))));
  }
  return events;
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

std::vector<EngineConsumer::GenerationResultData> ParseToolCallsFromOAIResponse(
    const base::DictValue& response,
    std::optional<std::string> model_key) {
  std::vector<EngineConsumer::GenerationResultData> results;
  const base::DictValue* content_container = GetOAIContentContainer(response);
  if (!content_container) {
    return results;
  }

  const base::ListValue* tool_calls = content_container->FindList("tool_calls");
  if (!tool_calls) {
    return results;
  }

  for (const auto& tool_call_value : *tool_calls) {
    if (!tool_call_value.is_dict()) {
      continue;
    }
    const auto& tool_call_dict = tool_call_value.GetDict();

    // Parse tool request or server tool result
    if (auto tool_use_event = ParseToolCallRequest(tool_call_dict)) {
      auto tool_event = mojom::ConversationEntryEvent::NewToolUseEvent(
          std::move(*tool_use_event));
      results.emplace_back(std::move(tool_event), model_key);
    } else {
      for (auto& event : ParseToolCallResult(tool_call_dict)) {
        results.emplace_back(std::move(event), model_key);
      }
    }
  }

  return results;
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
