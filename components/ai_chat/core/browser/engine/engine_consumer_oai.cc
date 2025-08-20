// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/engine_consumer_oai.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/adapters.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/prefs.h"
#include "components/grit/brave_components_strings.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

namespace {
constexpr char kQuestionPrompt[] =
    "Propose up to 3 very short questions that a reader may ask about the "
    "content. Wrap each in <question> tags.";
}  // namespace

namespace ai_chat {

namespace {

using mojom::CharacterType;
using mojom::ConversationTurn;

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

void EngineConsumerOAIRemote::UpdateModelOptions(
    const mojom::ModelOptions& options) {
  if (options.is_custom_model_options()) {
    model_options_ = *options.get_custom_model_options();
    max_associated_content_length_ =
        model_options_.max_associated_content_length;
  }
}

void EngineConsumerOAIRemote::GenerateRewriteSuggestion(
    std::string text,
    const std::string& question,
    const std::string& selected_language,
    GenerationDataCallback received_callback,
    GenerationCompletedCallback completed_callback) {
  std::string truncated_text = text.substr(0, max_associated_content_length_);
  std::string rewrite_prompt = base::ReplaceStringPlaceholders(
      l10n_util::GetStringUTF8(
          IDS_AI_CHAT_LLAMA2_GENERATE_REWRITE_SUGGESTION_PROMPT),
      {std::move(truncated_text), question}, nullptr);

  base::Value::List messages;

  {
    base::Value::Dict message;
    message.Set("role", "user");
    message.Set("content", std::move(rewrite_prompt));
    messages.Append(std::move(message));
  }

  api_->PerformRequest(model_options_, std::move(messages),
                       std::move(received_callback),
                       std::move(completed_callback));
}

void EngineConsumerOAIRemote::GenerateQuestionSuggestions(
    PageContents page_contents,
    const std::string& selected_language,
    SuggestedQuestionsCallback callback) {
  base::Value::List messages;

  auto remaining_length = max_associated_content_length_;
  for (auto& message :
       BuildPageContentMessages(page_contents, remaining_length,
                                IDS_AI_CHAT_CLAUDE_VIDEO_PROMPT_SEGMENT,
                                IDS_AI_CHAT_CLAUDE_ARTICLE_PROMPT_SEGMENT)) {
    messages.Append(std::move(message));
  }

  {
    base::Value::Dict message;
    message.Set("role", "user");
    message.Set("content", kQuestionPrompt);
    messages.Append(std::move(message));
  }

  {
    base::Value::Dict message;
    message.Set("role", "assistant");
    message.Set("content",
                "Here are three questions the user may ask about the content "
                "in <question> tags:\n");
    messages.Append(std::move(message));
  }

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

void EngineConsumerOAIRemote::GenerateAssistantResponse(
    PageContentsMap&& page_contents,
    const ConversationHistory& conversation_history,
    const std::string& selected_language,
    bool is_temporary_chat,
    const std::vector<base::WeakPtr<Tool>>& tools,
    std::optional<std::string_view> preferred_tool_name,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback) {
  if (!CanPerformCompletionRequest(conversation_history)) {
    std::move(completed_callback).Run(base::unexpected(mojom::APIError::None));
    return;
  }

  const auto& last_turn = conversation_history.back();
  std::optional<std::string> selected_text = std::nullopt;
  if (last_turn->selected_text.has_value()) {
    selected_text =
        last_turn->selected_text->substr(0, max_associated_content_length_);
  }

  base::Value::List messages = BuildMessages(
      model_options_, page_contents, BuildUserMemoryMessage(is_temporary_chat),
      selected_text, conversation_history);
  api_->PerformRequest(model_options_, std::move(messages),
                       std::move(data_received_callback),
                       std::move(completed_callback));
}

void EngineConsumerOAIRemote::SanitizeInput(std::string& input) {}

void EngineConsumerOAIRemote::GetSuggestedTopics(
    const std::vector<Tab>& tabs,
    GetSuggestedTopicsCallback callback) {
  std::move(callback).Run(base::unexpected(mojom::APIError::InternalError));
}

void EngineConsumerOAIRemote::GetFocusTabs(const std::vector<Tab>& tabs,
                                           const std::string& topic,
                                           GetFocusTabsCallback callback) {
  std::move(callback).Run(base::unexpected(mojom::APIError::InternalError));
}

base::Value::List EngineConsumerOAIRemote::BuildPageContentMessages(
    PageContents& page_contents,
    uint32_t& max_associated_content_length,
    int video_message_id,
    int page_message_id) {
  base::Value::List messages;
  for (const auto& page_content : base::Reversed(page_contents)) {
    std::string truncated_page_content =
        page_content.get().content.substr(0, max_associated_content_length);
    uint32_t truncated_page_content_size = truncated_page_content.size();

    SanitizeInput(truncated_page_content);
    std::string prompt = base::ReplaceStringPlaceholders(
        l10n_util::GetStringUTF8(page_content.get().is_video ? video_message_id
                                                             : page_message_id),
        {std::move(truncated_page_content)}, nullptr);

    base::Value::Dict message;
    message.Set("role", "user");
    message.Set("content", std::move(prompt));
    messages.Append(std::move(message));

    if (truncated_page_content_size >= max_associated_content_length) {
      max_associated_content_length = 0;
      break;
    } else {
      max_associated_content_length -= truncated_page_content_size;
    }
  }
  return messages;
}

base::Value::List EngineConsumerOAIRemote::BuildMessages(
    const mojom::CustomModelOptions& model_options,
    PageContentsMap& page_contents,
    std::optional<base::Value::Dict> user_memory_message,
    const std::optional<std::string>& selected_text,
    const EngineConsumer::ConversationHistory& conversation_history) {
  uint32_t remaining_content_length = max_associated_content_length_;
  uint32_t selected_text_length = selected_text.value_or("").size();
  if (selected_text_length > max_associated_content_length_) {
    remaining_content_length = 0;
  } else {
    remaining_content_length -= selected_text_length;
  }

  base::flat_map<std::string, base::Value::List> page_contents_messages;

  // We iterate over the page contents in reverse order so that the most recent
  // content is preferred.
  for (const auto& turn : base::Reversed(conversation_history)) {
    // If we have no remaining content length, no point in continuing.
    if (remaining_content_length == 0) {
      break;
    }

    auto page_content_it = page_contents.find(turn->uuid.value());
    if (page_content_it != page_contents.end()) {
      auto& messages = page_contents_messages[turn->uuid.value()];
      for (auto& message : BuildPageContentMessages(
               page_content_it->second, remaining_content_length,
               IDS_AI_CHAT_LLAMA2_VIDEO_PROMPT_SEGMENT,
               IDS_AI_CHAT_LLAMA2_ARTICLE_PROMPT_SEGMENT)) {
        messages.Append(std::move(message));
      }
    }
  }

  base::Value::List messages;

  // Append system message
  {
    bool has_custom_system_prompt = model_options.model_system_prompt &&
                                    !model_options.model_system_prompt->empty();

    std::string system_message;
    std::string date_and_time_string =
        base::UTF16ToUTF8(TimeFormatFriendlyDateAndTime(base::Time::Now()));

    if (has_custom_system_prompt) {
      system_message = model_options.model_system_prompt.value();
      // Let the user optionally specify the datetime placeholder
      base::ReplaceSubstringsAfterOffset(&system_message, 0, "%datetime%",
                                         date_and_time_string);
    } else {
      system_message = base::ReplaceStringPlaceholders(
          l10n_util::GetStringUTF8(
              IDS_AI_CHAT_DEFAULT_CUSTOM_MODEL_SYSTEM_PROMPT),
          {date_and_time_string}, nullptr);
      if (user_memory_message) {
        base::StrAppend(
            &system_message,
            {l10n_util::GetStringUTF8(
                IDS_AI_CHAT_CUSTOM_MODEL_USER_MEMORY_SYSTEM_PROMPT_SEGMENT)});
      }
    }

    base::Value::Dict message;
    message.Set("role", "system");
    message.Set("content", system_message);
    messages.Append(std::move(message));

    if (user_memory_message && !has_custom_system_prompt) {
      messages.Append(std::move(*user_memory_message));
    }
  }

  for (const mojom::ConversationTurnPtr& turn : conversation_history) {
    // If we have page content for this turn, append it to the messages.
    auto page_content_it = page_contents_messages.find(turn->uuid.value());
    if (page_content_it != page_contents_messages.end()) {
      for (auto& message : page_content_it->second) {
        messages.Append(std::move(message));
      }
    }

    if (turn->uploaded_files) {
      base::Value::List content_uploaded_images;
      base::Value::List content_screenshots;
      base::Value::List content_uploaded_pdfs;

      content_uploaded_images.Append(
          base::Value::Dict()
              .Set("type", "text")
              .Set("text", "These images are uploaded by the user"));
      content_screenshots.Append(
          base::Value::Dict()
              .Set("type", "text")
              .Set("text", "These images are screenshots"));
      content_uploaded_pdfs.Append(
          base::Value::Dict()
              .Set("type", "text")
              .Set("text", "These PDFs are uploaded by the user"));
      for (const auto& uploaded_file : turn->uploaded_files.value()) {
        if (uploaded_file->type == mojom::UploadedFileType::kImage ||
            uploaded_file->type == mojom::UploadedFileType::kScreenshot) {
          base::Value::Dict image;
          image.Set("type", "image_url");
          base::Value::Dict image_url_dict;
          image_url_dict.Set(
              "url", EngineConsumer::GetImageDataURL(uploaded_file->data));
          image.Set("image_url", std::move(image_url_dict));
          if (uploaded_file->type == mojom::UploadedFileType::kImage) {
            content_uploaded_images.Append(std::move(image));
          } else {
            content_screenshots.Append(std::move(image));
          }
        } else if (uploaded_file->type == mojom::UploadedFileType::kPdf) {
          base::Value::Dict pdf_file;
          pdf_file.Set("type", "file");
          base::Value::Dict file_dict;
          file_dict.Set("filename", uploaded_file->filename.empty()
                                        ? "uploaded.pdf"
                                        : uploaded_file->filename);
          file_dict.Set("file_data",
                        EngineConsumer::GetPdfDataURL(uploaded_file->data));
          pdf_file.Set("file", std::move(file_dict));
          content_uploaded_pdfs.Append(std::move(pdf_file));
        }
      }
      if (content_uploaded_images.size() > 1) {
        messages.Append(
            base::Value::Dict()
                .Set("role", "user")
                .Set("content", std::move(content_uploaded_images)));
      }
      if (content_screenshots.size() > 1) {
        messages.Append(base::Value::Dict()
                            .Set("role", "user")
                            .Set("content", std::move(content_screenshots)));
      }
      if (content_uploaded_pdfs.size() > 1) {
        messages.Append(base::Value::Dict()
                            .Set("role", "user")
                            .Set("content", std::move(content_uploaded_pdfs)));
      }
    }
    base::Value::Dict message;
    message.Set("role", turn->character_type == CharacterType::HUMAN
                            ? "user"
                            : "assistant");

    message.Set(
        "content",
        turn->selected_text
            ? base::StrCat(
                  {base::ReplaceStringPlaceholders(
                       l10n_util::GetStringUTF8(
                           IDS_AI_CHAT_LLAMA2_SELECTED_TEXT_PROMPT_SEGMENT),
                       {*turn->selected_text}, nullptr),
                   "\n\n", EngineConsumer::GetPromptForEntry(turn)})
            : EngineConsumer::GetPromptForEntry(turn));
    messages.Append(std::move(message));
  }

  return messages;
}

std::optional<base::Value::Dict>
EngineConsumerOAIRemote::BuildUserMemoryMessage(bool is_temporary_chat) {
  if (is_temporary_chat) {
    return std::nullopt;
  }
  auto memories = prefs::GetUserMemoryDictFromPrefs(*prefs_);
  if (!memories) {
    return std::nullopt;
  }

  // HTML-escape individual string values to avoid breaking HTML-style tags
  // in our prompts.
  base::Value::Dict escaped_memories;
  for (const auto [key, value] : *memories) {
    if (value.is_string()) {
      escaped_memories.Set(key, base::EscapeForHTML(value.GetString()));
    } else if (value.is_list()) {
      base::Value::List escaped_list;
      for (const auto& item : value.GetList()) {
        if (item.is_string()) {
          escaped_list.Append(base::EscapeForHTML(item.GetString()));
        }
      }
      escaped_memories.Set(key, std::move(escaped_list));
    }
  }

  auto memories_json = base::WriteJson(escaped_memories);
  if (!memories_json) {
    return std::nullopt;
  }

  std::string prompt = base::ReplaceStringPlaceholders(
      l10n_util::GetStringUTF8(
          IDS_AI_CHAT_CUSTOM_MODEL_USER_MEMORY_PROMPT_SEGMENT),
      {*memories_json}, nullptr);

  base::Value::Dict message;
  message.Set("role", "user");
  message.Set("content", prompt);
  return message;
}

}  // namespace ai_chat
