// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/engine/engine_consumer_llama.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/common/features.h"
#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/engine/remote_completion_client.h"
#include "components/grit/brave_components_strings.h"

#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

using ai_chat::mojom::ConversationTurn;

constexpr char kLlama2Bos[] = "<s>";
constexpr char kLlama2Eos[] = "</s>";
constexpr char kLlama2BIns[] = "[INST]";
constexpr char kLlama2EIns[] = "[/INST]";
constexpr char kLlama2BSys[] = "<<SYS>>\n";
constexpr char kLlama2ESys[] = "\n<</SYS>>\n\n";

static constexpr auto kStopSequences =
    base::MakeFixedFlatSet<std::string_view>({kLlama2Eos});

std::string BuildLlama2InstructionPrompt(const std::string& instruction) {
  return base::ReplaceStringPlaceholders(
      R"($1 $2 $3 )", {kLlama2BIns, instruction, kLlama2EIns}, nullptr);
}

std::string BuildLlama2FirstSequence(
    const std::string& system_message,
    const std::string& user_message,
    absl::optional<std::string> assistant_response,
    absl::optional<std::string> assistant_response_seed) {
  // Generates a partial sequence if there is no assistant_response:

  // <s> [INST] <<SYS>>
  // You will be acting as an assistant named Leo created by the company Brave.
  // Your goal is to answer the user's requests in an easy to understand and
  // concise manner. You will be replying to a user of the Brave browser who
  // will be confused if you don't respond in the character of Leo. Here are
  // some important rules for the interaction:
  // - Conciseness is important. Your responses should not exceed 6 sentences.
  // - Always respond in a neutral tone.
  // - Always stay in character, as Leo, an AI from Brave.
  // <</SYS>>
  //
  // How's it going? [/INST]

  // Or, if there is an assistant_response:

  // <s> [INST] <<SYS>>
  // You will be acting as an assistant named Leo created by the company Brave.
  // Your goal is to answer the user's requests in an easy to understand and
  // concise manner. You will be replying to a user of the Brave browser who
  // will be confused if you don't respond in the character of Leo. Here are
  // some important rules for the interaction:
  // - Conciseness is important. Your responses should not exceed 6 sentences.
  // - Always respond in a neutral tone.
  // - Always stay in character, as Leo, an AI from Brave.
  // <</SYS>>
  //
  // How's it going? [/INST] Hey there! I'm Leo, your AI assistant here to help
  // you out. I'm here to answer any questions you have, so feel free to ask me
  // anything! What's up?</s>

  // Create the system prompt through the first user message.
  std::string system_prompt =
      base::StrCat({kLlama2BSys, system_message, kLlama2ESys, user_message});

  // Wrap in [INST] [/INST] tags.
  std::string instruction_prompt = BuildLlama2InstructionPrompt(system_prompt);

  if (!assistant_response) {
    // Prepend just <s> if there's no assistant_response ( it will be completed
    // by the model).
    if (assistant_response_seed) {
      return base::StrCat(
          {kLlama2Bos, instruction_prompt, *assistant_response_seed});
    }
    return base::StrCat({kLlama2Bos, instruction_prompt});
  }

  // Add assistant response and wrap in <s> </s> tags.
  return base::StrCat(
      {kLlama2Bos, instruction_prompt, *assistant_response, kLlama2Eos});
}

std::string BuildLlama2SubsequentSequence(
    std::string user_message,
    absl::optional<std::string> assistant_response,
    absl::optional<std::string> assistant_response_seed) {
  // Builds a prompt segment that looks like this:
  // <s> [INST] Give me the first few numbers in the fibonacci sequence [/INST]

  // or, if there's an assistant_response:

  // <s> [INST] Give me the first few numbers in the fibonacci sequence [/INST]
  // Hey there! Sure thing! The first few numbers in the Fibonacci sequence are:
  // 1, 1, 2, 3, 5, 8, 13, and so on. </s>

  user_message = BuildLlama2InstructionPrompt(user_message);

  if (assistant_response_seed) {
    return base::StrCat({kLlama2Bos, user_message, *assistant_response_seed});
  }

  if (!assistant_response) {
    return base::StrCat({kLlama2Bos, user_message});
  }

  return base::StrCat(
      {kLlama2Bos, user_message, *assistant_response, kLlama2Eos});
}

std::string BuildLlama2GenerateQuestionsPrompt(bool is_video,
                                               const std::string content) {
  std::string content_template;
  if (is_video) {
    content_template =
        l10n_util::GetStringUTF8(IDS_AI_CHAT_LLAMA2_GENERATE_QUESTIONS_VIDEO);
  } else {
    content_template =
        l10n_util::GetStringUTF8(IDS_AI_CHAT_LLAMA2_GENERATE_QUESTIONS_ARTICLE);
  }

  const std::string& user_message =
      base::ReplaceStringPlaceholders(content_template, {content}, nullptr);

  return BuildLlama2FirstSequence(
      l10n_util::GetStringUTF8(
          IDS_AI_CHAT_LLAMA2_SYSTEM_MESSAGE_GENERATE_QUESTIONS),
      user_message, absl::nullopt,
      l10n_util::GetStringUTF8(
          IDS_AI_CHAT_LLAMA2_SYSTEM_MESSAGE_GENERATE_QUESTIONS_RESPONSE_SEED));
}

std::string BuildLlama2Prompt(
    const std::vector<ConversationTurn>& conversation_history,
    std::string page_content,
    const bool& is_video,
    const std::string user_message) {
  // Always use a generic system message
  std::string system_message =
      l10n_util::GetStringUTF8(IDS_AI_CHAT_LLAMA2_SYSTEM_MESSAGE_GENERIC);
  std::string date_and_time_string =
      base::UTF16ToUTF8(TimeFormatFriendlyDateAndTime(base::Time::Now()));
  std::string today_system_message = base::ReplaceStringPlaceholders(
      system_message, {date_and_time_string}, nullptr);

  // Get the raw first user message, which is in the chat history if this is
  // the first sequence.
  std::string raw_first_user_message;
  if (conversation_history.size() > 0) {
    raw_first_user_message = conversation_history[0].text;
  } else {
    raw_first_user_message = user_message;
  }

  // Build first_user_message, the first complete message sent to the AI model,
  // which may or may not include injected contents such as article text.
  std::string first_user_message;
  if (!page_content.empty()) {
    std::string first_message_template;
    if (is_video) {
      first_message_template =
          l10n_util::GetStringUTF8(IDS_AI_CHAT_VIDEO_PROMPT_SEGMENT_LLAMA2);
    } else {
      first_message_template =
          l10n_util::GetStringUTF8(IDS_AI_CHAT_ARTICLE_PROMPT_SEGMENT_LLAMA2);
    }
    first_user_message = base::ReplaceStringPlaceholders(
        first_message_template, {page_content, raw_first_user_message},
        nullptr);
  } else {
    // If there's no article or video context, just use the raw first user
    // message.
    first_user_message = raw_first_user_message;
  }

  // If there's no conversation history, then we just send a (partial)
  // first sequence.
  if (conversation_history.empty() || conversation_history.size() <= 1) {
    return BuildLlama2FirstSequence(
        today_system_message, first_user_message, absl::nullopt,
        l10n_util::GetStringUTF8(IDS_AI_CHAT_LLAMA2_GENERAL_SEED));
  }

  // Use the first two messages to build the first sequence,
  // which includes the system prompt.
  std::string prompt =
      BuildLlama2FirstSequence(today_system_message, first_user_message,
                               conversation_history[1].text, absl::nullopt);

  // Loop through the rest of the history two at a time building subsequent
  // sequences.
  for (size_t i = 2; i + 1 < conversation_history.size(); i += 2) {
    const std::string& prev_user_message = conversation_history[i].text;
    const std::string& assistant_message = conversation_history[i + 1].text;
    prompt += BuildLlama2SubsequentSequence(prev_user_message,
                                            assistant_message, absl::nullopt);
  }

  // Build the final subsequent exchange using the current turn.
  prompt += BuildLlama2SubsequentSequence(
      user_message, absl::nullopt,
      l10n_util::GetStringUTF8(IDS_AI_CHAT_LLAMA2_GENERAL_SEED));

  // Trimming recommended by Meta
  // https://huggingface.co/meta-llama/Llama-2-13b-chat#intended-use
  prompt = base::TrimWhitespaceASCII(prompt, base::TRIM_ALL);
  return prompt;
}

}  // namespace

namespace ai_chat {

EngineConsumerLlamaRemote::EngineConsumerLlamaRemote(
    const mojom::Model& model,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  // Allow specific model name to be overriden by feature flag
  // TODO(petemill): verify premium status, or ensure server will verify even
  // when given a model name override via cli flag param.
  std::string model_name = ai_chat::features::kAIModelName.Get();
  if (model_name.empty()) {
    model_name = model.name;
  }
  DCHECK(!model_name.empty());
  base::flat_set<std::string_view> stop_sequences(kStopSequences.begin(),
                                                  kStopSequences.end());
  api_ = std::make_unique<RemoteCompletionClient>(model_name, stop_sequences,
                                                  url_loader_factory);
}

EngineConsumerLlamaRemote::~EngineConsumerLlamaRemote() = default;

void EngineConsumerLlamaRemote::ClearAllQueries() {
  api_->ClearAllQueries();
}

void EngineConsumerLlamaRemote::GenerateQuestionSuggestions(
    const bool& is_video,
    const std::string& page_content,
    SuggestedQuestionsCallback callback) {
  std::string prompt;
  std::vector<std::string> stop_sequences;
  prompt = BuildLlama2GenerateQuestionsPrompt(is_video, page_content);
  stop_sequences.push_back(kLlama2Eos);
  stop_sequences.push_back("</ul>");
  DCHECK(api_);
  api_->QueryPrompt(
      prompt, stop_sequences,
      base::BindOnce(
          &EngineConsumerLlamaRemote::OnGenerateQuestionSuggestionsResponse,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void EngineConsumerLlamaRemote::OnGenerateQuestionSuggestionsResponse(
    SuggestedQuestionsCallback callback,
    GenerationResult result) {
  if (!result.has_value() || result->empty()) {
    // Query resulted in error
    LOG(ERROR) << "Error getting question suggestions.";
    return;
  }

  // Success
  // Llama 2 results look something like this:
  // Can ChatGPT actually summarize a seven-hour video in under a minute?</li>
  // <li>What are the limitations of ChatGPT's browsing capabilities, and how
  // does it affect its ability to provide accurate information?</li> <li>Can
  // ChatGPT's tonewood research be applied to other areas of scientific
  // inquiry beyond guitar making?</li>  These questions capture interesting
  // aspects of the video, such as the AI's ability to summarize long content,
  // its limitations, and its potential applications beyond the specific use
  // case presented in the video.

  // Split out the questions using </li>
  std::vector<std::string> questions = base::SplitStringUsingSubstr(
      *result, "</li>", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  // Remove the last entry in questions if it doesn't contain an <li> tag
  // which means its not an actually a question
  if (questions.size() > 1) {
    if (questions.back().find("<li>") == std::string::npos) {
      questions.pop_back();
    }
  }

  // Remove leading <li> from each question
  for (auto& question : questions) {
    auto parts = base::SplitStringUsingSubstr(
        question, "<li>", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (!parts.empty()) {
      question = parts.back();  // If there's an <li>, parts will contain an
                                // empty string and then the question. We take
                                // the last element.
    }
  }

  std::move(callback).Run(std::move(questions));
}

void EngineConsumerLlamaRemote::GenerateAssistantResponse(
    const bool& is_video,
    const std::string& page_content,
    const ConversationHistory& conversation_history,
    const std::string& human_input,
    GenerationDataCallback data_received_callback,
    GenerationCompletedCallback completed_callback) {
  std::string prompt = BuildLlama2Prompt(conversation_history, page_content,
                                         is_video, human_input);
  DCHECK(api_);
  api_->QueryPrompt(prompt, {"</response>"}, std::move(completed_callback),
                    std::move(data_received_callback));
}

void EngineConsumerLlamaRemote::SanitizeInput(std::string& input) {
  base::ReplaceSubstringsAfterOffset(&input, 0, kLlama2Bos, "");
  base::ReplaceSubstringsAfterOffset(&input, 0, kLlama2Eos, "");
  base::ReplaceSubstringsAfterOffset(&input, 0, kLlama2BIns, "");
  base::ReplaceSubstringsAfterOffset(&input, 0, kLlama2EIns, "");
  base::ReplaceSubstringsAfterOffset(&input, 0, kLlama2BSys, "");
  base::ReplaceSubstringsAfterOffset(&input, 0, kLlama2ESys, "");
  // TODO(petemill): Case-sensitive?
  base::ReplaceSubstringsAfterOffset(&input, 0, "<SYS>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "<article>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "</article>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "<history>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "</history>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "<question>", "");
  base::ReplaceSubstringsAfterOffset(&input, 0, "</question>", "");
}

}  // namespace ai_chat
