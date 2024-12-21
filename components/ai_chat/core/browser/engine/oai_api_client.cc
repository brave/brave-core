// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_api_client.h"

#include <ios>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/strcat.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/constants/brave_services_key.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat", R"(
      semantics {
        sender: "AI Chat"
        description:
          "This is used to communicate with user-provided server url"
          "on behalf of the user interacting with different browser AI"
          "features."
        trigger:
          "Triggered by user interactions such as submitting an AI Chat"
          "conversation message, or requesting a text rewrite."
        data:
          "Conversational messages input by the user as well as associated"
          "content or user text to be rewritten. Can contain PII."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

std::string CreateJSONRequestBody(
    base::Value::List messages,
    EngineConsumer::Tools tools,
    std::optional<std::string_view> preferred_tool_name,
    const bool is_sse_enabled,
    const mojom::CustomModelOptions& model_options) {
  base::Value::Dict dict;

  dict.Set("messages", std::move(messages));
  dict.Set("stream", is_sse_enabled);
  dict.Set("temperature", 0.7);
  dict.Set("model", model_options.model_request_name);

  if (!tools.empty()) {
    base::Value::List tools_list;
    for (const auto& tool : tools) {
      base::Value::Dict tool_dict;

      tool_dict.Set("type", tool->type().empty() ? "function" : tool->type());

      if (!tool->name().empty()) {
        base::Value::Dict function_dict;
        function_dict.Set("name", tool->name());

        if (!tool->description().empty()) {
          function_dict.Set("description", tool->description());
        }
        auto input_schema = tool->GetInputSchemaJson();
        if (input_schema) {
          // input_schema is string of JSON Schema for the input properties
          // of the Tool. Set it on the "parameters" field as actual Value JSON.
          std::optional<base::Value> parameters =
              base::JSONReader::Read(input_schema.value());
          CHECK(parameters)
              << "Failed to parse input schema for tool: " << tool->name();

          function_dict.Set("parameters", std::move(*parameters));

          if (tool->required_properties().has_value() &&
              !tool->required_properties()->empty()) {
            base::Value::List required_properties;
            const auto properties = tool->required_properties().value();
            for (const auto& property : properties) {
              required_properties.Append(property);
            }
            function_dict.Set("required", std::move(required_properties));
          }
        }

        tool_dict.Set("function", std::move(function_dict));
      }
      tools_list.Append(std::move(tool_dict));
    }
    dict.Set("tools", std::move(tools_list));
  }

  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

}  // namespace

OAIAPIClient::OAIAPIClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_request_helper_ = std::make_unique<api_request_helper::APIRequestHelper>(
      GetNetworkTrafficAnnotationTag(), url_loader_factory);
}

OAIAPIClient::~OAIAPIClient() = default;

void OAIAPIClient::ClearAllQueries() {
  api_request_helper_->CancelAll();
}

void OAIAPIClient::PerformRequest(
    const mojom::CustomModelOptions& model_options,
    base::Value::List messages,
    EngineConsumer::Tools tools,
    std::optional<std::string_view> preferred_tool_name,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback) {
  if (!model_options.endpoint.is_valid()) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  const bool is_sse_enabled =
      ai_chat::features::kAIChatSSE.Get() && !data_received_callback.is_null();
  const std::string request_body =
      CreateJSONRequestBody(std::move(messages), std::move(tools),
                            preferred_tool_name, is_sse_enabled, model_options);
  base::flat_map<std::string, std::string> headers;
  if (!model_options.api_key.empty()) {
    headers.emplace("Authorization",
                    base::StrCat({"Bearer ", model_options.api_key}));
  }

  if (is_sse_enabled) {
    auto on_received = base::BindRepeating(&OAIAPIClient::OnQueryDataReceived,
                                           weak_ptr_factory_.GetWeakPtr(),
                                           std::move(data_received_callback));
    auto on_complete = base::BindOnce(&OAIAPIClient::OnQueryCompleted,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      std::move(completed_callback));

    api_request_helper_->RequestSSE(net::HttpRequestHeaders::kPostMethod,
                                    model_options.endpoint, request_body,
                                    "application/json", std::move(on_received),
                                    std::move(on_complete), headers, {});
  } else {
    auto on_complete = base::BindOnce(&OAIAPIClient::OnQueryCompleted,
                                      weak_ptr_factory_.GetWeakPtr(),
                                      std::move(completed_callback));
    api_request_helper_->Request(
        net::HttpRequestHeaders::kPostMethod, model_options.endpoint,
        request_body, "application/json", std::move(on_complete), headers, {});
  }
}

void OAIAPIClient::OnQueryCompleted(GenerationCompletedCallback callback,
                                    APIRequestResult result) {
  const bool success = result.Is2XXResponseCode();
  // Handle successful request
  if (success) {
    std::string completion = "";
    // We're checking for a value body in case for non-streaming API results.
    if (result.value_body().is_dict()) {
      const base::Value::List* choices =
          result.value_body().GetDict().FindList("choices");
      if (!choices) {
        DVLOG(2) << "No choices list found in response.";
        return;
      }
      if (choices->front().is_dict()) {
        const base::Value::Dict* message =
            choices->front().GetDict().FindDict("message");
        if (!message) {
          DVLOG(2) << "No message dict found in response.";
          return;
        }
        completion = *message->FindString("content");
      }
    }

    std::move(callback).Run(base::ok(std::move(completion)));
    return;
  }

  // Handle error
  std::move(callback).Run(base::unexpected(mojom::APIError::ConnectionIssue));
}

void OAIAPIClient::OnQueryDataReceived(
    GenerationDataCallback callback,
    base::expected<base::Value, std::string> result) {
  if (!result.has_value() || !result->is_dict()) {
    return;
  }

  const base::Value::List* choices = result->GetDict().FindList("choices");

  if (!choices || choices->empty()) {
    VLOG(2) << "No choices list found in response, or it is empty.";
    return;
  }

  // We only support 1 choice
  const auto& choice = choices->front();
  if (!choice.is_dict()) {
    VLOG(2) << "First choice is not a dict.";
    return;
  }
  const base::Value::Dict* delta = choice.GetDict().FindDict("delta");

  if (!delta) {
    VLOG(2) << "No delta info found in first completion choice.";
    return;
  }

  const std::string* content = delta->FindString("content");
  if (content && !content->empty()) {
    auto event = mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(*content));
    callback.Run(std::move(event));
  }

  if (const base::Value::List* tool_calls = delta->FindList("tool_calls")) {
    // https://platform.openai.com/docs/api-reference/chat/create#chat-create-tools
    for (auto& tool_call_raw : *tool_calls) {
      if (!tool_call_raw.is_dict()) {
        continue;
      }
      const auto& tool_call = tool_call_raw.GetDict();
      mojom::ToolUseEventPtr tool_use_event = mojom::ToolUseEvent::New();
      const base::Value::Dict* function = tool_call.FindDict("function");
      if (!function) {
        DVLOG(0) << "No function info found in tool call.";
        continue;
      }
      // Tool call results can be partial and should be added to the previous
      // event by the event handler.
      const std::string* id = tool_call.FindString("id");
      if (id) {
        tool_use_event->tool_id = *id;
      }

      const std::string* name = function->FindString("name");
      if (name) {
        tool_use_event->tool_name = *name;
      }
      const std::string* arguments = function->FindString("arguments");
      if (arguments) {
        tool_use_event->input_json = *arguments;
      }

      auto event = mojom::ConversationEntryEvent::NewToolUseEvent(
          std::move(tool_use_event));
      callback.Run(std::move(event));
    }
  }
}

}  // namespace ai_chat
