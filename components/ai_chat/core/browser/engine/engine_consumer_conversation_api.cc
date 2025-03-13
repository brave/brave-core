// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api.h"

#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/numerics/clamped_math.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

namespace {

using ConversationEvent = ConversationAPIClient::ConversationEvent;
using ConversationEventType = ConversationAPIClient::ConversationEventType;

}  // namespace

EngineConsumerConversationAPI::EngineConsumerConversationAPI(
    const mojom::LeoModelOptions& model_options,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager) {
  DCHECK(!model_options.name.empty());
  api_ = std::make_unique<ConversationAPIClient>(
      model_options.name, url_loader_factory, credential_manager);
  max_associated_content_length_ = model_options.max_associated_content_length;
}

EngineConsumerConversationAPI::~EngineConsumerConversationAPI() = default;

void EngineConsumerConversationAPI::ClearAllQueries() {
  api_->ClearAllQueries();
}

void EngineConsumerConversationAPI::GenerateRewriteSuggestion(
    std::string text,
    const std::string& question,
    const std::string& selected_language,
    GenerationDataCallback received_callback,
    GenerationCompletedCallback completed_callback) {
  std::vector<ConversationEvent> conversation;
  conversation.emplace_back(ConversationEvent(mojom::CharacterType::HUMAN,
                              ConversationEventType::UserText, text));
  conversation.emplace_back(ConversationEvent(mojom::CharacterType::HUMAN,
                    ConversationEventType::RequestRewrite, question));
  api_->PerformRequest(std::move(conversation), {}, selected_language,
                       std::move(received_callback),
                       std::move(completed_callback));
}

void EngineConsumerConversationAPI::GenerateQuestionSuggestions(
    const bool& is_video,
    const std::string& page_content,
    const std::string& selected_language,
    SuggestedQuestionsCallback callback) {
  std::vector<ConversationEvent> conversation;
  conversation.emplace_back(
      GetAssociatedContentConversationEvent(page_content, is_video));
  conversation.emplace_back(
      ConversationEvent(mojom::CharacterType::HUMAN,
                        ConversationEventType::RequestSuggestedActions, ""));

  auto on_response = base::BindOnce(
      &EngineConsumerConversationAPI::OnGenerateQuestionSuggestionsResponse,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_->PerformRequest(std::move(conversation), {}, selected_language,
                       base::NullCallback(), std::move(on_response));
}

void EngineConsumerConversationAPI::OnGenerateQuestionSuggestionsResponse(
    SuggestedQuestionsCallback callback,
    GenerationResult result) {
  if (!result.has_value() || result->empty()) {
    // Query resulted in error
    std::move(callback).Run(base::unexpected(std::move(result.error())));
    return;
  }

  // Success
  std::vector<std::string> questions =
      base::SplitString(*result, "|", base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);
  std::move(callback).Run(std::move(questions));
}

void EngineConsumerConversationAPI::GenerateAssistantResponse(
    const bool& is_video,
    const std::string& page_content,
    const ConversationHistory& conversation_history,
    const std::string& selected_language,
    Tools tools,
    std::optional<std::string_view> preferred_tool_name,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback) {
  if (!CanPerformCompletionRequest(conversation_history)) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  std::vector<ConversationEvent> conversation;
  // associated content
  if (!page_content.empty()) {
    conversation.push_back(
        GetAssociatedContentConversationEvent(page_content, is_video));
  }

  // Whilst iterating in reverse, each earlier message should
  // be inserted at this position:
  auto conversation_message_insertion_it = conversation.end();

  // history
  // Keep count of large tool results to limit the number of large results,
  // (mainly screenshots), but keep the ones at the end by iterating in reverse.
  int large_event_count = 0;
  for (const auto& message : base::Reversed(conversation_history)) {
    std::vector<ConversationEvent> events_before_message;
    if (message->uploaded_images) {
      size_t counter = 0;
      for (const auto& uploaded_image : message->uploaded_images.value()) {
        // Only send the first uploaded_image becasue llama-vision seems to take
        // the last one if there are multiple images
        if (counter++ > 0) {
          break;
        }
        // Event should be inserted before any subsequent message
        events_before_message.push_back({mojom::CharacterType::HUMAN,
                                ConversationEventType::UploadImage,
                                GetImageDataURL(uploaded_image->image_data)});
      }
    }
    ConversationEvent event;
    event.role = message->character_type;

    event.content = EngineConsumer::GetPromptForEntry(message);

    std::vector<ConversationEvent> tool_results;

    // Construct tool calls and responses for each turn
    if (message->character_type == mojom::CharacterType::ASSISTANT &&
        message->events.has_value() && !message->events->empty()) {
      base::Value::List tool_calls;

      for (auto& message_event : base::Reversed(message->events.value())) {
        if (!message_event->is_tool_use_event()) {
          continue;
        }

        // Reconstruct tool call from assistant
        const auto& tool_event = message_event->get_tool_use_event();
        base::Value::Dict tool_call;
        tool_call.Set("id", tool_event->tool_id);
        tool_call.Set("type", "function");

        base::Value::Dict function;
        function.Set("name", tool_event->tool_name);
        function.Set("arguments", tool_event->input_json);
        tool_call.Set("function", std::move(function));

        tool_calls.Insert(tool_calls.begin(), base::Value(std::move(tool_call)));

        // Tool result
        if (tool_event->output_json.has_value()) {
          ConversationEvent tool_result;
          tool_result.role = mojom::CharacterType::TOOL;
          tool_result.type = ConversationEventType::ToolUse;
          tool_result.tool_call_id = tool_event->tool_id;

          // Only send the large results (e.g. images or xml trees) in the last
          // X tool result events, otherwise the context gets filled.
          if (!tool_event->output_json->empty() &&
              (large_event_count < 2 ||
               tool_event->output_json.value().size() < 1000)) {
            if (tool_event->output_json.value().size() >= 1000) {
              large_event_count++;
            }
            auto possible_content =
                base::JSONReader::Read(tool_event->output_json.value());
            if (!possible_content.has_value()) {
              DLOG(ERROR) << "Failed to parse tool output JSON: "
                         << tool_event->output_json.value();
              tool_result.content =  tool_event->output_json.value();
            } else if (possible_content->is_list()) {
              base::Value::List list =
                  possible_content.value().GetList().Clone();
              tool_result.content =  std::move(list);
            } else if (possible_content->is_dict()) {
              base::Value::List list;
              auto& content_dict = possible_content.value().GetDict();
              if (!content_dict.empty()) {
                list.Append(content_dict.Clone());
              }
              tool_result.content = std::move(list);
            } else {
              std::string& possible_string = possible_content.value().GetString();
              if (!possible_string.empty()) {
                tool_result.content =  possible_content.value().GetString();
              }
            }
          }
          tool_results.emplace_back(std::move(tool_result));
        }
      }
      if (!tool_calls.empty()) {
        event.tool_calls = std::move(tool_calls);
      }
    }

    // TODO(petemill): Shouldn't the server handle the map of mojom::ActionType
    // to prompts in addition to SUMMARIZE_PAGE (e.g. PARAPHRASE, EXPLAIN,
    // IMPROVE, etc.)
    if (message->action_type == mojom::ActionType::SUMMARIZE_PAGE) {
      event.type = ConversationEventType::RequestSummary;
      event.content = "";
    } else {
      event.type = ConversationEventType::ChatMessage;
    }

    // Tool result should be sorted after the message
    if (!tool_results.empty()) {
      // conversation_message_insertion_it = conversation.insert(conversation_message_insertion_it,
      //   std::make_move_iterator(tool_results.rbegin()),
      //   std::make_move_iterator(tool_results.rend()));
      for (auto& tool_event : tool_results) {
        conversation_message_insertion_it = conversation.insert(
            conversation_message_insertion_it, std::move(tool_event));
      }
    }

    conversation_message_insertion_it = conversation.insert(
        conversation_message_insertion_it, std::move(event));

    if (message->selected_text.has_value() &&
        !message->selected_text->empty()) {
      conversation_message_insertion_it = conversation.insert(
          conversation_message_insertion_it,
          ConversationEvent(mojom::CharacterType::HUMAN, ConversationEventType::PageExcerpt,
           message->selected_text.value()));
    }

    // Insert any events before the message
    if (!events_before_message.empty()) {
      conversation_message_insertion_it = conversation.insert(
          conversation_message_insertion_it,
          std::make_move_iterator(events_before_message.begin()),
          std::make_move_iterator(events_before_message.end()));
    }
  }

  api_->PerformRequest(std::move(conversation), tools, selected_language,
                       std::move(data_received_callback),
                       std::move(completed_callback));
}

void EngineConsumerConversationAPI::SanitizeInput(std::string& input) {
  // Sanitization is handled by the server
}

bool EngineConsumerConversationAPI::SupportsDeltaTextResponses() const {
  return true;
}

ConversationEvent
EngineConsumerConversationAPI::GetAssociatedContentConversationEvent(
    const std::string& content,
    const bool is_video) {
  const std::string& truncated_page_content =
      content.substr(0, max_associated_content_length_);

  ConversationEvent event;
  event.role = mojom::CharacterType::HUMAN;
  event.content = truncated_page_content;
  // TODO(petemill): Differentiate video transcript / XML / VTT
  event.type = is_video ? ConversationEventType::VideoTranscript
                        : ConversationEventType::PageText;
  return event;
}

}  // namespace ai_chat
