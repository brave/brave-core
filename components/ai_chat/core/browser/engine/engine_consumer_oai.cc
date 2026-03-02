// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_oai.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/barrier_callback.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/i18n/time_formatting.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/oai_api_client.h"
#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

namespace {

bool HasCustomSystemPrompt(const mojom::CustomModelOptions& model_options) {
  return model_options.model_system_prompt &&
         !model_options.model_system_prompt->empty();
}

}  // namespace

EngineConsumerOAIRemote::EngineConsumerOAIRemote(
    const mojom::CustomModelOptions& model_options,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    ModelService* model_service,
    PrefService* prefs)
    : EngineConsumer(model_service, prefs) {
  model_options_ = model_options;
  max_associated_content_length_ = model_options.max_associated_content_length;

  // Initialize the API client
  api_ = std::make_unique<OAIAPIClient>(url_loader_factory);
}

EngineConsumerOAIRemote::~EngineConsumerOAIRemote() = default;

void EngineConsumerOAIRemote::ClearAllQueries() {
  api_->ClearAllQueries();
}

bool EngineConsumerOAIRemote::SupportsDeltaTextResponses() const {
  return true;
}

bool EngineConsumerOAIRemote::RequiresClientSideTitleGeneration() const {
  return true;  // OAI engines need client-side title generation
}

void EngineConsumerOAIRemote::UpdateModelOptions(
    const mojom::ModelOptions& options) {
  if (options.is_custom_model_options()) {
    model_options_ = *options.get_custom_model_options();
    max_associated_content_length_ =
        model_options_.max_associated_content_length;
  }
}

void EngineConsumerOAIRemote::GenerateRewriteSuggestion(
    const std::string& text,
    mojom::ActionType action_type,
    GenerationDataCallback received_callback,
    GenerationCompletedCallback completed_callback) {
  auto messages = BuildOAIRewriteSuggestionMessages(text, action_type);
  if (!messages) {
    std::move(completed_callback)
        .Run(base::unexpected(mojom::APIError::InternalError));
    return;
  }

  messages->push_back(BuildOAISeedMessage(
      "Here is the requested rewritten version of the excerpt "
      "in <response> tags:\n<response>"));
  api_->PerformRequest(
      model_options_, std::move(*messages), std::move(received_callback),
      std::move(completed_callback), std::vector<std::string>{"</response>"});
}

void EngineConsumerOAIRemote::GenerateQuestionSuggestions(
    PageContents page_contents,
    SuggestedQuestionsCallback callback) {
  auto messages = BuildOAIQuestionSuggestionsMessages(
      page_contents, max_associated_content_length_,
      [this](std::string& input) { SanitizeInput(input); });

  messages.push_back(BuildOAISeedMessage(
      "Here are three questions the user may ask about the content "
      "in <question> tags:\n"));

  api_->PerformRequest(
      model_options_, std::move(messages), base::NullCallback(),
      base::BindOnce(
          &EngineConsumerOAIRemote::OnGenerateQuestionSuggestionsResponse,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void EngineConsumerOAIRemote::OnGenerateQuestionSuggestionsResponse(
    SuggestedQuestionsCallback callback,
    GenerationResult result) {
  if (!result.has_value()) {
    // Query resulted in error
    std::move(callback).Run(base::unexpected(std::move(result.error())));
    return;
  }

  if (!result->event || !result->event->is_completion_event() ||
      result->event->get_completion_event()->completion.empty()) {
    // No questions were generated
    std::move(callback).Run(base::unexpected(mojom::APIError::InternalError));
    return;
  }

  base::StringTokenizer tokenizer(
      result->event->get_completion_event()->completion, "</>");
  tokenizer.set_options(base::StringTokenizer::RETURN_DELIMS);

  std::vector<std::string> questions;

  while (tokenizer.GetNext()) {
    std::string token = tokenizer.token();
    std::string_view trimmed_token =
        base::TrimWhitespaceASCII(token, base::TrimPositions::TRIM_ALL);

    if (*tokenizer.token_begin() == '\n') {
      continue;
    }

    if (!tokenizer.token_is_delim() && trimmed_token != "question") {
      questions.emplace_back(trimmed_token);
    }
  }

  std::move(callback).Run(std::move(questions));
}

void EngineConsumerOAIRemote::GenerateConversationTitle(
    const PageContentsMap& page_contents,
    const ConversationHistory& conversation_history,
    GenerationCompletedCallback completed_callback) {
  auto messages = BuildOAIGenerateConversationTitleMessages(
      page_contents, conversation_history, max_associated_content_length_,
      [this](std::string& input) { SanitizeInput(input); });

  if (!messages) {
    std::move(completed_callback)
        .Run(base::unexpected(mojom::APIError::InternalError));
    return;
  }

  messages->push_back(BuildOAISeedMessage(
      "Here is the title for the above conversation in <title> tags:\n"
      "<title>"));

  api_->PerformRequest(
      model_options_, std::move(*messages), base::NullCallback(),
      base::BindOnce(&EngineConsumerOAIRemote::OnConversationTitleGenerated,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(completed_callback)),
      std::vector<std::string>{"</title>"});
}

void EngineConsumerOAIRemote::GenerateAssistantResponse(
    PageContentsMap&& page_contents,
    const ConversationHistory& conversation_history,
    bool is_temporary_chat,
    const std::vector<base::WeakPtr<Tool>>& tools,
    std::optional<std::string_view> preferred_tool_name,
    mojom::ConversationCapability conversation_capability,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback) {
  if (!CanPerformCompletionRequest(conversation_history)) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  bool exclude_memory =
      is_temporary_chat || HasCustomSystemPrompt(model_options_);
  auto conversation_messages =
      BuildOAIMessages(std::move(page_contents), conversation_history, prefs_,
                       exclude_memory, max_associated_content_length_,
                       [this](std::string& input) { SanitizeInput(input); });

  std::vector<OAIMessage> messages;
  messages.reserve(conversation_messages.size() + 1);
  messages.push_back(BuildSystemMessage(conversation_messages));
  std::ranges::move(conversation_messages, std::back_inserter(messages));

  api_->PerformRequest(model_options_, std::move(messages),
                       std::move(data_received_callback),
                       std::move(completed_callback));
}

OAIMessage EngineConsumerOAIRemote::BuildSystemMessage(
    const std::vector<OAIMessage>& conversation_messages) {
  OAIMessage system_message;
  system_message.role = "system";

  std::string system_text;
  std::string date_and_time_string =
      base::UTF16ToUTF8(TimeFormatFriendlyDateAndTime(base::Time::Now()));

  if (HasCustomSystemPrompt(model_options_)) {
    system_text = model_options_.model_system_prompt.value();
    base::ReplaceSubstringsAfterOffset(&system_text, 0, "%datetime%",
                                       date_and_time_string);
  } else {
    system_text = base::ReplaceStringPlaceholders(
        l10n_util::GetStringUTF8(
            IDS_AI_CHAT_DEFAULT_CUSTOM_MODEL_SYSTEM_PROMPT),
        {date_and_time_string}, nullptr);

    bool include_memory_prompt =
        std::ranges::any_of(conversation_messages, [](const auto& msg) {
          return std::ranges::any_of(msg.content, [](const auto& block) {
            return block->is_memory_content_block();
          });
        });
    if (include_memory_prompt) {
      base::StrAppend(
          &system_text,
          {l10n_util::GetStringUTF8(
              IDS_AI_CHAT_CUSTOM_MODEL_USER_MEMORY_SYSTEM_PROMPT_SEGMENT)});
    }
  }

  system_message.content.push_back(mojom::ContentBlock::NewTextContentBlock(
      mojom::TextContentBlock::New(std::move(system_text))));
  return system_message;
}

void EngineConsumerOAIRemote::SanitizeInput(std::string& input) {}

void EngineConsumerOAIRemote::DedupeTopics(
    base::expected<std::vector<std::string>, mojom::APIError> topics_result,
    GetSuggestedTopicsCallback callback) {
  if (!topics_result.has_value() || topics_result->empty()) {
    std::move(callback).Run(topics_result);
    return;
  }

  auto messages = BuildOAIDedupeTopicsMessages(*topics_result);

  api_->PerformRequest(
      model_options_, std::move(messages), base::NullCallback(),
      base::BindOnce(
          [](GetSuggestedTopicsCallback callback, GenerationResult result) {
            // Return deduped topics from the response.
            std::vector<GenerationResult> results;
            results.emplace_back(std::move(result));
            std::move(callback).Run(
                EngineConsumer::GetStrArrFromTabOrganizationResponses(results));
          },
          std::move(callback)));
}

void EngineConsumerOAIRemote::GetSuggestedTopics(
    const std::vector<Tab>& tabs,
    GetSuggestedTopicsCallback callback) {
  auto chunked_messages = BuildChunkedTabFocusMessages(tabs, "");
  if (chunked_messages.empty()) {
    std::move(callback).Run(base::unexpected(mojom::APIError::InternalError));
    return;
  }

  const auto barrier_callback = base::BarrierCallback<GenerationResult>(
      chunked_messages.size(),
      base::BindOnce(&EngineConsumerOAIRemote::MergeSuggestTopicsResults,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));

  for (auto& messages : chunked_messages) {
    api_->PerformRequest(model_options_, std::move(messages),
                         base::NullCallback(), barrier_callback);
  }
}

void EngineConsumerOAIRemote::GetFocusTabs(const std::vector<Tab>& tabs,
                                           const std::string& topic,
                                           GetFocusTabsCallback callback) {
  auto chunked_messages = BuildChunkedTabFocusMessages(tabs, topic);
  if (chunked_messages.empty()) {
    std::move(callback).Run(base::unexpected(mojom::APIError::InternalError));
    return;
  }

  const auto barrier_callback = base::BarrierCallback<GenerationResult>(
      chunked_messages.size(),
      base::BindOnce(
          [](GetFocusTabsCallback callback,
             std::vector<GenerationResult> results) {
            // Merge the results and call callback with tab IDs or error.
            std::move(callback).Run(
                EngineConsumer::GetStrArrFromTabOrganizationResponses(results));
          },
          std::move(callback)));

  for (auto& messages : chunked_messages) {
    api_->PerformRequest(model_options_, std::move(messages),
                         base::NullCallback(), barrier_callback);
  }
}

}  // namespace ai_chat
