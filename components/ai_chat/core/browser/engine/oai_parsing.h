// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/e2ee_processor.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_PARSING_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_PARSING_H_

namespace ai_chat {

// Construct tool use events from tool calls (tool requests with function)
std::vector<mojom::ToolUseEventPtr> ToolUseEventFromToolCallsResponse(
    const base::ListValue* tool_calls_api_response);

// Parse a single tool call dict with function (tool request)
std::optional<mojom::ToolUseEventPtr> ParseToolCallRequest(
    const base::DictValue& tool_call);

// Parse a single tool call dict with output_content (tool result).
// Returns a vector of ConversationEntryEvents: always a ToolUseEvent,
// optionally followed by WebSourcesEvent and SearchQueriesEvent if
// the output contains web sources. Returns empty vector on failure.
std::vector<mojom::ConversationEntryEventPtr> ParseToolCallResult(
    const base::DictValue& tool_call);

// Extract WebSourcesEvent and SearchQueriesEvent from content
// blocks (e.g. a tool's output). Collects sources, queries, and
// rich_results across all WebSourcesContentBlocks. Returns empty
// vector if no web sources are found.
std::vector<mojom::ConversationEntryEventPtr> ExtractWebSourceEvents(
    const std::vector<mojom::ContentBlockPtr>& content_blocks);

// Parse a JSON dict into ContentBlock based on its "type" field.
// Supports "text" and "brave-chat.webSources" types.
// Returns nullopt if type is unsupported or parsing fails.
std::optional<mojom::ContentBlockPtr> ParseContentBlockFromDict(
    const base::DictValue& dict);

// Convert some Tools to Chat API-style JSON list of tool definitions
std::optional<base::ListValue> ToolApiDefinitionsFromTools(
    const std::vector<base::WeakPtr<Tool>>& tools);

// Extract the content container (delta or message) from an OpenAI response.
// Returns nullptr if the response doesn't follow OpenAI format.
const base::DictValue* GetOAIContentContainer(const base::DictValue& response);

// Parse tool calls from an OAI-format streaming or non-streaming response.
// Handles both tool requests (with function) and server tool results
// (with output_content). Returns a vector of GenerationResultData entries.
std::vector<EngineConsumer::GenerationResultData> ParseToolCallsFromOAIResponse(
    const base::DictValue& response,
    std::optional<std::string> model_key);

// Parse OpenAI-format completion response for both streaming and
// non-streaming requests. Response can have either delta.content (streaming)
// or message.content (non-streaming).
// model_key: Optional, will be propagated into returned result.
// decrypt_callback: When provided, called on the raw content chunk to decrypt
// it before building the completion event.
std::optional<EngineConsumer::GenerationResultData> ParseOAICompletionResponse(
    const base::DictValue& response,
    std::optional<std::string> model_key,
    const E2EEProcessor::DecryptCallback& decrypt_callback = {});

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_PARSING_H_
