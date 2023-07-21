/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/browser/ai_chat_tab_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/browser/constants.h"
#include "brave/components/ai_chat/browser/page_content_fetcher.h"
#include "brave/components/ai_chat/common/features.h"
#include "brave/components/ai_chat/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/common/pref_names.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/storage_partition.h"
#include "ui/base/l10n/l10n_util.h"

using ai_chat::mojom::CharacterType;
using ai_chat::mojom::ConversationTurn;
using ai_chat::mojom::ConversationTurnVisibility;

namespace ai_chat {

AIChatTabHelper::AIChatTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<AIChatTabHelper>(*web_contents),
      pref_service_(
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())) {
  DCHECK(pref_service_);
  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      prefs::kBraveChatHasSeenDisclaimer,
      base::BindRepeating(&AIChatTabHelper::OnUserOptedIn,
                          weak_ptr_factory_.GetWeakPtr()));
  pref_change_registrar_.Add(
      prefs::kBraveChatAutoGenerateQuestions,
      base::BindRepeating(
          &AIChatTabHelper::OnPermissionChangedAutoGenerateQuestions,
          weak_ptr_factory_.GetWeakPtr()));
  ai_chat_api_ =
      std::make_unique<AIChatAPI>(web_contents->GetBrowserContext()
                                      ->GetDefaultStoragePartition()
                                      ->GetURLLoaderFactoryForBrowserProcess());
  favicon::ContentFaviconDriver::FromWebContents(web_contents)
      ->AddObserver(this);
}

AIChatTabHelper::~AIChatTabHelper() = default;

const std::vector<ConversationTurn>& AIChatTabHelper::GetConversationHistory() {
  return chat_history_;
}

void AIChatTabHelper::OnConversationActiveChanged(bool is_conversation_active) {
  is_conversation_active_ = is_conversation_active;
  DVLOG(3) << "Conversation active changed: " << is_conversation_active;
  MaybeGeneratePageText();
  MaybeGenerateQuestions();
}

bool AIChatTabHelper::HasUserOptedIn() {
  return pref_service_->GetBoolean(ai_chat::prefs::kBraveChatHasSeenDisclaimer);
}

void AIChatTabHelper::OnUserOptedIn() {
  MaybeGeneratePageText();
}

void AIChatTabHelper::OnPermissionChangedAutoGenerateQuestions() {
  MaybeGenerateQuestions();
}

std::string AIChatTabHelper::GetConversationHistoryString() {
  std::vector<std::string> turn_strings;
  for (const ConversationTurn& turn : chat_history_) {
    turn_strings.push_back((turn.character_type == CharacterType::HUMAN
                                ? ai_chat::kHumanPromptPlaceholder
                                : ai_chat::kAIPromptPlaceholder) +
                           turn.text);
  }

  return base::JoinString(turn_strings, "");
}

void AIChatTabHelper::AddToConversationHistory(const ConversationTurn& turn) {
  chat_history_.push_back(turn);

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

void AIChatTabHelper::UpdateOrCreateLastAssistantEntry(
    std::string updated_text) {
  if (chat_history_.empty() ||
      chat_history_.back().character_type != CharacterType::ASSISTANT) {
    updated_text = base::TrimWhitespaceASCII(updated_text, base::TRIM_LEADING);
    AddToConversationHistory({CharacterType::ASSISTANT,
                              ConversationTurnVisibility::VISIBLE,
                              updated_text});
  } else {
    chat_history_.back().text = updated_text;
  }

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

void AIChatTabHelper::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void AIChatTabHelper::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void AIChatTabHelper::MaybeGeneratePageText() {
  // Make sure user is opted in since this may make a network request
  // for more page content (e.g. video transcript).
  // Perf: make sure we're not doing this when the feature
  // won't be used (e.g. not opted in or no active conversation).
  if (is_page_text_fetch_in_progress_ || !article_text_.empty() ||
      !HasUserOptedIn() || !is_conversation_active_ ||
      !web_contents()->IsDocumentOnLoadCompletedInPrimaryMainFrame()) {
    return;
  }

  auto* primary_rfh = web_contents()->GetPrimaryMainFrame();

  if (!primary_rfh) {
    VLOG(1) << "Summary request submitted for a WebContents without a "
               "primary main frame";
    return;
  }
  is_page_text_fetch_in_progress_ = true;
  FetchPageContent(
      web_contents(),
      base::BindOnce(&AIChatTabHelper::OnTabContentRetrieved,
                     weak_ptr_factory_.GetWeakPtr(), current_navigation_id_));
}

void AIChatTabHelper::MaybeGenerateQuestions() {
  // Automatically fetch questions related to page content, if allowed
  bool can_auto_fetch_questions =
      HasUserOptedIn() && is_conversation_active_ &&
      pref_service_->GetBoolean(
          ai_chat::prefs::kBraveChatAutoGenerateQuestions) &&
      !article_text_.empty() && (suggested_questions_.size() <= 1);
  if (can_auto_fetch_questions) {
    GenerateQuestions();
  }
}

void AIChatTabHelper::OnTabContentRetrieved(int64_t for_navigation_id,
                                            std::string contents_text,
                                            bool is_video) {
  if (for_navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }

  is_page_text_fetch_in_progress_ = false;
  if (contents_text.empty()) {
    VLOG(1) << __func__ << ": No data";
    return;
  }
  is_video_ = is_video;
  article_text_ = contents_text;

  OnPageHasContentChanged();

  // Prevent indirect prompt injections being sent to the AI model.
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, ai_chat::kHumanPrompt,
                                     "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, ai_chat::kAIPrompt, "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "<article>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "</article>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "<history>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "</history>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "<question>", "");
  base::ReplaceSubstringsAfterOffset(&contents_text, 0, "</question>", "");

  // Now that we have article text, we can suggest to summarize it
  DCHECK(suggested_questions_.empty())
      << "Expected suggested questions to be clear when there has been no"
      << " previous text content but there were " << suggested_questions_.size()
      << " suggested questions: "
      << base::JoinString(suggested_questions_, ", ");

  // Now that we have content, we can provide a summary on-demand. Add that to
  // suggested questions.
  // TODO(petemill): translation for this question
  suggested_questions_.emplace_back(is_video_ ? "Summarize this video"
                                              : "Summarize this page");
  OnSuggestedQuestionsChanged();
  MaybeGenerateQuestions();
}

void AIChatTabHelper::CleanUp() {
  chat_history_.clear();
  article_text_.clear();
  suggested_questions_.clear();
  is_page_text_fetch_in_progress_ = false;
  is_request_in_progress_ = false;
  has_generated_questions_ = false;
  OnSuggestedQuestionsChanged();

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
    obs.OnPageHasContent();
  }
}

std::vector<std::string> AIChatTabHelper::GetSuggestedQuestions(
    bool& can_generate,
    mojom::AutoGenerateQuestionsPref& auto_generate) {
  // Can we get suggested questions
  can_generate = !has_generated_questions_ && !article_text_.empty();
  // Are we allowed to auto-generate
  auto_generate = GetAutoGeneratePref();
  return suggested_questions_;
}

bool AIChatTabHelper::HasPageContent() {
  return !article_text_.empty();
}

void AIChatTabHelper::GenerateQuestions() {
  DVLOG(1) << __func__;
  // This function should not be presented in the UI if the user has not
  // opted-in yet.
  DCHECK(HasUserOptedIn());
  DCHECK(is_conversation_active_);
  // Can't operate if we don't have an article text
  if (article_text_.empty()) {
    return;
  }
  // Don't perform the operation more than once
  if (suggested_questions_.size() > 1u) {
    return;
  }

  has_generated_questions_ = true;
  OnSuggestedQuestionsChanged();

  std::string prompt;
  std::vector<std::string> stop_sequences;
  if (UsesLlama2PromptTemplate(ai_chat::features::kAIModelName.Get())) {
    prompt = BuildLlama2GenerateQuestionsPrompt(is_video_, article_text_);
    stop_sequences.push_back(ai_chat::kLlama2Eos);
  } else {
    prompt = base::StrCat(
        {GetHumanPromptSegment(),
         base::ReplaceStringPlaceholders(
             l10n_util::GetStringUTF8(is_video_
                                          ? IDS_AI_CHAT_VIDEO_PROMPT_SEGMENT
                                          : IDS_AI_CHAT_ARTICLE_PROMPT_SEGMENT),
             {article_text_}, nullptr),
         "\n\n", l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_PROMPT_SEGMENT),
         GetAssistantPromptSegment(), " <response>"});
    stop_sequences.push_back("</response>");
  }
  // Make API request for questions.
  // Do not call SetRequestInProgress, this progress
  // does not need to be shown to the UI.
  auto navigation_id_for_query = current_navigation_id_;
  ai_chat_api_->QueryPrompt(
      prompt, stop_sequences,
      base::BindOnce(&AIChatTabHelper::OnAPISuggestedQuestionsResponse,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(navigation_id_for_query)));
}

void AIChatTabHelper::OnAPISuggestedQuestionsResponse(
    int64_t for_navigation_id,
    api_request_helper::APIRequestResult result) {
  // We might have navigated away whilst this async operation is in
  // progress, so check if we're the same navigation.
  if (for_navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }
  auto success = result.Is2XXResponseCode();
  if (!success) {
    LOG(ERROR) << "Error getting question suggestions. Code: "
               << result.response_code();
    return;
  }
  // Validate
  if (!result.value_body().is_dict()) {
    DVLOG(1) << "Expected dictionary for question suggestion result"
             << " but got: " << result.value_body().DebugString();
    return;
  }
  const std::string* completion =
      result.value_body().GetDict().FindString("completion");
  if (!completion || completion->empty()) {
    DVLOG(1) << "Expected completion param for question suggestion"
             << " result but got: " << result.value_body().DebugString();
    return;
  }

  DVLOG(2) << "Received " << (success ? "success" : "failed")
           << " suggested questions response: " << completion;
  // Parse questions
  auto questions = base::SplitString(*completion, "|",
                                     base::WhitespaceHandling::TRIM_WHITESPACE,
                                     base::SplitResult::SPLIT_WANT_NONEMPTY);
  suggested_questions_.insert(suggested_questions_.end(), questions.begin(),
                              questions.end());
  // Notify observers
  OnSuggestedQuestionsChanged();
  DVLOG(2) << "Got questions:" << base::JoinString(suggested_questions_, "\n");
}

std::string AIChatTabHelper::BuildClaudePrompt(const std::string& question_part,
                                               bool is_suggested_question) {
  auto prompt_segment_article =
      article_text_.empty()
          ? ""
          : base::StrCat(
                {base::ReplaceStringPlaceholders(
                     l10n_util::GetStringUTF8(
                         is_video_ ? IDS_AI_CHAT_VIDEO_PROMPT_SEGMENT
                                   : IDS_AI_CHAT_ARTICLE_PROMPT_SEGMENT),
                     {article_text_}, nullptr),
                 "\n\n"});

  auto prompt_segment_history =
      (chat_history_.empty() || is_suggested_question)
          ? ""
          : base::ReplaceStringPlaceholders(
                l10n_util::GetStringUTF8(
                    IDS_AI_CHAT_ASSISTANT_HISTORY_PROMPT_SEGMENT),
                {GetConversationHistoryString()}, nullptr);

  std::string prompt = base::StrCat(
      {GetHumanPromptSegment(), prompt_segment_article,
       base::ReplaceStringPlaceholders(
           l10n_util::GetStringUTF8(IDS_AI_CHAT_ASSISTANT_PROMPT_SEGMENT),
           {prompt_segment_history, question_part}, nullptr),
       GetAssistantPromptSegment(), " <response>\n"});

  return prompt;
}

std::string AIChatTabHelper::BuildLlama2Prompt(
    const std::string& user_message) {
  std::string system_message;
  if (article_text_.empty()) {
    system_message =
        l10n_util::GetStringUTF8(IDS_AI_CHAT_LLAMA2_SYSTEM_MESSAGE_GENERIC);
  } else if (is_video_) {
    system_message = base::ReplaceStringPlaceholders(
        l10n_util::GetStringUTF8(IDS_AI_CHAT_LLAMA2_SYSTEM_MESSAGE_VIDEO),
        {article_text_}, nullptr);
  } else {
    system_message = base::ReplaceStringPlaceholders(
        l10n_util::GetStringUTF8(IDS_AI_CHAT_LLAMA2_SYSTEM_MESSAGE_ARTICLE),
        {article_text_}, nullptr);
  }

  auto conversation_history = GetConversationHistory();
  // If there's no conversation history, then we just send a (partial)
  // first sequence.
  if (conversation_history.size() == 0) {
    return BuildLlama2FirstSequence(system_message, user_message,
                                    absl::nullopt);
  }

  // Use the first two messages to build the first sequence,
  // which includes the system prompt.
  std::string prompt =
      BuildLlama2FirstSequence(system_message, conversation_history[0].text,
                               conversation_history[1].text);

  // Loop through the rest of the history two at a time building subsequent
  // sequences.
  for (size_t i = 2; i + 1 < conversation_history.size(); i += 2) {
    const std::string& prev_user_message = conversation_history[i].text;
    const std::string& assistant_message = conversation_history[i + 1].text;
    prompt +=
        BuildLlama2SubsequentSequence(prev_user_message, assistant_message);
  }

  // Build the final subsequent exchange using the current turn.
  prompt += BuildLlama2SubsequentSequence(user_message, absl::nullopt);

  // Trimming recommended by Meta
  // https://huggingface.co/meta-llama/Llama-2-13b-chat#intended-use
  prompt = base::TrimWhitespaceASCII(prompt, base::TRIM_ALL);
  return prompt;
}

std::string AIChatTabHelper::BuildLlama2InstructionPrompt(
    const std::string& instruction) {
  return base::ReplaceStringPlaceholders(
      R"( $1 $2 $3 )",
      {ai_chat::kLlama2BIns, instruction, ai_chat::kLlama2EIns}, nullptr);
}

std::string AIChatTabHelper::BuildLlama2GenerateQuestionsPrompt(
    bool is_video,
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
      user_message, absl::nullopt);
}

std::string AIChatTabHelper::BuildLlama2FirstSequence(
    const std::string& system_message,
    const std::string& user_message,
    absl::optional<std::string> assistant_response) {
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
      base::StrCat({ai_chat::kLlama2BSys, system_message, ai_chat::kLlama2ESys,
                    user_message});

  // Wrap in [INST] [/INST] tags.
  std::string instruction_prompt = BuildLlama2InstructionPrompt(system_prompt);

  if (!assistant_response) {
    // Prepend just <s> if there's no assistant_response ( it will be completed
    // by the model).
    return base::StrCat({ai_chat::kLlama2Bos, instruction_prompt});
  }

  // Add assistant response and wrap in <s> </s> tags.
  return base::StrCat({ai_chat::kLlama2Bos, instruction_prompt,
                       *assistant_response, ai_chat::kLlama2Eos});
}

std::string AIChatTabHelper::BuildLlama2SubsequentSequence(
    std::string user_message,
    absl::optional<std::string> assistant_response) {
  // Builds a prompt segment that looks like this:
  // <s> [INST] Give me the first few numbers in the fibonacci sequence [/INST]

  // or, if there's an assistant_response:

  // <s> [INST] Give me the first few numbers in the fibonacci sequence [/INST]
  // Hey there! Sure thing! The first few numbers in the Fibonacci sequence are:
  // 1, 1, 2, 3, 5, 8, 13, and so on. </s>

  user_message = BuildLlama2InstructionPrompt(user_message);
  if (!assistant_response) {
    return base::StrCat({ai_chat::kLlama2Bos, user_message});
  }

  return base::StrCat({ai_chat::kLlama2Bos, user_message, *assistant_response,
                       ai_chat::kLlama2Eos});
}

void AIChatTabHelper::MakeAPIRequestWithConversationHistoryUpdate(
    const ConversationTurn& turn) {
  // This function should not be presented in the UI if the user has not
  // opted-in yet.
  DCHECK(HasUserOptedIn());
  DCHECK(is_conversation_active_);

  DCHECK(turn.character_type == CharacterType::HUMAN);

  bool is_suggested_question = false;

  // If it's a suggested question, remove it
  auto found_question_iter =
      base::ranges::find(suggested_questions_, turn.text);
  if (found_question_iter != suggested_questions_.end()) {
    is_suggested_question = true;
    suggested_questions_.erase(found_question_iter);
    OnSuggestedQuestionsChanged();
  }

  // TODO(petemill): Tokenize the summary question so that we
  // don't have to do this weird substitution.
  std::string question_part;
  if (turn.text == "Summarize this video") {
    question_part =
        l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_VIDEO_BULLETS);
  } else {
    question_part = turn.text;
  }

  std::string prompt;
  if (UsesLlama2PromptTemplate(ai_chat::features::kAIModelName.Get())) {
    prompt = BuildLlama2Prompt(question_part);
  } else {
    prompt = BuildClaudePrompt(question_part, is_suggested_question);
  }

  if (turn.visibility != ConversationTurnVisibility::HIDDEN) {
    AddToConversationHistory(turn);
  }

  DCHECK(ai_chat_api_);
  auto data_received_callback = base::BindRepeating(
      &AIChatTabHelper::OnAPIStreamDataReceived, weak_ptr_factory_.GetWeakPtr(),
      current_navigation_id_);

  auto data_completed_callback =
      base::BindOnce(&AIChatTabHelper::OnAPIStreamDataComplete,
                     weak_ptr_factory_.GetWeakPtr(), current_navigation_id_);

  is_request_in_progress_ = true;
  ai_chat_api_->QueryPrompt(std::move(prompt), {"</response>"},
                            std::move(data_completed_callback),
                            std::move(data_received_callback));
}

bool AIChatTabHelper::IsRequestInProgress() {
  DCHECK(ai_chat_api_);

  return is_request_in_progress_;
}

void AIChatTabHelper::OnAPIStreamDataReceived(
    int64_t for_navigation_id,
    data_decoder::DataDecoder::ValueOrError result) {
  if (for_navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }
  if (!result.has_value() || !result->is_dict()) {
    return;
  }

  if (const std::string* completion =
          result->GetDict().FindString("completion")) {
    UpdateOrCreateLastAssistantEntry(*completion);

    // Trigger an observer update to refresh the UI.
    for (auto& obs : observers_) {
      obs.OnAPIRequestInProgress(IsRequestInProgress());
    }
  }
}

void AIChatTabHelper::OnAPIStreamDataComplete(
    int64_t for_navigation_id,
    api_request_helper::APIRequestResult result) {
  if (for_navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }
  const bool success = result.Is2XXResponseCode();
  if (success) {
    // We're checking for a value body in case for non-streaming API results.
    if (result.value_body().is_dict()) {
      if (const std::string* completion =
              result.value_body().GetDict().FindString("completion")) {
        // Trimming necessary for Llama 2 which prepends responses with a " ".
        const std::string& trimmed =
            std::string(base::TrimWhitespaceASCII(*completion, base::TRIM_ALL));
        AddToConversationHistory(
            ConversationTurn{CharacterType::ASSISTANT,
                             ConversationTurnVisibility::VISIBLE, trimmed});
      }
    }
  }

  if (!success) {
    // TODO(petemill): show error state separate from assistant message
    AddToConversationHistory(ConversationTurn{
        CharacterType::ASSISTANT, ConversationTurnVisibility::VISIBLE,
        l10n_util::GetStringUTF8(IDS_CHAT_UI_API_ERROR)});
  }

  is_request_in_progress_ = false;

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }
}

void AIChatTabHelper::OnSuggestedQuestionsChanged() {
  for (auto& obs : observers_) {
    obs.OnSuggestedQuestionsChanged(
        suggested_questions_, has_generated_questions_, GetAutoGeneratePref());
  }
}

void AIChatTabHelper::OnPageHasContentChanged() {
  for (auto& obs : observers_) {
    obs.OnPageHasContent();
  }
}

void AIChatTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  // Store current navigation ID of the main document
  // so that we can ignore async responses against any navigated-away-from
  // documents.
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  DVLOG(2) << __func__ << navigation_handle->GetNavigationId()
           << " url: " << navigation_handle->GetURL().spec();
  current_navigation_id_ = navigation_handle->GetNavigationId();
}

void AIChatTabHelper::OnFaviconUpdated(
    favicon::FaviconDriver* favicon_driver,
    NotificationIconType notification_icon_type,
    const GURL& icon_url,
    bool icon_url_changed,
    const gfx::Image& image) {
  for (Observer& obs : observers_) {
    obs.OnFaviconImageDataChanged();
  }
}

mojom::AutoGenerateQuestionsPref AIChatTabHelper::GetAutoGeneratePref() {
  mojom::AutoGenerateQuestionsPref pref =
      mojom::AutoGenerateQuestionsPref::Unset;

  const base::Value* auto_generate_value = pref_service_->GetUserPrefValue(
      ai_chat::prefs::kBraveChatAutoGenerateQuestions);

  if (auto_generate_value) {
    pref = (auto_generate_value->GetBool()
                ? mojom::AutoGenerateQuestionsPref::Enabled
                : mojom::AutoGenerateQuestionsPref::Disabled);
  }

  return pref;
}

void AIChatTabHelper::PrimaryPageChanged(content::Page& page) {
  CleanUp();
}

void AIChatTabHelper::DocumentOnLoadCompletedInPrimaryMainFrame() {
  // We might have content here, so check.
  // TODO(petemill): If there are other navigation events to also
  // check if content is available at, then start a queue and make
  // sure we don't have multiple async distills going on at the same time.
  MaybeGeneratePageText();
}

void AIChatTabHelper::WebContentsDestroyed() {
  CleanUp();
  favicon::ContentFaviconDriver::FromWebContents(web_contents())
      ->RemoveObserver(this);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(AIChatTabHelper);

}  // namespace ai_chat
