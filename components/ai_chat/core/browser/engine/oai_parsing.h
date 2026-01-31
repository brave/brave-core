// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_PARSING_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_PARSING_H_

namespace ai_chat {

// Construct a tool use event from a tool calls part of a Chat API-style
// response
std::vector<mojom::ToolUseEventPtr> ToolUseEventFromToolCallsResponse(
    const base::Value::List* tool_calls_api_response);

// Convert some Tools to Chat API-style JSON list of tool definitions
std::optional<base::Value::List> ToolApiDefinitionsFromTools(
    const std::vector<base::WeakPtr<Tool>>& tools);

// Extract the content container (delta or message) from an OpenAI response.
// Returns nullptr if the response doesn't follow OpenAI format.
const base::Value::Dict* GetOAIContentContainer(
    const base::Value::Dict& response);

// Parse OpenAI-format completion response for both streaming and
// non-streaming requests. Response can have either delta.content (streaming)
// or message.content (non-streaming).
// model_key: Optional, will be propergated into returned result.
std::optional<EngineConsumer::GenerationResultData> ParseOAICompletionResponse(
    const base::Value::Dict& response,
    std::optional<std::string> model_key);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_OAI_PARSING_H_
