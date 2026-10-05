// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/memory_semantic_search_tool.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/ai_chat/core/browser/embeddings/ai_chat_embeddings_service.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"

namespace ai_chat {

namespace {

constexpr char kPropertyQuery[] = "query";
constexpr char kPropertyCount[] = "count";

constexpr char kOutputKeyQuery[] = "query";
constexpr char kOutputKeyMemories[] = "memories";

constexpr int kDefaultResultCount = 5;
constexpr int kMaxResultCount = 20;

}  // namespace

namespace internal {

std::string BuildMemorySearchResultJson(
    const std::string& query,
    const std::vector<std::string>& memories) {
  base::ListValue memory_list;
  for (const std::string& memory : memories) {
    memory_list.Append(memory);
  }
  base::DictValue root;
  root.Set(kOutputKeyQuery, query);
  root.Set(kOutputKeyMemories, std::move(memory_list));
  std::string json;
  base::JSONWriter::Write(root, &json);
  return json;
}

}  // namespace internal

MemorySemanticSearchTool::MemorySemanticSearchTool(
    base::WeakPtr<AIChatEmbeddingsService> service)
    : service_(std::move(service)) {}

MemorySemanticSearchTool::~MemorySemanticSearchTool() = default;

std::string_view MemorySemanticSearchTool::Name() const {
  return mojom::kMemorySemanticSearchToolName;
}

std::string_view MemorySemanticSearchTool::Description() const {
  return "Searches the memories the user asked you to keep about them by "
         "meaning, and returns those related to the query as JSON, most "
         "related first. Use to recall what the user told you to remember "
         "about a topic. Runs entirely on-device.";
}

std::optional<base::DictValue> MemorySemanticSearchTool::InputProperties()
    const {
  return CreateInputProperties(
      {{kPropertyQuery,
        StringProperty("Natural language description of the topic to find "
                       "memories about.")},
       {kPropertyCount,
        IntegerProperty("Maximum number of memories to return. Defaults to "
                        "5; capped at 20.")}});
}

std::optional<std::vector<std::string>>
MemorySemanticSearchTool::RequiredProperties() const {
  return std::vector<std::string>{kPropertyQuery};
}

bool MemorySemanticSearchTool::IsSupportedByModel(
    const mojom::Model& model,
    const ConversationCapabilitySet& conversation_capabilities) const {
  if (!Tool::IsSupportedByModel(model, conversation_capabilities)) {
    return false;
  }
  // A model given its own system prompt isn't sent the user's memories.
  if (!model.options || !model.options->is_custom_model_options()) {
    return true;
  }
  const mojom::CustomModelOptions& options =
      *model.options->get_custom_model_options();
  return !options.model_system_prompt || options.model_system_prompt->empty();
}

bool MemorySemanticSearchTool::SupportsConversation(
    bool is_temporary,
    bool has_untrusted_content,
    const ConversationCapabilitySet& conversation_capabilities) const {
  // A temporary conversation isn't sent the user's memories.
  return !is_temporary;
}

void MemorySemanticSearchTool::UseTool(const std::string& input_json,
                                       UseToolCallback callback) {
  std::optional<base::DictValue> input = base::JSONReader::ReadDict(
      input_json, base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: failed to parse input JSON"), {});
    return;
  }
  const std::string* query = input->FindString(kPropertyQuery);
  if (!query || query->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: missing or empty 'query' field"),
        {});
    return;
  }
  if (!service_) {
    std::move(callback).Run(
        CreateContentBlocksForText("Error: searching memories is unavailable"),
        {});
    return;
  }
  const int count =
      std::clamp(input->FindInt(kPropertyCount).value_or(kDefaultResultCount),
                 1, kMaxResultCount);
  service_->SearchMemories(
      *query, static_cast<size_t>(count),
      base::BindOnce(&MemorySemanticSearchTool::OnSearchResults,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     *query));
}

void MemorySemanticSearchTool::OnSearchResults(
    UseToolCallback callback,
    const std::string& query,
    std::vector<std::string> memories) {
  std::move(callback).Run(
      CreateContentBlocksForText(
          internal::BuildMemorySearchResultJson(query, memories)),
      {});
}

}  // namespace ai_chat
