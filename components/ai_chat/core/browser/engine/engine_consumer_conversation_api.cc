// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api.h"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "base/barrier_callback.h"
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/numerics/clamped_math.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_set.h"
#include "third_party/re2/src/re2/re2.h"

namespace ai_chat {

namespace {

using ConversationEvent = ConversationAPIClient::ConversationEvent;
using ConversationEventType = ConversationAPIClient::ConversationEventType;
using ConversationEventRole = ConversationAPIClient::ConversationEventRole;

constexpr size_t kTabListChunkSize = 75;
constexpr char kArrayPattern[] = R"((\[.*?\]))";

}  // namespace

EngineConsumerConversationAPI::EngineConsumerConversationAPI(
    const mojom::LeoModelOptions& model_options,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager,
    ModelService* model_service)
    : EngineConsumer(model_service) {
  DCHECK(!model_options.name.empty());
  model_name_ = model_options.name;
  api_ = std::make_unique<ConversationAPIClient>(
      model_options.name, url_loader_factory, credential_manager,
      model_service);
  max_associated_content_length_ = model_options.max_associated_content_length;
}

EngineConsumerConversationAPI::~EngineConsumerConversationAPI() = default;

void EngineConsumerConversationAPI::ClearAllQueries() {
  api_->ClearAllQueries();
}

// static
base::expected<std::vector<std::string>, mojom::APIError>
EngineConsumerConversationAPI::GetStrArrFromTabOrganizationResponses(
    std::vector<EngineConsumer::GenerationResult>& results) {
  // Use RE2 to extract the array from the response, then use rust JSON::Reader
  // to safely parse the array.
  std::vector<std::string> str_arr;
  mojom::APIError error = mojom::APIError::None;
  for (auto& result : results) {
    // Fail the operation if server returns an error, such as rate limiting.
    // On the other hand, ignore the result which cannot be parsed as expected.
    if (!result.has_value()) {
      error = result.error();
      break;
    }

    // Skip empty results.
    if (!result->event || !result->event->is_completion_event() ||
        result->event->get_completion_event()->completion.empty()) {
      continue;
    }

    std::string strArr = "";
    if (!RE2::PartialMatch(result->event->get_completion_event()->completion,
                           kArrayPattern, &strArr)) {
      continue;
    }
    auto value = base::JSONReader::Read(strArr, base::JSON_PARSE_RFC);
    if (!value) {
      continue;
    }

    auto* list = value->GetIfList();
    if (!list) {
      continue;
    }

    for (const auto& item : *list) {
      auto* str = item.GetIfString();
      if (!str || str->empty()) {
        continue;
      }
      str_arr.push_back(*str);
    }
  }

  if (error != mojom::APIError::None) {
    return base::unexpected(error);
  }

  if (str_arr.empty()) {
    return base::unexpected(mojom::APIError::InternalError);
  }

  return str_arr;
}

void EngineConsumerConversationAPI::GenerateRewriteSuggestion(
    std::string text,
    const std::string& question,
    const std::string& selected_language,
    GenerationDataCallback received_callback,
    GenerationCompletedCallback completed_callback) {
  std::vector<ConversationEvent> conversation;
  conversation.emplace_back(ConversationEventRole::User,
                            ConversationEventType::UserText,
                            std::vector<std::string>{text});
  conversation.emplace_back(ConversationEventRole::User,
                            ConversationEventType::RequestRewrite,
                            /*Content=*/std::vector<std::string>{question});
  api_->PerformRequest(std::move(conversation), selected_language, std::nullopt,
                       std::nullopt, std::move(received_callback),
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
  conversation.emplace_back(ConversationEventRole::User,
                            ConversationEventType::RequestSuggestedActions,
                            std::vector<std::string>{""});

  auto on_response = base::BindOnce(
      &EngineConsumerConversationAPI::OnGenerateQuestionSuggestionsResponse,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  api_->PerformRequest(std::move(conversation), selected_language, std::nullopt,
                       std::nullopt, base::NullCallback(),
                       std::move(on_response));
}

void EngineConsumerConversationAPI::OnGenerateQuestionSuggestionsResponse(
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

  // Success
  std::vector<std::string> questions =
      base::SplitString(result->event->get_completion_event()->completion, "|",
                        base::WhitespaceHandling::TRIM_WHITESPACE,
                        base::SplitResult::SPLIT_WANT_NONEMPTY);
  std::move(callback).Run(std::move(questions));
}

void EngineConsumerConversationAPI::GenerateAssistantResponse(
    const bool& is_video,
    const std::string& page_content,
    const ConversationHistory& conversation_history,
    const std::string& selected_language,
    const std::vector<base::WeakPtr<Tool>>& tools,
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

  // We're going to iterate over the conversation entries and
  // build a list of events for the remote API.
  // We largely want to send the full conversation with all events back to the
  // model in order to preserve the context of the conversation. However, some
  // tool results are extremely large (especially for images), and repetitive.
  // We need a way to remove the noise in order to 1) not overwhelm the model
  // and 2) not surpass the max token limit. For now, this is a rudimentary
  // approach that only keeps the most recent large tool results. Use a two-pass
  // approach: first identify which large tool results to keep, then build the
  // conversation in chronological order.

  // Step 1: First pass - identify large tool results and remember which ones to
  // remove.
  absl::flat_hash_set<std::pair<size_t, size_t>> large_tool_result_remove_set;
  size_t large_tool_count = 0;

  for (size_t message_index = conversation_history.size(); message_index > 0;
       --message_index) {
    const auto& message = conversation_history[message_index - 1];
    if (message->character_type == mojom::CharacterType::ASSISTANT &&
        message->events.has_value() && !message->events->empty()) {
      for (size_t event_index = message->events->size(); event_index > 0;
           --event_index) {
        const auto& message_event = message->events.value()[event_index - 1];
        if (!message_event->is_tool_use_event()) {
          continue;
        }

        const auto& tool_event = message_event->get_tool_use_event();
        if (!tool_event->output.has_value() || tool_event->output->empty()) {
          continue;
        }

        // Check if this tool result is large
        bool is_large = false;
        size_t content_size = 0;
        for (const auto& content : tool_event->output.value()) {
          if (content->is_image_content_block()) {
            is_large = true;
            break;
          } else if (content->is_text_content_block()) {
            content_size += content->get_text_content_block()->text.size();
            if (content_size >= features::kContentSizeLargeToolUseEvent.Get()) {
              is_large = true;
              break;
            }
          }
        }

        if (is_large) {
          large_tool_count++;
          if (large_tool_count > features::kMaxCountLargeToolUseEvents.Get()) {
            large_tool_result_remove_set.insert(
                {message_index - 1, event_index - 1});
          }
        }
      }
    }
  }

  // Step 2: Main pass - build conversation in chronological order
  for (size_t message_index = 0; message_index < conversation_history.size();
       ++message_index) {
    const auto& message = conversation_history[message_index];

    // Events that come before the main message
    if (message->uploaded_files) {
      std::vector<std::string> uploaded_images;
      std::vector<std::string> screenshot_images;
      for (const auto& uploaded_file : message->uploaded_files.value()) {
        if (uploaded_file->type == mojom::UploadedFileType::kScreenshot) {
          screenshot_images.emplace_back(GetImageDataURL(uploaded_file->data));
        } else if (uploaded_file->type == mojom::UploadedFileType::kImage) {
          uploaded_images.emplace_back(GetImageDataURL(uploaded_file->data));
        }
      }
      if (!uploaded_images.empty()) {
        conversation.emplace_back(ConversationEventRole::User,
                                  ConversationEventType::UploadImage,
                                  std::move(uploaded_images));
      }
      if (!screenshot_images.empty()) {
        conversation.emplace_back(ConversationEventRole::User,
                                  ConversationEventType::PageScreenshot,
                                  std::move(screenshot_images));
      }
    }

    if (message->selected_text.has_value() &&
        !message->selected_text->empty()) {
      conversation.emplace_back(
          ConversationEventRole::User, ConversationEventType::PageExcerpt,
          std::vector<std::string>{message->selected_text.value()});
    }

    // Build the main conversation event
    ConversationEvent event;
    event.role = message->character_type == mojom::CharacterType::HUMAN
                     ? ConversationEventRole::User
                     : ConversationEventRole::Assistant;
    // TODO(petemill): Rebuild an event for most of `message->events` so that
    // we are sending the full context back to the API, including search
    // results, annotations, etc.
    event.content =
        std::vector<std::string>{EngineConsumer::GetPromptForEntry(message)};

    // Add tool calls to the main event
    if (message->character_type == mojom::CharacterType::ASSISTANT &&
        message->events.has_value() && !message->events->empty()) {
      for (size_t event_index = 0; event_index < message->events->size();
           ++event_index) {
        const auto& message_event = message->events.value()[event_index];
        if (!message_event->is_tool_use_event()) {
          continue;
        }

        const auto& tool_event = message_event->get_tool_use_event();
        if (tool_event->output.has_value()) {
          event.tool_calls.emplace_back(tool_event->Clone());
        }
      }
    }

    // TODO(petemill): Shouldn't the server handle the map of mojom::ActionType
    // to prompts in addition to SUMMARIZE_PAGE (e.g. PARAPHRASE, EXPLAIN,
    // IMPROVE, etc.)
    if (message->action_type == mojom::ActionType::SUMMARIZE_PAGE) {
      event.type = ConversationEventType::RequestSummary;
      event.content = std::vector<std::string>{""};
    } else {
      event.type = ConversationEventType::ChatMessage;
    }

    conversation.emplace_back(std::move(event));

    // Add tool results after the main message
    if (message->character_type == mojom::CharacterType::ASSISTANT &&
        message->events.has_value() && !message->events->empty()) {
      for (size_t event_index = 0; event_index < message->events->size();
           ++event_index) {
        const auto& message_event = message->events.value()[event_index];
        if (!message_event->is_tool_use_event()) {
          continue;
        }

        const auto& tool_event = message_event->get_tool_use_event();
        if (!tool_event->output.has_value()) {
          continue;
        }

        ConversationEvent tool_result;
        tool_result.role = ConversationEventRole::Tool;
        tool_result.type = ConversationEventType::ToolUse;
        tool_result.tool_call_id = tool_event->id;

        // Check if we should keep the full content for this large tool result
        bool should_keep_full_content = !large_tool_result_remove_set.contains(
            {message_index, event_index});

        if (should_keep_full_content) {
          std::vector<mojom::ContentBlockPtr> tool_result_content;
          for (const auto& item : tool_event->output.value()) {
            tool_result_content.push_back(item.Clone());
          }
          tool_result.content = std::move(tool_result_content);
        } else {
          tool_result.content = std::vector<std::string>{
              "[Large result removed to save space for subsequent results]"};
        }

        conversation.emplace_back(std::move(tool_result));
      }
    }
  }

  // Override model_name to be used if model_key existed, used when
  // regenerating answer.
  std::optional<std::string> model_name = std::nullopt;
  if (conversation_history.back()->model_key) {
    model_name = model_service_->GetLeoModelNameByKey(
        *conversation_history.back()->model_key);
  }
  api_->PerformRequest(std::move(conversation), selected_language,
                       ToolApiDefinitionsFromTools(tools), std::nullopt,
                       std::move(data_received_callback),
                       std::move(completed_callback), model_name);
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
  event.role = ConversationEventRole::User;
  event.content = std::vector<std::string>{truncated_page_content};
  // TODO(petemill): Differentiate video transcript / XML / VTT
  event.type = is_video ? ConversationEventType::VideoTranscript
                        : ConversationEventType::PageText;
  return event;
}

void EngineConsumerConversationAPI::DedupeTopics(
    base::expected<std::vector<std::string>, mojom::APIError> topics_result,
    GetSuggestedTopicsCallback callback) {
  if (!topics_result.has_value() || topics_result->empty()) {
    std::move(callback).Run(topics_result);
    return;
  }

  base::Value::List topic_list;
  for (const auto& topic : *topics_result) {
    topic_list.Append(topic);
  }
  std::vector<ConversationEvent> conversation;
  conversation.push_back(
      {ConversationEventRole::User, ConversationEventType::DedupeTopics,
       std::vector<std::string>{
           base::WriteJson(topic_list).value_or(std::string())}});
  api_->PerformRequest(
      std::move(conversation), "" /* selected_language */, std::nullopt,
      std::nullopt, base::NullCallback() /* data_received_callback */,
      base::BindOnce(
          [](GetSuggestedTopicsCallback callback,
             EngineConsumer::GenerationResult result) {
            // Return deduped topics from the response.
            std::vector<EngineConsumer::GenerationResult> results;
            results.emplace_back(std::move(result));
            std::move(callback).Run(
                EngineConsumerConversationAPI::
                    GetStrArrFromTabOrganizationResponses(results));
          },
          std::move(callback)));
}

void EngineConsumerConversationAPI::ProcessTabChunks(
    const std::vector<Tab>& tabs,
    ConversationEventType event_type,
    base::OnceCallback<void(std::vector<GenerationResult>)> merge_callback,
    const std::string& topic) {
  CHECK(event_type == ConversationEventType::GetSuggestedTopicsForFocusTabs ||
        event_type ==
            ConversationEventType::GetSuggestedAndDedupeTopicsForFocusTabs ||
        event_type == ConversationEventType::GetFocusTabsForTopic);

  // Split tab into chunks of 75
  size_t num_chunks = (tabs.size() + kTabListChunkSize - 1) / kTabListChunkSize;
  const auto barrier_callback = base::BarrierCallback<GenerationResult>(
      num_chunks, std::move(merge_callback));

  for (size_t chunk = 0; chunk < num_chunks; ++chunk) {
    base::Value::List tab_value_list;
    for (size_t i = chunk * kTabListChunkSize;
         i < std::min((chunk + 1) * kTabListChunkSize, tabs.size()); ++i) {
      tab_value_list.Append(base::Value::Dict()
                                .Set("id", tabs[i].id)
                                .Set("title", tabs[i].title)
                                .Set("url", tabs[i].origin.Serialize()));
    }

    std::vector<ConversationEvent> conversation;
    conversation.push_back(
        {ConversationEventRole::User, event_type,
         std::vector<std::string>{
             base::WriteJson(tab_value_list).value_or(std::string())},
         topic});

    api_->PerformRequest(std::move(conversation), "" /* selected_language */,
                         std::nullopt, std::nullopt,
                         base::NullCallback() /* data_received_callback */,
                         barrier_callback /* data_completed_callback */);
  }
}

void EngineConsumerConversationAPI::MergeSuggestTopicsResults(
    GetSuggestedTopicsCallback callback,
    std::vector<GenerationResult> results) {
  if (results.size() == 1) {
    // No need to dedupe topics if there is only one result.
    std::move(callback).Run(
        EngineConsumerConversationAPI::GetStrArrFromTabOrganizationResponses(
            results));
    return;
  }

  // Merge the result and send another request to dedupe topics.
  DedupeTopics(GetStrArrFromTabOrganizationResponses(results),
               std::move(callback));
}

void EngineConsumerConversationAPI::GetSuggestedTopics(
    const std::vector<Tab>& tabs,
    GetSuggestedTopicsCallback callback) {
  auto event_type =
      tabs.size() > kTabListChunkSize
          ? ConversationEventType::GetSuggestedTopicsForFocusTabs
          : ConversationEventType::GetSuggestedAndDedupeTopicsForFocusTabs;
  ProcessTabChunks(
      tabs, event_type,
      base::BindOnce(&EngineConsumerConversationAPI::MergeSuggestTopicsResults,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)),
      "" /* topic */);
}

void EngineConsumerConversationAPI::GetFocusTabs(
    const std::vector<Tab>& tabs,
    const std::string& topic,
    EngineConsumer::GetFocusTabsCallback callback) {
  ProcessTabChunks(
      tabs, ConversationEventType::GetFocusTabsForTopic,
      base::BindOnce(
          [](EngineConsumer::GetFocusTabsCallback callback,
             std::vector<GenerationResult> results) {
            // Merge the results and call callback with tab IDs or
            // error.
            std::move(callback).Run(
                EngineConsumerConversationAPI::
                    GetStrArrFromTabOrganizationResponses(results));
          },
          std::move(callback)),
      topic);
}

}  // namespace ai_chat
