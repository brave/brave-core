/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/conversation_driver.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_claude.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_llama.h"
#include "brave/components/ai_chat/core/browser/models.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "ui/base/l10n/l10n_util.h"

using ai_chat::mojom::CharacterType;
using ai_chat::mojom::ConversationTurn;
using ai_chat::mojom::ConversationTurnVisibility;

namespace {
static const auto kAllowedSchemes = base::MakeFixedFlatSet<std::string_view>(
    {url::kHttpsScheme, url::kHttpScheme, url::kFileScheme, url::kDataScheme});
}  // namespace

namespace ai_chat {

ConversationDriver::ConversationDriver(
    PrefService* profile_prefs,
    PrefService* local_state_prefs,
    AIChatMetrics* ai_chat_metrics,
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::string_view channel_string)
    : pref_service_(profile_prefs),
      ai_chat_metrics_(ai_chat_metrics),
      credential_manager_(std::make_unique<ai_chat::AIChatCredentialManager>(
          skus_service_getter,
          local_state_prefs)),
      feedback_api_(std::make_unique<ai_chat::AIChatFeedbackAPI>(
          url_loader_factory,
          std::string(channel_string))),
      url_loader_factory_(url_loader_factory) {
  DCHECK(pref_service_);

  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      prefs::kLastAcceptedDisclaimer,
      base::BindRepeating(&ConversationDriver::OnUserOptedIn,
                          weak_ptr_factory_.GetWeakPtr()));

  // Engines and model names are selectable per conversation, not static.
  // Start with default from pref value but only if user set. We can't rely on
  // actual default pref value since we should vary if user is premium or not.
  // TODO(petemill): When we have an event for premium status changed, and a
  // profile service for AIChat, then we can call
  // |pref_service_->SetDefaultPrefValue| when the user becomes premium. With
  // that, we'll be able to simply call GetString(prefs::kDefaultModelKey) and
  // not vary on premium status.
  if (!pref_service_->GetUserPrefValue(prefs::kDefaultModelKey)) {
    credential_manager_->GetPremiumStatus(base::BindOnce(
        [](ConversationDriver* instance, mojom::PremiumStatus status,
           mojom::PremiumInfoPtr) {
          instance->last_premium_status_ = status;
          if (status == mojom::PremiumStatus::Inactive) {
            // Not premium
            return;
          }
          // Use default premium model for this instance
          instance->ChangeModel(features::kAIModelsPremiumDefaultKey.Get());
          // Make sure default model reflects premium status
          const auto* current_default =
              instance->pref_service_
                  ->GetDefaultPrefValue(prefs::kDefaultModelKey)
                  ->GetIfString();

          if (current_default &&
              *current_default != features::kAIModelsPremiumDefaultKey.Get()) {
            instance->pref_service_->SetDefaultPrefValue(
                prefs::kDefaultModelKey,
                    base::Value(features::kAIModelsPremiumDefaultKey.Get()));
          }
        },
        // Unretained is ok as credential manager is owned by this class,
        // and it owns the mojo binding that is used to make async call in
        // |GetPremiumStatus|.
        base::Unretained(this)));
  }
  // Most calls to credential_manager_->GetPremiumStatus will call the callback
  // synchronously - when the user is premium and does not have expired
  // credentials. We avoid double-constructing engine_ in those cases
  // by checking here if the callback has already fired. In the case where the
  // callback will be called asynchronously, we need to initialize a model now.
  // Worst-case is that this will get double initialized for premium users
  // once whenever all credentials are expired.
  if (model_key_.empty()) {
    model_key_ = pref_service_->GetString(prefs::kDefaultModelKey);
  }
  InitEngine();
  DCHECK(engine_);

  if (ai_chat_metrics_ != nullptr) {
    ai_chat_metrics_->RecordEnabled(
        HasUserOptedIn(), false,
        base::BindOnce(&ConversationDriver::GetPremiumStatus,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

ConversationDriver::~ConversationDriver() = default;

void ConversationDriver::ChangeModel(const std::string& model_key) {
  DCHECK(!model_key.empty());
  // Check that the key exists
  if (kAllModels.find(model_key) == kAllModels.end()) {
    NOTREACHED() << "No matching model found for key: " << model_key;
    return;
  }
  model_key_ = model_key;
  InitEngine();
}

const mojom::Model& ConversationDriver::GetCurrentModel() {
  return kAllModels.find(model_key_)->second;
}

std::vector<mojom::ModelPtr> ConversationDriver::GetModels() {
  std::vector<mojom::ModelPtr> models(kAllModelKeysDisplayOrder.size());
  // Ensure we return only in intended display order
  std::transform(kAllModelKeysDisplayOrder.cbegin(),
                 kAllModelKeysDisplayOrder.cend(), models.begin(),
                 [](auto& model_key) {
                   auto model_match = kAllModels.find(model_key);
                   DCHECK(model_match != kAllModels.end());
                   return model_match->second.Clone();
                 });
  return models;
}

const std::vector<ConversationTurn>& ConversationDriver::GetConversationHistory() {
  return chat_history_;
}

std::vector<mojom::ConversationTurnPtr>
ConversationDriver::GetVisibleConversationHistory() {
  // Remove conversations that are meant to be hidden from the user
  std::vector<ai_chat::mojom::ConversationTurnPtr> list;
  for (const auto& turn : GetConversationHistory()) {
    if (turn.visibility != ConversationTurnVisibility::HIDDEN) {
      list.push_back(turn.Clone());
    }
  }
  return list;
}

void ConversationDriver::OnConversationActiveChanged(bool is_conversation_active) {
  is_conversation_active_ = is_conversation_active;
  DVLOG(3) << "Conversation active changed: " << is_conversation_active;
  if (MaybePopPendingRequests()) {
    return;
  }
  MaybeGeneratePageText();
}

void ConversationDriver::InitEngine() {
  DCHECK(!model_key_.empty());
  auto model_match = kAllModels.find(model_key_);
  // Make sure we get a valid model, defaulting to static default or first.
  if (model_match == kAllModels.end()) {
    NOTREACHED() << "Model was not part of static model list";
    // Use default
    model_match = kAllModels.find(features::kAIModelsDefaultKey.Get());
    const auto is_found = model_match != kAllModels.end();
    DCHECK(is_found);
    if (!is_found) {
      model_match = kAllModels.begin();
    }
  }

  auto model = model_match->second;
  // Model's key might not be the same as what we asked for (e.g. if the model
  // no longer exists).
  model_key_ = model.key;

  // Engine enum on model to decide which one
  if (model.engine_type == mojom::ModelEngineType::LLAMA_REMOTE) {
    VLOG(1) << "Started AI engine: llama";
    engine_ = std::make_unique<EngineConsumerLlamaRemote>(
        model, url_loader_factory_, credential_manager_.get());
  } else {
    VLOG(1) << "Started AI engine: claude";
    engine_ = std::make_unique<EngineConsumerClaudeRemote>(
        model, url_loader_factory_, credential_manager_.get());
  }

  // Pending requests have been deleted along with the model engine
  is_request_in_progress_ = false;
  for (auto& obs : observers_) {
    obs.OnModelChanged(model_key_);
    obs.OnAPIRequestInProgress(false);
  }

  // When the model changes, the content truncation might be different,
  // and the UI needs to know.
  if (!article_text_.empty()) {
    OnPageHasContentChanged(BuildSiteInfo());
  }
}

bool ConversationDriver::HasUserOptedIn() {
  base::Time last_accepted_disclaimer =
        pref_service_->GetTime(ai_chat::prefs::kLastAcceptedDisclaimer);
  return !last_accepted_disclaimer.is_null();
}

void ConversationDriver::SetUserOptedIn(bool user_opted_in) {
  if (user_opted_in) {
    pref_service_->SetTime(ai_chat::prefs::kLastAcceptedDisclaimer,
                           base::Time::Now());
  } else {
    pref_service_->ClearPref(ai_chat::prefs::kLastAcceptedDisclaimer);
  }
}

void ConversationDriver::OnUserOptedIn() {
  if (!MaybePopPendingRequests()) {
    MaybeGeneratePageText();
  }

  if (ai_chat_metrics_ != nullptr && HasUserOptedIn()) {
    ai_chat_metrics_->RecordEnabled(true, true, {});
  }
}

void ConversationDriver::AddToConversationHistory(mojom::ConversationTurn turn) {
  chat_history_.push_back(std::move(turn));

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }

  if (ai_chat_metrics_ != nullptr) {
    if (chat_history_.size() == 1) {
      ai_chat_metrics_->RecordNewChat();
    }

    if (turn.character_type == CharacterType::HUMAN) {
      ai_chat_metrics_->RecordNewPrompt();
    }
  }
}

void ConversationDriver::UpdateOrCreateLastAssistantEntry(std::string updated_text) {
  updated_text = base::TrimWhitespaceASCII(updated_text, base::TRIM_LEADING);
  if (chat_history_.empty() ||
      chat_history_.back().character_type != CharacterType::ASSISTANT) {
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

void ConversationDriver::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ConversationDriver::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void ConversationDriver::OnFaviconImageDataChanged() {
  for (Observer& obs : observers_) {
    obs.OnFaviconImageDataChanged();
  }
}

bool ConversationDriver::MaybePopPendingRequests() {
  if (!is_conversation_active_ || !HasUserOptedIn()) {
    return false;
  }

  if (!pending_conversation_entry_) {
    return false;
  }

  // We don't discard requests related to summarization until we have the
  // article text.
  if (article_text_.empty() && pending_message_needs_page_content_) {
    return false;
  }

  mojom::ConversationTurn request = std::move(*pending_conversation_entry_);
  pending_conversation_entry_.reset();
  pending_message_needs_page_content_ = false;
  MakeAPIRequestWithConversationHistoryUpdate(std::move(request));
  return true;
}

void ConversationDriver::MaybeGeneratePageText() {
  const GURL url = GetPageURL();

  if (!base::Contains(kAllowedSchemes, url.scheme())) {
    return;
  }

  // User might have already asked questions before the page is loaded. It'd be
  // strange if we generate contents based on the page.
  // TODO(sko) This makes it impossible to ask something like "Summarize this
  // page" once a user already asked a question. But for now we'd like to keep
  // it simple and not confuse users with the context changing. We'll see what
  // users say.
  if (!chat_history_.empty()) {
    return;
  }

  // Make sure user is opted in since this may make a network request
  // for more page content (e.g. video transcript).
  // Perf: make sure we're not doing this when the feature
  // won't be used (e.g. not opted in or no active conversation).
  if (is_page_text_fetch_in_progress_ || !article_text_.empty() ||
      !HasUserOptedIn() || !is_conversation_active_ ||
      !IsDocumentOnLoadCompletedInPrimaryMainFrame()) {
    return;
  }

  if (!HasPrimaryMainFrame()) {
    VLOG(1) << "Summary request submitted for a WebContents without a "
               "primary main frame";
    return;
  }

  if (should_send_page_contents_) {
    is_page_text_fetch_in_progress_ = true;
    // Update fetching status
    OnPageHasContentChanged(BuildSiteInfo());
    GetPageContent(
        base::BindOnce(&ConversationDriver::OnPageContentRetrieved,
                       weak_ptr_factory_.GetWeakPtr(), current_navigation_id_));
  }
}

void ConversationDriver::OnPageContentRetrieved(int64_t navigation_id,
                                          std::string contents_text,
                                          bool is_video) {
  if (navigation_id != current_navigation_id_) {
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
  engine_->SanitizeInput(article_text_);

  // Update completion status
  OnPageHasContentChanged(BuildSiteInfo());

  // Now that we have article text, we can suggest to summarize it
  DCHECK(suggestions_.empty())
      << "Expected suggested questions to be clear when there has been no"
      << " previous text content but there were " << suggestions_.size()
      << " suggested questions: " << base::JoinString(suggestions_, ", ");

  // Now that we have content, we can provide a summary on-demand. Add that to
  // suggested questions.
  suggestions_.emplace_back(
      is_video_ ? l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_VIDEO)
                : l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE));
  suggestion_generation_status_ =
      mojom::SuggestionGenerationStatus::CanGenerate;
  OnSuggestedQuestionsChanged();
  // We check again to see if any page content related prompt is pending
  MaybePopPendingRequests();
}

void ConversationDriver::CleanUp() {
  chat_history_.clear();
  article_text_.clear();
  suggestions_.clear();
  pending_conversation_entry_.reset();
  pending_message_needs_page_content_ = false;
  is_same_document_navigation_ = false;
  is_page_text_fetch_in_progress_ = false;
  is_request_in_progress_ = false;
  suggestion_generation_status_ = mojom::SuggestionGenerationStatus::None;
  should_send_page_contents_ = true;
  OnSuggestedQuestionsChanged();
  SetAPIError(mojom::APIError::None);
  engine_->ClearAllQueries();

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
    obs.OnAPIRequestInProgress(false);
    obs.OnPageHasContent(BuildSiteInfo());
  }
}

int64_t ConversationDriver::GetNavigationId() const {
  return current_navigation_id_;
}

void ConversationDriver::SetNavigationId(int64_t navigation_id) {
  current_navigation_id_ = navigation_id;
}

bool ConversationDriver::IsSameDocumentNavigation() const {
  return is_same_document_navigation_;
}

void ConversationDriver::SetSameDocumentNavigation(bool same_document_navigation) {
  is_same_document_navigation_ = same_document_navigation;
}

std::vector<std::string> ConversationDriver::GetSuggestedQuestions(
    mojom::SuggestionGenerationStatus& suggestion_status) {
  // Can we get suggested questions
  suggestion_status = suggestion_generation_status_;
  return suggestions_;
}

void ConversationDriver::SetShouldSendPageContents(bool should_send) {
  DCHECK(should_send_page_contents_ != should_send);

  should_send_page_contents_ = should_send;
}

bool ConversationDriver::GetShouldSendPageContents() {
  return should_send_page_contents_;
}

void ConversationDriver::ClearConversationHistory() {
  chat_history_.clear();
  engine_->ClearAllQueries();
  current_error_ = mojom::APIError::None;

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
    obs.OnAPIResponseError(current_error_);
  }
}

mojom::APIError ConversationDriver::GetCurrentAPIError() {
  return current_error_;
}

void ConversationDriver::GenerateQuestions() {
  DVLOG(1) << __func__;
  // This function should not be presented in the UI if the user has not
  // opted-in yet.
  if (!HasUserOptedIn()) {
    NOTREACHED() << "GenerateQuestions should not be called before user is "
                 << "opted in to AI Chat";
    return;
  }
  // We're not expecting to call this if the UI is not active for this
  // conversation.
  DCHECK(is_conversation_active_);
  // We're not expecting to already have generated suggestions
  DCHECK_LE(suggestions_.size(), 1u);

  // Can't operate if we don't have an article text
  if (article_text_.empty()) {
    return;
  }

  if (suggestion_generation_status_ ==
          mojom::SuggestionGenerationStatus::IsGenerating ||
      suggestion_generation_status_ ==
          mojom::SuggestionGenerationStatus::HasGenerated) {
    NOTREACHED() << "UI should not allow GenerateQuestions to be called more "
                 << "than once";
    return;
  }

  suggestion_generation_status_ =
      mojom::SuggestionGenerationStatus::IsGenerating;
  OnSuggestedQuestionsChanged();
  // Make API request for questions.
  // Do not call SetRequestInProgress, this progress
  // does not need to be shown to the UI.
  auto navigation_id_for_query = current_navigation_id_;
  engine_->GenerateQuestionSuggestions(
      is_video_, article_text_,
      base::BindOnce(&ConversationDriver::OnSuggestedQuestionsResponse,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(navigation_id_for_query)));
}

void ConversationDriver::OnSuggestedQuestionsResponse(
    int64_t navigation_id,
    std::vector<std::string> result) {
  // We might have navigated away whilst this async operation is in
  // progress, so check if we're the same navigation.
  if (navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }

  suggestions_.insert(suggestions_.end(), result.begin(), result.end());
  suggestion_generation_status_ =
      mojom::SuggestionGenerationStatus::HasGenerated;
  // Notify observers
  OnSuggestedQuestionsChanged();
  DVLOG(2) << "Got questions:" << base::JoinString(suggestions_, "\n");
}

void ConversationDriver::MakeAPIRequestWithConversationHistoryUpdate(
    mojom::ConversationTurn turn,
    bool needs_page_content /* = false */) {
  // Decide if this entry needs to wait for one of:
  // - user to be opted-in
  // - conversation to be active
  // - content to be retrieved
  if (!is_conversation_active_ || !HasUserOptedIn() ||
      (article_text_.empty() && needs_page_content)) {
    // This function should not be presented in the UI if the user has not
    // opted-in yet.
    pending_conversation_entry_ =
        std::make_unique<mojom::ConversationTurn>(std::move(turn));

    if (article_text_.empty() && needs_page_content) {
      pending_message_needs_page_content_ = true;
    }

    // Invoking this before the creation of the page handler means the pending
    // request will not be reported.
    OnConversationEntryPending();
    return;
  }

  DCHECK(turn.character_type == CharacterType::HUMAN);

  bool is_suggested_question = false;

  // If it's a suggested question, remove it
  auto found_question_iter = base::ranges::find(suggestions_, turn.text);
  if (found_question_iter != suggestions_.end()) {
    is_suggested_question = true;
    suggestions_.erase(found_question_iter);
    OnSuggestedQuestionsChanged();
  }

  // Directly modify Entry's text to remove engine-breaking substrings
  engine_->SanitizeInput(turn.text);

  // TODO(petemill): Tokenize the summary question so that we
  // don't have to do this weird substitution.
  std::string question_part;
  if (turn.text == l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE)) {
    question_part =
        l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE);
  } else if (turn.text ==
             l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_VIDEO)) {
    question_part =
        l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_VIDEO);
  } else {
    question_part = turn.text;
  }

  // Suggested questions were based on only the initial prompt (with content),
  // so no need to submit all conversation history when they are used.
  std::vector<mojom::ConversationTurn> history =
      is_suggested_question ? std::vector<mojom::ConversationTurn>()
                            : chat_history_;

  auto data_received_callback = base::BindRepeating(
      &ConversationDriver::OnEngineCompletionDataReceived,
      weak_ptr_factory_.GetWeakPtr(), current_navigation_id_);

  auto data_completed_callback =
      base::BindOnce(&ConversationDriver::OnEngineCompletionComplete,
                     weak_ptr_factory_.GetWeakPtr(), current_navigation_id_);

  // Now the conversation is committed, we can remove some unneccessary data if
  // we're not associated with a page.
  if (!should_send_page_contents_) {
    article_text_.clear();
    suggestions_.clear();
    OnSuggestedQuestionsChanged();
  }

  engine_->GenerateAssistantResponse(
      is_video_, article_text_, history, question_part,
      std::move(data_received_callback), std::move(data_completed_callback));

  // Add the human part to the conversation
  AddToConversationHistory(std::move(turn));

  is_request_in_progress_ = true;
  for (auto& obs : observers_) {
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }
}

void ConversationDriver::RetryAPIRequest() {
  SetAPIError(mojom::APIError::None);
  DCHECK(!chat_history_.empty());

  // We're using a reverse iterator here to find the latest human turn
  for (std::vector<ConversationTurn>::reverse_iterator rit =
           chat_history_.rbegin();
       rit != chat_history_.rend(); ++rit) {
    if (rit->character_type == CharacterType::HUMAN) {
      auto turn = *std::make_move_iterator(rit);
      auto human_turn_iter = rit.base() - 1;
      chat_history_.erase(human_turn_iter, chat_history_.end());
      MakeAPIRequestWithConversationHistoryUpdate(turn);
      break;
    }
  }
}

bool ConversationDriver::IsRequestInProgress() {
  return is_request_in_progress_;
}

void ConversationDriver::OnEngineCompletionDataReceived(int64_t navigation_id,
                                                  std::string result) {
  if (navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }

  UpdateOrCreateLastAssistantEntry(result);

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }
}

void ConversationDriver::OnEngineCompletionComplete(
    int64_t navigation_id,
    EngineConsumer::GenerationResult result) {
  if (navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }

  is_request_in_progress_ = false;

  if (result.has_value()) {
    // Handle success, which might mean do nothing much since all
    // data was passed in the streaming "received" callback.
    if (!result->empty()) {
      UpdateOrCreateLastAssistantEntry(*result);
    }
  } else {
    // handle failure
    SetAPIError(std::move(result.error()));
  }

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }
}

void ConversationDriver::OnSuggestedQuestionsChanged() {
  for (auto& obs : observers_) {
    obs.OnSuggestedQuestionsChanged(suggestions_,
                                    suggestion_generation_status_);
  }
}

void ConversationDriver::OnPageHasContentChanged(mojom::SiteInfoPtr site_info) {
  for (auto& obs : observers_) {
    obs.OnPageHasContent(std::move(site_info));
  }
}

void ConversationDriver::SetAPIError(const mojom::APIError& error) {
  current_error_ = error;

  for (Observer& obs : observers_) {
    obs.OnAPIResponseError(current_error_);
  }
}

bool ConversationDriver::HasPendingConversationEntry() {
  return pending_conversation_entry_ != nullptr;
}

bool ConversationDriver::IsPageContentsTruncated() {
  if (article_text_.empty()) {
    return false;
  }
  return (static_cast<uint32_t>(article_text_.length()) >
          GetCurrentModel().max_page_content_length);
}

void ConversationDriver::SubmitSummarizationRequest() {
  DCHECK(IsContentAssociationPossible())
      << "This conversation request is not associated with content\n";
  DCHECK(should_send_page_contents_)
      << "This conversation request should send page contents\n";

  mojom::ConversationTurn turn = {
      CharacterType::HUMAN, ConversationTurnVisibility::VISIBLE,
      l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE)};
  MakeAPIRequestWithConversationHistoryUpdate(std::move(turn),
                                              /*needs_page_content=*/true);
}

mojom::SiteInfoPtr ConversationDriver::BuildSiteInfo() {
  mojom::SiteInfoPtr site_info = mojom::SiteInfo::New();
  site_info->title = base::UTF16ToUTF8(GetPageTitle());
  site_info->is_content_truncated = IsPageContentsTruncated();
  site_info->is_content_association_possible = IsContentAssociationPossible();
  return site_info;
}

bool ConversationDriver::IsContentAssociationPossible() {
  const GURL url = GetPageURL();

  if (!base::Contains(kAllowedSchemes, url.scheme())) {
    return false;
  }

  return true;
}

void ConversationDriver::GetPremiumStatus(
    mojom::PageHandler::GetPremiumStatusCallback callback) {
  credential_manager_->GetPremiumStatus(
      base::BindOnce(&ConversationDriver::OnPremiumStatusReceived,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ConversationDriver::OnPremiumStatusReceived(
    mojom::PageHandler::GetPremiumStatusCallback parent_callback,
    mojom::PremiumStatus premium_status,
    mojom::PremiumInfoPtr premium_info) {
  if (last_premium_status_ != premium_status &&
      premium_status == mojom::PremiumStatus::Active) {
    // Change model if we haven't already
    ChangeModel(features::kAIModelsPremiumDefaultKey.Get());
  }
  last_premium_status_ = premium_status;
  if (HasUserOptedIn()) {
    ai_chat_metrics_->OnPremiumStatusUpdated(false, premium_status,
                                             std::move(premium_info));
  }
  std::move(parent_callback).Run(premium_status, std::move(premium_info));
}

void ConversationDriver::OnConversationEntryPending() {
  for (auto& obs : observers_) {
    obs.OnConversationEntryPending();
  }
}

bool ConversationDriver::GetCanShowPremium() {
  bool has_user_dismissed_prompt =
      pref_service_->GetBoolean(ai_chat::prefs::kUserDismissedPremiumPrompt);

  if (has_user_dismissed_prompt) {
    return false;
  }

  base::Time last_accepted_disclaimer =
      pref_service_->GetTime(ai_chat::prefs::kLastAcceptedDisclaimer);

  // Can't show if we haven't accepted disclaimer yet
  if (last_accepted_disclaimer.is_null()) {
    return false;
  }

  base::Time time_1_day_ago = base::Time::Now() - base::Days(1);
  bool is_more_than_24h_since_last_seen =
      last_accepted_disclaimer < time_1_day_ago;

  if (is_more_than_24h_since_last_seen) {
    return true;
  }

  return false;
}

void ConversationDriver::DismissPremiumPrompt() {
  pref_service_->SetBoolean(ai_chat::prefs::kUserDismissedPremiumPrompt, true);
}

void ConversationDriver::RateMessage(
    bool is_liked,
    uint32_t turn_id,
    mojom::PageHandler::RateMessageCallback callback) {
  auto on_complete = base::BindOnce(
      [](mojom::PageHandler::RateMessageCallback callback,
         APIRequestResult result) {
        if (result.Is2XXResponseCode() && result.value_body().is_dict()) {
          std::string id = *result.value_body().GetDict().FindString("id");
          std::move(callback).Run(id);
          return;
        }
        std::move(callback).Run(absl::nullopt);
      },
      std::move(callback));

  const std::vector<mojom::ConversationTurn>& history =
      GetConversationHistory();

  // TODO(petemill): Something more robust than relying on message index,
  // and probably a message uuid.
  uint32_t current_turn_id = turn_id + 1;

  if (current_turn_id <= history.size()) {
    base::span<const mojom::ConversationTurn> history_slice =
        base::make_span(history).first(current_turn_id);

    feedback_api_->SendRating(is_liked, history_slice, GetCurrentModel().name,
                              std::move(on_complete));

    return;
  }

  std::move(callback).Run(absl::nullopt);
}

void ConversationDriver::SendFeedback(
    const std::string& category,
    const std::string& feedback,
    const std::string& rating_id,
    mojom::PageHandler::SendFeedbackCallback callback) {
  auto on_complete = base::BindOnce(
      [](mojom::PageHandler::SendFeedbackCallback callback,
         APIRequestResult result) {
        if (result.Is2XXResponseCode()) {
          std::move(callback).Run(true);
          return;
        }

        std::move(callback).Run(false);
      },
      std::move(callback));

  feedback_api_->SendFeedback(category, feedback, rating_id,
                              std::move(on_complete));
}

}  // namespace ai_chat
