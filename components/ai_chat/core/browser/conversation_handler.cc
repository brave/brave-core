// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_handler.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_feedback_api.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/associated_archive_content.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/local_models_updater.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/l10n/l10n_util.h"

namespace ai_chat {

namespace {

using ai_chat::mojom::CharacterType;
using ai_chat::mojom::ConversationTurn;

using AssociatedContentDelegate =
    ConversationHandler::AssociatedContentDelegate;

const base::flat_map<mojom::ActionType, std::string>&
GetActionTypeQuestionMap() {
  static const base::NoDestructor<
      base::flat_map<mojom::ActionType, std::string>>
      kMap({{mojom::ActionType::SUMMARIZE_PAGE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE)},
            {mojom::ActionType::SUMMARIZE_VIDEO,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_VIDEO)},
            {mojom::ActionType::SUMMARIZE_SELECTED_TEXT,
             l10n_util::GetStringUTF8(
                 IDS_AI_CHAT_QUESTION_SUMMARIZE_SELECTED_TEXT)},
            {mojom::ActionType::EXPLAIN,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_EXPLAIN)},
            {mojom::ActionType::PARAPHRASE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_PARAPHRASE)},
            {mojom::ActionType::CREATE_TAGLINE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_CREATE_TAGLINE)},
            {mojom::ActionType::CREATE_SOCIAL_MEDIA_COMMENT_SHORT,
             l10n_util::GetStringUTF8(
                 IDS_AI_CHAT_QUESTION_CREATE_SOCIAL_MEDIA_COMMENT_SHORT)},
            {mojom::ActionType::CREATE_SOCIAL_MEDIA_COMMENT_LONG,
             l10n_util::GetStringUTF8(
                 IDS_AI_CHAT_QUESTION_CREATE_SOCIAL_MEDIA_COMMENT_LONG)},
            {mojom::ActionType::IMPROVE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_IMPROVE)},
            {mojom::ActionType::PROFESSIONALIZE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_PROFESSIONALIZE)},
            {mojom::ActionType::PERSUASIVE_TONE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_PERSUASIVE_TONE)},
            {mojom::ActionType::CASUALIZE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_CASUALIZE)},
            {mojom::ActionType::FUNNY_TONE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_FUNNY_TONE)},
            {mojom::ActionType::ACADEMICIZE,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_ACADEMICIZE)},
            {mojom::ActionType::SHORTEN,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SHORTEN)},
            {mojom::ActionType::EXPAND,
             l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_EXPAND)}});
  return *kMap;
}

const std::string& GetActionTypeQuestion(mojom::ActionType action_type) {
  const auto& map = GetActionTypeQuestionMap();
  auto iter = map.find(action_type);
  CHECK(iter != map.end());
  return iter->second;
}

uint32_t GetMaxContentLengthForModel(const mojom::Model& model) {
  return model.options->is_custom_model_options()
             ? kCustomModelMaxPageContentLength
             : model.options->get_leo_model_options()->max_page_content_length;
}

}  // namespace

AssociatedContentDelegate::AssociatedContentDelegate()
    : text_embedder_(nullptr, base::OnTaskRunnerDeleter(nullptr)) {}

AssociatedContentDelegate::~AssociatedContentDelegate() = default;

void AssociatedContentDelegate::OnNewPage(int64_t navigation_id) {
  pending_top_similarity_requests_.clear();
  if (text_embedder_) {
    text_embedder_->CancelAllTasks();
    text_embedder_.reset();
  }
}

void AssociatedContentDelegate::GetStagedEntriesFromContent(
    GetStagedEntriesCallback callback) {
  std::move(callback).Run(std::nullopt);
}

void AssociatedContentDelegate::GetTopSimilarityWithPromptTilContextLimit(
    const std::string& prompt,
    const std::string& text,
    uint32_t context_limit,
    TextEmbedder::TopSimilarityCallback callback) {
  // Create TextEmbedder
  if (!text_embedder_) {
    base::FilePath universal_qa_model_path =
        LocalModelsUpdaterState::GetInstance()->GetUniversalQAModel();
    // Tasks in TextEmbedder are run on |embedder_task_runner|. The
    // text_embedder_ must be deleted on that sequence to guarantee that pending
    // tasks can safely be executed.
    scoped_refptr<base::SequencedTaskRunner> embedder_task_runner =
        base::ThreadPool::CreateSequencedTaskRunner(
            {base::MayBlock(), base::TaskPriority::USER_BLOCKING});
    text_embedder_ = TextEmbedder::Create(
        base::FilePath(universal_qa_model_path), embedder_task_runner);
    if (!text_embedder_) {
      std::move(callback).Run(
          base::unexpected("Failed to create TextEmbedder"));
      pending_top_similarity_requests_.pop_back();
      return;
    }
  }

  if (!text_embedder_->IsInitialized()) {
    // Will have to wait for initialization to complete, store params for
    // calling later.
    pending_top_similarity_requests_.emplace_back(prompt, text, context_limit,
                                                  std::move(callback));

    text_embedder_->Initialize(
        base::BindOnce(&ConversationHandler::AssociatedContentDelegate::
                           OnTextEmbedderInitialized,
                       weak_ptr_factory_.GetWeakPtr()));
  } else {
    // Run immediately if already initialized
    text_embedder_->GetTopSimilarityWithPromptTilContextLimit(
        prompt, text, context_limit, std::move(callback));
  }
}

void AssociatedContentDelegate::OnTextEmbedderInitialized(bool initialized) {
  if (!initialized) {
    VLOG(1) << "Failed to initialize TextEmbedder";
    for (auto& callback_info : pending_top_similarity_requests_) {
      std::move(std::get<3>(callback_info))
          .Run(base::unexpected<std::string>(
              "Failed to initialize TextEmbedder"));
    }
    pending_top_similarity_requests_.clear();
    return;
  }

  CHECK(text_embedder_);
  for (auto& callback_info : pending_top_similarity_requests_) {
    text_embedder_->GetTopSimilarityWithPromptTilContextLimit(
        std::get<0>(callback_info), std::get<1>(callback_info),
        std::get<2>(callback_info), std::move(std::get<3>(callback_info)));
  }
  pending_top_similarity_requests_.clear();
}

ConversationHandler::ConversationHandler(
    const mojom::Conversation* conversation,
    AIChatService* ai_chat_service,
    ModelService* model_service,
    AIChatCredentialManager* credential_manager,
    AIChatFeedbackAPI* feedback_api,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : metadata_(conversation),
      ai_chat_service_(ai_chat_service),
      model_service_(model_service),
      credential_manager_(credential_manager),
      feedback_api_(feedback_api),
      url_loader_factory_(url_loader_factory) {
  // When a client disconnects, let observers know
  receivers_.set_disconnect_handler(
      base::BindRepeating(&ConversationHandler::OnClientConnectionChanged,
                          weak_ptr_factory_.GetWeakPtr()));
  models_observer_.Observe(model_service_.get());
  // TODO(petemill): differ based on premium status, if different
  ChangeModel(model_service->GetDefaultModelKey());
}

ConversationHandler::~ConversationHandler() {
  if (associated_content_delegate_) {
    associated_content_delegate_->OnRelatedConversationDestroyed(this);
  }
}

void ConversationHandler::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ConversationHandler::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void ConversationHandler::Bind(
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  receivers_.Add(this, std::move(receiver));
  conversation_ui_handlers_.Add(std::move(conversation_ui_handler));
  OnClientConnectionChanged();
  // In some cases, this page handler hasn't been created and remote might not
  // have been set yet.
  // ex. A user may ask a question from the location bar
  if (!pending_conversation_entry_.is_null()) {
    OnHistoryUpdate();
  }
  MaybeFetchOrClearContentStagedConversation();
  MaybePopPendingRequests();
}

bool ConversationHandler::IsAnyClientConnected() {
  DVLOG(2) << metadata_->uuid << " HAS " << receivers_.size() << " RECEIVERS!";
  return !receivers_.empty();
}

bool ConversationHandler::HasAnyHistory() {
  return !chat_history_.empty();
}

void ConversationHandler::InitEngine() {
  CHECK(!model_key_.empty());
  const mojom::Model* model = model_service_->GetModel(model_key_);
  // Make sure we get a valid model, defaulting to static default or first.
  if (!model) {
    // It is unexpected that we get here. Dump a call stack
    // to help figure out why it happens.
    SCOPED_CRASH_KEY_STRING1024("BraveAIChatModel", "key", model_key_);
    base::debug::DumpWithoutCrashing();
    // Use default
    model = model_service_->GetModel(features::kAIModelsDefaultKey.Get());
    DCHECK(model) << "The default model set via feature param does not exist";
    if (!model) {
      const auto& all_models = model_service_->GetModels();
      // Use first if given bad default value
      model = all_models.at(0).get();
    }
  }

  // Model's key might not be the same as what we asked for (e.g. if the model
  // no longer exists).
  model_key_ = model->key;

  engine_ = model_service_->GetEngineForModel(model_key_, url_loader_factory_,
                                              credential_manager_);

  OnModelDataChanged();

  if (is_request_in_progress_) {
    // Pending requests have been deleted along with the model engine
    is_request_in_progress_ = false;
    for (auto& client : conversation_ui_handlers_) {
      client->OnAPIRequestInProgress(is_request_in_progress_);
    }
  }

  // When the model changes, the content truncation might be different,
  // and the UI needs to know.
  if (associated_content_delegate_ &&
      !associated_content_delegate_->GetCachedTextContent().empty()) {
    OnAssociatedContentInfoChanged();
  }
}

void ConversationHandler::OnAssociatedContentDestroyed(
    std::string last_text_content,
    bool is_video) {
  // The associated content delegate is destroyed, so we should not try to
  // fetch. It may be populated later, e.g. through back navigation.
  // If this conversation is allowed to be associated with content, we can keep
  // using our current cached content.
  associated_content_delegate_ = nullptr;
  if (!chat_history_.empty() && should_send_page_contents_ &&
      associated_content_info_ && associated_content_info_->url.has_value()) {
    // Get the latest version of article text and
    // associated_content_info_ if this chat has history and was connected to
    // the associated conversation, then construct a "content archive"
    // implementation of AssociatedContentDelegate with a duplicate of the
    // article text.
    auto archive_content = std::make_unique<AssociatedArchiveContent>(
        associated_content_info_->url.value_or(GURL()), last_text_content,
        base::UTF8ToUTF16(associated_content_info_->title.value_or("")),
        is_video);
    associated_content_delegate_ = archive_content->GetWeakPtr();
    archive_content_ = std::move(archive_content);
  }
  OnAssociatedContentInfoChanged();
}

void ConversationHandler::SetAssociatedContentDelegate(
    base::WeakPtr<AssociatedContentDelegate> delegate) {
  // If this conversation is allowed to fetch content, this is the delegate
  // that can provide fresh content for the conversation.
  CHECK(delegate)
      << "Don't send a null delegate. Start a new conversation instead.";

  if (associated_content_delegate_ &&
      (delegate.get() == associated_content_delegate_.get())) {
    return;
  }

  // Unarchive content
  if (archive_content_) {
    associated_content_delegate_ = nullptr;
    archive_content_ = nullptr;
  } else if (!chat_history_.empty()) {
    // Cannot associate new content with a conversation which already has
    // messages but this is ok since we're probably just defaulting this
    // conversation to be "alongside" this target content (e.g. sidebar). The
    // service will do the association and we can ignore the request to
    // associate content.
    return;
  }

  associated_content_delegate_ = delegate;
  associated_content_delegate_->AddRelatedConversation(this);
  // Default to send page contents when we have a valid contents.
  // This class should only be provided with a delegate when
  // it is allowed to use it (e.g. not internal WebUI content).
  // The user can toggle this via the UI.
  should_send_page_contents_ = true;

  MaybeSeedOrClearSuggestions();
  MaybeFetchOrClearContentStagedConversation();
  OnAssociatedContentInfoChanged();
}

const mojom::Model& ConversationHandler::GetCurrentModel() {
  const mojom::Model* model = model_service_->GetModel(model_key_);
  CHECK(model);
  return *model;
}

const std::vector<mojom::ConversationTurnPtr>&
ConversationHandler::GetConversationHistory() const {
  return chat_history_;
}

void ConversationHandler::GetConversationHistory(
    GetConversationHistoryCallback callback) {
  std::vector<mojom::ConversationTurnPtr> history;
  for (const auto& turn : chat_history_) {
    history.emplace_back(turn->Clone());
  }

  if (pending_conversation_entry_ &&
      pending_conversation_entry_->visibility !=
          mojom::ConversationTurnVisibility::HIDDEN) {
    history.push_back(pending_conversation_entry_->Clone());
  }

  std::move(callback).Run(std::move(history));
}

void ConversationHandler::RateMessage(bool is_liked,
                                      uint32_t turn_id,
                                      RateMessageCallback callback) {
  auto& model = GetCurrentModel();

  // We only allow Leo models to be rated.
  CHECK(model.options->is_leo_model_options());

  const std::vector<mojom::ConversationTurnPtr>& history = chat_history_;

  auto on_complete = base::BindOnce(
      [](RateMessageCallback callback, APIRequestResult result) {
        if (result.Is2XXResponseCode() && result.value_body().is_dict()) {
          std::string id = *result.value_body().GetDict().FindString("id");
          std::move(callback).Run(id);
          return;
        }
        std::move(callback).Run(std::nullopt);
      },
      std::move(callback));

  // TODO(petemill): Something more robust than relying on message index,
  // and probably a message uuid.
  uint32_t current_turn_id = turn_id + 1;

  if (current_turn_id <= history.size()) {
    base::span<const mojom::ConversationTurnPtr> history_slice =
        base::make_span(history).first(current_turn_id);

    feedback_api_->SendRating(
        is_liked, ai_chat_service_->IsPremiumStatus(), history_slice,
        model.options->get_leo_model_options()->name, std::move(on_complete));

    return;
  }

  std::move(callback).Run(std::nullopt);
}

void ConversationHandler::SendFeedback(const std::string& category,
                                       const std::string& feedback,
                                       const std::string& rating_id,
                                       bool send_hostname,
                                       SendFeedbackCallback callback) {
  auto on_complete = base::BindOnce(
      [](SendFeedbackCallback callback, APIRequestResult result) {
        if (result.Is2XXResponseCode()) {
          std::move(callback).Run(true);
          return;
        }

        std::move(callback).Run(false);
      },
      std::move(callback));

  if (!associated_content_delegate_) {
    send_hostname = false;
  }

  const GURL page_url =
      send_hostname ? associated_content_delegate_->GetURL() : GURL();

  feedback_api_->SendFeedback(category, feedback, rating_id,
                              (send_hostname && page_url.SchemeIsHTTPOrHTTPS())
                                  ? std::optional<std::string>(page_url.host())
                                  : std::nullopt,
                              std::move(on_complete));
}

void ConversationHandler::GetConversationUuid(
    GetConversationUuidCallback callback) {
  std::move(callback).Run(metadata_->uuid);
}

void ConversationHandler::GetModels(GetModelsCallback callback) {
  const auto& models = model_service_->GetModels();
  std::vector<mojom::ModelPtr> models_copy(models.size());
  std::transform(models.cbegin(), models.cend(), models_copy.begin(),
                 [](auto& model) { return model.Clone(); });
  std::move(callback).Run(std::move(models_copy), GetCurrentModel().key);
}

void ConversationHandler::ChangeModel(const std::string& model_key) {
  CHECK(!model_key.empty());
  // Check that the key exists
  auto* new_model = model_service_->GetModel(model_key);
  if (!new_model) {
    NOTREACHED_IN_MIGRATION()
        << "No matching model found for key: " << model_key;
    return;
  }
  model_key_ = new_model->key;
  InitEngine();
}

void ConversationHandler::GetIsRequestInProgress(
    GetIsRequestInProgressCallback callback) {
  std::move(callback).Run(is_request_in_progress_);
}

void ConversationHandler::SubmitHumanConversationEntry(
    const std::string& input) {
  DCHECK(!is_request_in_progress_)
      << "Should not be able to submit more"
      << "than a single human conversation turn at a time.";

  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      CharacterType::HUMAN, mojom::ActionType::UNSPECIFIED,
      mojom::ConversationTurnVisibility::VISIBLE, input, std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, false);
  SubmitHumanConversationEntry(std::move(turn));
}

void ConversationHandler::SubmitHumanConversationEntry(
    mojom::ConversationTurnPtr turn) {
  VLOG(1) << __func__;
  DVLOG(4) << __func__ << ": " << turn->text;

  // If there's edits, use the last one as the latest turn.
  bool has_edits = turn->edits && !turn->edits->empty();
  mojom::ConversationTurnPtr& latest_turn =
      has_edits ? turn->edits->back() : turn;
  // Decide if this entry needs to wait for one of:
  // - user to be opted-in
  // - conversation to be active
  // - is request in progress (should only be possible if regular entry is
  // in-progress and another entry is submitted outside of regular UI, e.g. from
  // location bar or context menu.
  // if (!is_conversation_active_ || !ai_chat_service_->HasUserOptedIn() ||
  if (!ai_chat_service_->HasUserOptedIn() || is_request_in_progress_) {
    VLOG(1) << "Adding as a pending conversation entry";
    // This is possible (on desktop) if user submits multiple location bar
    // messages before an entry is complete. But that should be obvious from the
    // UI that the 1 in-progress + 1 pending message is the limit.
    if (pending_conversation_entry_) {
      VLOG(1) << "Should not be able to add a pending conversation entry "
              << "when there is already a pending conversation entry.";
      return;
    }
    pending_conversation_entry_ = std::move(turn);
    // Pending entry is added to conversation history when asked for
    // so notify observers.
    OnHistoryUpdate();
    return;
  }
  DCHECK(latest_turn->character_type == mojom::CharacterType::HUMAN);
  is_request_in_progress_ = true;
  OnAPIRequestInProgressChanged();
  // If it's a suggested question, remove it
  auto found_question_iter =
      base::ranges::find(suggestions_, latest_turn->text);
  if (found_question_iter != suggestions_.end()) {
    suggestions_.erase(found_question_iter);
    OnSuggestedQuestionsChanged();
  }
  // Directly modify Entry's text to remove engine-breaking substrings
  if (!has_edits) {  // Edits are already sanitized.
    engine_->SanitizeInput(latest_turn->text);
  }
  if (latest_turn->selected_text) {
    engine_->SanitizeInput(*latest_turn->selected_text);
  }
  // TODO(petemill): Tokenize the summary question so that we
  // don't have to do this weird substitution.
  // TODO(jocelyn): Assigning turn.type below is a workaround for now since
  // callers of SubmitHumanConversationEntry mojo API currently don't have
  // action_type specified.
  std::string question_part = latest_turn->text;
  if (latest_turn->action_type == mojom::ActionType::UNSPECIFIED) {
    if (latest_turn->text ==
        l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE)) {
      latest_turn->action_type = mojom::ActionType::SUMMARIZE_PAGE;
      question_part =
          l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE);
    } else if (latest_turn->text ==
               l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_VIDEO)) {
      latest_turn->action_type = mojom::ActionType::SUMMARIZE_VIDEO;
      question_part =
          l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_VIDEO);
    } else {
      latest_turn->action_type = mojom::ActionType::QUERY;
    }
  }

  // Add the human part to the conversation
  AddToConversationHistory(std::move(turn));
  const bool is_page_associated =
      IsContentAssociationPossible() && should_send_page_contents_;
  if (is_page_associated) {
    // Fetch updated page content before performing generation
    GeneratePageContent(
        base::BindOnce(&ConversationHandler::PerformAssistantGeneration,
                       weak_ptr_factory_.GetWeakPtr(), question_part));
  } else {
    // Now the conversation is committed, we can remove some unneccessary data
    // if we're not associated with a page.
    suggestions_.clear();
    associated_content_delegate_ = nullptr;
    OnSuggestedQuestionsChanged();
    // Perform generation immediately
    PerformAssistantGeneration(question_part);
  }
}

void ConversationHandler::SubmitHumanConversationEntryWithAction(
    const std::string& input,
    mojom::ActionType action_type) {
  DCHECK(!is_request_in_progress_)
      << "Should not be able to submit more"
      << "than a single human conversation turn at a time.";

  SubmitSelectedText(input, action_type);
}

void ConversationHandler::ModifyConversation(uint32_t turn_index,
                                             const std::string& new_text) {
  if (turn_index >= chat_history_.size()) {
    return;
  }

  auto& turn = chat_history_.at(turn_index);
  // Modifying answer, create an entry in edits with updated completion event.
  if (turn->character_type == CharacterType::ASSISTANT) {
    if (!turn->events || turn->events->empty()) {
      return;
    }

    std::optional<size_t> completion_event_index;
    for (size_t i = 0; i < turn->events->size(); ++i) {
      if (turn->events->at(i)->is_completion_event()) {
        completion_event_index = i;
        break;
      }
    }
    if (!completion_event_index.has_value()) {
      return;
    }

    std::string trimmed_input;
    base::TrimWhitespaceASCII(new_text, base::TRIM_ALL, &trimmed_input);
    if (trimmed_input.empty() ||
        trimmed_input == turn->events->at(*completion_event_index)
                             ->get_completion_event()
                             ->completion) {
      return;
    }

    std::vector<mojom::ConversationEntryEventPtr> events;
    for (auto& event : *turn->events) {
      events.push_back(event->Clone());
    }

    auto edited_turn = mojom::ConversationTurn::New(
        turn->character_type, turn->action_type, turn->visibility,
        trimmed_input, std::nullopt /* selected_text */, std::move(events),
        base::Time::Now(), std::nullopt /* edits */, false);
    edited_turn->events->at(*completion_event_index)
        ->get_completion_event()
        ->completion = trimmed_input;

    if (!turn->edits) {
      turn->edits.emplace();
    }
    turn->edits->emplace_back(std::move(edited_turn));

    OnHistoryUpdate();
    return;
  }

  // Modifying human turn, create an entry in edits with updated text, drop
  // anything after this turn_index and resubmit.
  std::string sanitized_input = new_text;
  engine_->SanitizeInput(sanitized_input);
  const auto& current_text = turn->edits && !turn->edits->empty()
                                 ? turn->edits->back()->text
                                 : turn->text;
  if (sanitized_input.empty() || sanitized_input == current_text) {
    return;
  }

  // turn->selected_text and turn->events are actually std::nullopt for
  // editable human turns in our current implementation, just use std::nullopt
  // here directly to be more explicit and avoid confusion.
  auto edited_turn = mojom::ConversationTurn::New(
      turn->character_type, turn->action_type, turn->visibility,
      sanitized_input, std::nullopt /* selected_text */,
      std::nullopt /* events */, base::Time::Now(), std::nullopt /* edits */,
      false);
  if (!turn->edits) {
    turn->edits.emplace();
  }
  turn->edits->emplace_back(std::move(edited_turn));

  auto new_turn = std::move(chat_history_.at(turn_index));
  chat_history_.erase(chat_history_.begin() + turn_index, chat_history_.end());
  OnHistoryUpdate();

  SubmitHumanConversationEntry(std::move(new_turn));
}

void ConversationHandler::SubmitSummarizationRequest() {
  DCHECK(IsContentAssociationPossible())
      << "This conversation request is not associated with content";
  DCHECK(should_send_page_contents_)
      << "This conversation request should send page contents";

  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_PAGE,
      mojom::ConversationTurnVisibility::VISIBLE,
      l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE), std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, false);
  SubmitHumanConversationEntry(std::move(turn));
}

void ConversationHandler::GetSuggestedQuestions(
    GetSuggestedQuestionsCallback callback) {
  std::move(callback).Run(suggestions_, suggestion_generation_status_);
}

void ConversationHandler::GenerateQuestions() {
  DVLOG(1) << __func__;
  // This function should not be presented in the UI if the user has not
  // opted-in yet.
  if (!ai_chat_service_->HasUserOptedIn()) {
    DLOG(ERROR) << "GenerateQuestions should not be called before user is "
                << "opted in to AI Chat";
    return;
  }
  if (!should_send_page_contents_) {
    DLOG(ERROR) << "Cannot get suggestions when not associated with content.";
    return;
  }
  if (!IsContentAssociationPossible()) {
    DLOG(ERROR)
        << "Should not be associated with content when not allowed to be";
    return;
  }
  // We're not expecting to call this if the UI is not active for this
  // conversation.
  if (!IsAnyClientConnected()) {
    DLOG(ERROR) << "GenerateQuestions should not be called when no clients are "
                   "connected to this conversation";
    return;
  }
  // We're not expecting to already have generated suggestions
  if (suggestions_.size() >= 1u) {
    DLOG(ERROR) << "GenerateQuestions should not be called more than once";
    return;
  }

  if (suggestion_generation_status_ ==
          mojom::SuggestionGenerationStatus::IsGenerating ||
      suggestion_generation_status_ ==
          mojom::SuggestionGenerationStatus::HasGenerated) {
    DLOG(ERROR) << "UI should not allow GenerateQuestions to be called more "
                << "than once";
    return;
  }

  suggestion_generation_status_ =
      mojom::SuggestionGenerationStatus::IsGenerating;
  OnSuggestedQuestionsChanged();
  // Make API request for questions but first get page content.
  // Do not call SetRequestInProgress, this progress
  // does not need to be shown to the UI.
  GeneratePageContent(
      base::BindOnce(&ConversationHandler::PerformQuestionGeneration,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ConversationHandler::PerformQuestionGeneration(
    std::string page_content,
    bool is_video,
    std::string invalidation_token) {
  engine_->GenerateQuestionSuggestions(
      is_video, page_content,
      base::BindOnce(&ConversationHandler::OnSuggestedQuestionsResponse,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ConversationHandler::GetAssociatedContentInfo(
    GetAssociatedContentInfoCallback callback) {
  BuildAssociatedContentInfo();
  std::move(callback).Run(associated_content_info_->Clone(),
                          should_send_page_contents_);
}

void ConversationHandler::SetShouldSendPageContents(bool should_send) {
  if (should_send_page_contents_ == should_send) {
    return;
  }
  if (!IsContentAssociationPossible() && should_send) {
    return;
  }
  should_send_page_contents_ = should_send;

  OnAssociatedContentInfoChanged();
  MaybeSeedOrClearSuggestions();
  MaybeFetchOrClearContentStagedConversation();
}

void ConversationHandler::RetryAPIRequest() {
  SetAPIError(mojom::APIError::None);
  DCHECK(!chat_history_.empty());

  // We're using a reverse iterator here to find the latest human turn
  for (std::vector<mojom::ConversationTurnPtr>::reverse_iterator rit =
           chat_history_.rbegin();
       rit != chat_history_.rend(); ++rit) {
    if (rit->get()->character_type == CharacterType::HUMAN) {
      auto turn = *std::make_move_iterator(rit);
      auto human_turn_iter = rit.base() - 1;
      chat_history_.erase(human_turn_iter, chat_history_.end());
      SubmitHumanConversationEntry(std::move(turn));
      break;
    }
  }
}

void ConversationHandler::GetAPIResponseError(
    GetAPIResponseErrorCallback callback) {
  std::move(callback).Run(current_error_);
}

void ConversationHandler::ClearErrorAndGetFailedMessage(
    ClearErrorAndGetFailedMessageCallback callback) {
  DCHECK(!chat_history_.empty());

  SetAPIError(mojom::APIError::None);
  mojom::ConversationTurnPtr turn = std::move(*chat_history_.end());
  chat_history_.erase(chat_history_.end());

  OnHistoryUpdate();

  std::move(callback).Run(std::move(turn));
}

void ConversationHandler::SubmitSelectedText(
    const std::string& selected_text,
    mojom::ActionType action_type,
    GeneratedTextCallback received_callback,
    EngineConsumer::GenerationCompletedCallback completed_callback) {
  const std::string& question = GetActionTypeQuestion(action_type);
  SubmitSelectedTextWithQuestion(selected_text, question, action_type,
                                 std::move(received_callback),
                                 std::move(completed_callback));
}

void ConversationHandler::SubmitSelectedTextWithQuestion(
    const std::string& selected_text,
    const std::string& question,
    mojom::ActionType action_type,
    GeneratedTextCallback received_callback,
    EngineConsumer::GenerationCompletedCallback completed_callback) {
  if (received_callback && completed_callback) {
    // Start a one-off request and replace in-place with the result.
    // TODO(petemill): This should only belong in the caller location,
    // such as ai rewriter dialog (or a shared utility).
    engine_->GenerateRewriteSuggestion(
        selected_text, question,
        base::BindRepeating(
            [](GeneratedTextCallback received_callback,
               mojom::ConversationEntryEventPtr rewrite_event) {
              constexpr char kResponseTagPattern[] =
                  "<\\/?(response|respons|respon|respo|resp|res|re|r)?$";
              if (!rewrite_event->is_completion_event()) {
                return;
              }

              std::string suggestion =
                  rewrite_event->get_completion_event()->completion;

              base::TrimWhitespaceASCII(suggestion, base::TRIM_ALL,
                                        &suggestion);
              if (suggestion.empty()) {
                return;
              }

              // Avoid showing the ending tag.
              if (RE2::PartialMatch(suggestion, kResponseTagPattern)) {
                return;
              }

              received_callback.Run(suggestion);
            },
            std::move(received_callback)),
        std::move(completed_callback));
  } else if (!received_callback && !completed_callback) {
    // Use sidebar.
    mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
        CharacterType::HUMAN, action_type,
        mojom::ConversationTurnVisibility::VISIBLE, question, selected_text,
        std::nullopt, base::Time::Now(), std::nullopt, false);

    SubmitHumanConversationEntry(std::move(turn));
  } else {
    NOTREACHED_NORETURN() << "Both callbacks must be set or unset";
  }
}

bool ConversationHandler::MaybePopPendingRequests() {
  if (IsAnyClientConnected() && !ai_chat_service_->HasUserOptedIn()) {
    return false;
  }

  if (!pending_conversation_entry_) {
    return false;
  }

  mojom::ConversationTurnPtr request = std::move(pending_conversation_entry_);
  pending_conversation_entry_.reset();
  SubmitHumanConversationEntry(std::move(request));
  return true;
}

void ConversationHandler::MaybeUnlinkAssociatedContent() {
  // Only unlink if panel is closed and there is no conversation history.
  // When panel is open or has existing conversation, do not change the state.
  // if (!is_conversation_active_ && chat_history_.empty()) {
  if (chat_history_.empty()) {
    SetShouldSendPageContents(false);
  }
}

void ConversationHandler::AddSubmitSelectedTextError(
    const std::string& selected_text,
    mojom::ActionType action_type,
    mojom::APIError error) {
  if (error == mojom::APIError::None) {
    return;
  }
  const std::string& question = GetActionTypeQuestion(action_type);
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      CharacterType::HUMAN, action_type,
      mojom::ConversationTurnVisibility::VISIBLE, question, selected_text,
      std::nullopt, base::Time::Now(), std::nullopt, false);
  AddToConversationHistory(std::move(turn));
  SetAPIError(error);
}

void ConversationHandler::OnFaviconImageDataChanged() {
  for (const mojo::Remote<mojom::ConversationUI>& client :
       conversation_ui_handlers_) {
    client->OnFaviconImageDataChanged();
  }
}

void ConversationHandler::OnUserOptedIn() {
  MaybePopPendingRequests();
  MaybeFetchOrClearContentStagedConversation();
}

void ConversationHandler::AddToConversationHistory(
    mojom::ConversationTurnPtr turn) {
  if (!turn) {
    return;
  }

  chat_history_.push_back(std::move(turn));

  OnHistoryUpdate();
}

void ConversationHandler::PerformAssistantGeneration(
    const std::string& input,
    std::string page_content /* = "" */,
    bool is_video /* = false */,
    std::string invalidation_token /* = "" */) {
  auto data_received_callback =
      base::BindRepeating(&ConversationHandler::OnEngineCompletionDataReceived,
                          weak_ptr_factory_.GetWeakPtr());

  auto data_completed_callback =
      base::BindOnce(&ConversationHandler::OnEngineCompletionComplete,
                     weak_ptr_factory_.GetWeakPtr());

  bool should_refine_page_content =
      features::IsPageContentRefineEnabled() &&
      page_content.length() > GetMaxContentLengthForModel(GetCurrentModel()) &&
      input != l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE);
  if (should_refine_page_content && associated_content_delegate_) {
    DVLOG(2) << "Asking to refine content, which is of length: "
             << page_content.length();
    associated_content_delegate_->GetTopSimilarityWithPromptTilContextLimit(
        input, page_content, GetMaxContentLengthForModel(GetCurrentModel()),
        base::BindOnce(&ConversationHandler::OnGetRefinedPageContent,
                       weak_ptr_factory_.GetWeakPtr(), input,
                       std::move(data_received_callback),
                       std::move(data_completed_callback), page_content,
                       is_video));
    return;
  } else if (!should_refine_page_content && is_content_refined_) {
    is_content_refined_ = false;
    OnAssociatedContentInfoChanged();
  }

  engine_->GenerateAssistantResponse(is_video, page_content, chat_history_,
                                     input, std::move(data_received_callback),
                                     std::move(data_completed_callback));
}

void ConversationHandler::SetAPIError(const mojom::APIError& error) {
  current_error_ = error;

  for (auto& client : conversation_ui_handlers_) {
    client->OnAPIResponseError(error);
  }
}

void ConversationHandler::UpdateOrCreateLastAssistantEntry(
    mojom::ConversationEntryEventPtr event) {
  if (chat_history_.empty() ||
      chat_history_.back()->character_type != CharacterType::ASSISTANT) {
    mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New(
        CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
        mojom::ConversationTurnVisibility::VISIBLE, "", std::nullopt,
        std::vector<mojom::ConversationEntryEventPtr>{}, base::Time::Now(),
        std::nullopt, false);
    chat_history_.push_back(std::move(entry));
  }

  auto& entry = chat_history_.back();

  if (event->is_completion_event()) {
    if (!engine_->SupportsDeltaTextResponses() || entry->events->size() == 0 ||
        !entry->events->back()->is_completion_event()) {
      // The start of completion responses needs whitespace trim
      // TODO(petemill): This should happen server-side?
      event->get_completion_event()->completion = base::TrimWhitespaceASCII(
          event->get_completion_event()->completion, base::TRIM_LEADING);
    }

    // Optimize by merging with previous completion events if delta updates
    // are supported or otherwise replacing the previous event.
    if (entry->events->size() > 0) {
      auto& last_event = entry->events->back();
      if (last_event->is_completion_event()) {
        // Merge completion events
        if (engine_->SupportsDeltaTextResponses()) {
          event->get_completion_event()->completion =
              base::StrCat({last_event->get_completion_event()->completion,
                            event->get_completion_event()->completion});
        }
        // Remove the last event because we'll replace in both delta and
        // non-delta cases
        entry->events->pop_back();
      }
    }

    // TODO(petemill): Remove ConversationTurn.text backwards compatibility when
    // all UI is updated to instead use ConversationEntryEvent items.
    entry->text = event->get_completion_event()->completion;
  }

  entry->events->push_back(std::move(event));

  OnHistoryUpdate();
}

void ConversationHandler::MaybeSeedOrClearSuggestions() {
  const bool is_page_associated =
      IsContentAssociationPossible() && should_send_page_contents_;

  if (!is_page_associated && !suggestions_.empty()) {
    suggestions_.clear();
    OnSuggestedQuestionsChanged();
    return;
  }

  if (is_page_associated && suggestions_.empty() &&
      suggestion_generation_status_ !=
          mojom::SuggestionGenerationStatus::IsGenerating &&
      suggestion_generation_status_ !=
          mojom::SuggestionGenerationStatus::HasGenerated) {
    // TODO(petemill): ask content fetcher if it knows whether current page is a
    // video.
    auto found_iter = base::ranges::find_if(
        chat_history_, [](mojom::ConversationTurnPtr& turn) {
          if (turn->action_type == mojom::ActionType::SUMMARIZE_PAGE ||
              turn->action_type == mojom::ActionType::SUMMARIZE_VIDEO) {
            return true;
          }
          return false;
        });
    const bool has_summarized = found_iter != chat_history_.end();
    if (!has_summarized) {
      suggestions_.emplace_back(
          associated_content_delegate_->GetCachedIsVideo()
              ? l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_VIDEO)
              : l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE));
    }
    suggestion_generation_status_ =
        mojom::SuggestionGenerationStatus::CanGenerate;
    OnSuggestedQuestionsChanged();
  }
}

void ConversationHandler::MaybeFetchOrClearContentStagedConversation() {
  const bool can_check_for_staged_conversation =
      IsAnyClientConnected() && ai_chat_service_->HasUserOptedIn() &&
      IsContentAssociationPossible() && should_send_page_contents_;
  if (!can_check_for_staged_conversation) {
    // Clear any staged conversation entries since user might have unassociated
    // content with this conversation
    // For now, we assume all staged conversations are 2 entries (question and
    // answer).
    if (chat_history_.size() != 2) {
      return;
    }

    const auto& last_turn = chat_history_.back();
    if (last_turn->from_brave_search_SERP) {
      chat_history_.clear();  // Clear the staged query and summary.
      OnHistoryUpdate();
      return;
    }
  }

  // Try later when we get a connected client
  if (!IsAnyClientConnected()) {
    return;
  }

  // Currently only have search query summary at the start of a conversation.
  if (!chat_history_.empty()) {
    return;
  }

  associated_content_delegate_->GetStagedEntriesFromContent(
      base::BindOnce(&ConversationHandler::OnGetStagedEntriesFromContent,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ConversationHandler::OnGetStagedEntriesFromContent(
    const std::optional<SearchQuerySummary>& search_query_summary) {
  // Check if all requirements are still met.
  if (!search_query_summary || !chat_history_.empty() ||
      !IsContentAssociationPossible() || !should_send_page_contents_ ||
      !ai_chat_service_->HasUserOptedIn()) {
    return;
  }

  // Add the query & summary to the conversation history and call
  // OnHistoryUpdate to update UI.
  chat_history_.push_back(mojom::ConversationTurn::New(
      CharacterType::HUMAN, mojom::ActionType::QUERY,
      mojom::ConversationTurnVisibility::VISIBLE, search_query_summary->query,
      std::nullopt, std::nullopt, base::Time::Now(), std::nullopt, true));
  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New(search_query_summary->summary)));
  chat_history_.push_back(mojom::ConversationTurn::New(
      CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
      mojom::ConversationTurnVisibility::VISIBLE, search_query_summary->summary,
      std::nullopt, std::move(events), base::Time::Now(), std::nullopt, true));
  OnHistoryUpdate();
}

void ConversationHandler::GeneratePageContent(GetPageContentCallback callback) {
  VLOG(1) << __func__;
  DCHECK(should_send_page_contents_);
  DCHECK(IsContentAssociationPossible())
      << "Shouldn't have been asked to generate page text when "
      << "|IsContentAssociationPossible()| is false.";

  // Make sure user is opted in since this may make a network request
  // for more page content (e.g. video transcript).
  DCHECK(ai_chat_service_->HasUserOptedIn())
      << "UI shouldn't allow operations before user has accepted agreement";

  // Perf: make sure we're not doing this when the feature
  // won't be used (e.g. no active conversation).
  // DCHECK(is_conversation_active_)
  //     << "UI shouldn't allow operations for an inactive conversation";

  associated_content_delegate_->GetContent(
      base::BindOnce(&ConversationHandler::OnGeneratePageContentComplete,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void ConversationHandler::OnGeneratePageContentComplete(
    GetPageContentCallback callback,
    std::string contents_text,
    bool is_video,
    std::string invalidation_token) {
  engine_->SanitizeInput(contents_text);

  std::move(callback).Run(contents_text, is_video, invalidation_token);

  // Content-used percentage might have changed
  OnAssociatedContentInfoChanged();
}

void ConversationHandler::OnGetRefinedPageContent(
    const std::string& input,
    EngineConsumer::GenerationDataCallback data_received_callback,
    EngineConsumer::GenerationCompletedCallback data_completed_callback,
    std::string page_content,
    bool is_video,
    base::expected<std::string, std::string> refined_page_content) {
  std::string page_content_to_use = std::move(page_content);
  if (refined_page_content.has_value()) {
    page_content_to_use = std::move(refined_page_content.value());
    is_content_refined_ = true;
    OnAssociatedContentInfoChanged();
  } else {
    VLOG(1) << "Failed to get refined page content: "
            << refined_page_content.error();
    if (is_content_refined_) {
      is_content_refined_ = false;
      OnAssociatedContentInfoChanged();
    }
  }
  engine_->GenerateAssistantResponse(
      is_video, page_content_to_use, chat_history_, input,
      std::move(data_received_callback), std::move(data_completed_callback));
}

void ConversationHandler::OnEngineCompletionDataReceived(
    mojom::ConversationEntryEventPtr result) {
  UpdateOrCreateLastAssistantEntry(std::move(result));
}

void ConversationHandler::OnEngineCompletionComplete(
    EngineConsumer::GenerationResult result) {
  is_request_in_progress_ = false;

  if (result.has_value()) {
    // Handle success, which might mean do nothing much since all
    // data was passed in the streaming "received" callback.
    if (!result->empty()) {
      UpdateOrCreateLastAssistantEntry(
          mojom::ConversationEntryEvent::NewCompletionEvent(
              mojom::CompletionEvent::New(*result)));
    }
    MaybePopPendingRequests();
  } else {
    // handle failure
    SetAPIError(std::move(result.error()));
  }

  OnAPIRequestInProgressChanged();
}

void ConversationHandler::OnSuggestedQuestionsResponse(
    EngineConsumer::SuggestedQuestionResult result) {
  if (result.has_value()) {
    suggestions_.insert(suggestions_.end(), result->begin(), result->end());
    suggestion_generation_status_ =
        mojom::SuggestionGenerationStatus::HasGenerated;
  } else {
    // TODO(nullhook): Set a specialized error state generated questions
    suggestion_generation_status_ =
        mojom::SuggestionGenerationStatus::CanGenerate;
  }

  // Notify observers
  OnSuggestedQuestionsChanged();
  DVLOG(2) << "Got questions:" << base::JoinString(suggestions_, "\n");
}

void ConversationHandler::OnModelListUpdated() {
  OnModelDataChanged();

  const mojom::Model* model = model_service_->GetModel(model_key_);

  if (model && engine_) {
    engine_->UpdateModelOptions(*model->options);
  }
}

void ConversationHandler::OnDefaultModelChanged(const std::string& old_key,
                                                const std::string& new_key) {
  // When default model changes, change any conversation that
  // has that model.
  DVLOG(1) << "Default model changed from " << old_key << " to " << new_key;
  if (model_key_ == old_key) {
    ChangeModel(new_key);
  }
}

void ConversationHandler::OnModelRemoved(const std::string& removed_key) {
  // Any current model is removed, switch to default
  if (model_key_ == removed_key) {
    // TODO(nullhook): Inform the UI that the model has been removed, so it can
    // show a message
    model_key_ = model_service_->GetDefaultModelKey();
  }

  // Update the engine and fetch the new models
  InitEngine();
}

void ConversationHandler::OnModelDataChanged() {
  const std::vector<mojom::ModelPtr>& models = model_service_->GetModels();

  for (const mojo::Remote<mojom::ConversationUI>& client :
       conversation_ui_handlers_) {
    std::vector<mojom::ModelPtr> models_copy(models.size());
    std::transform(models.cbegin(), models.cend(), models_copy.begin(),
                   [](auto& model) { return model.Clone(); });
    client->OnModelDataChanged(model_key_, std::move(models_copy));
  }
}

void ConversationHandler::OnHistoryUpdate() {
  // TODO(petemill): Provide the updated converation history item so that
  // we don't need to clone every entry.
  for (auto& client : conversation_ui_handlers_) {
    client->OnConversationHistoryUpdate();
  }

  for (auto& observer : observers_) {
    // TODO(petemill): only tell observers about complete turns. This is
    // expensive to do for every event generated by in-progress turns,
    // and consumers likely only need complete ones (e.g. database save).
    std::vector<mojom::ConversationTurnPtr> history;
    for (const auto& turn : chat_history_) {
      history.emplace_back(turn->Clone());
    }
    observer.OnConversationEntriesChanged(this, std::move(history));
  }
}

int ConversationHandler::GetContentUsedPercentage() {
  CHECK(associated_content_delegate_);
  auto& model = GetCurrentModel();
  uint32_t max_page_content_length =
      model.options->is_custom_model_options()
          ? kCustomModelMaxPageContentLength
          : model.options->get_leo_model_options()->max_page_content_length;

  auto content_length =
      associated_content_delegate_->GetCachedTextContent().length();

  if (max_page_content_length > static_cast<uint32_t>(content_length)) {
    return 100;
  }

  // Convert to float to avoid integer division, which truncates towards zero
  // and could lead to inaccurate results before multiplication.
  float pct = static_cast<float>(max_page_content_length) /
              static_cast<float>(content_length) * 100;

  return base::ClampRound(pct);
}

bool ConversationHandler::IsContentAssociationPossible() {
  return (associated_content_delegate_ != nullptr);
}

void ConversationHandler::BuildAssociatedContentInfo() {
  // Save in class instance so that we have a cache for when live
  // AssociatedContentDelegate disconnects. Only modify in this function.
  associated_content_info_ = mojom::SiteInfo::New();
  if (associated_content_delegate_) {
    associated_content_info_->title =
        base::UTF16ToUTF8(associated_content_delegate_->GetTitle());
    const GURL url = associated_content_delegate_->GetURL();
    if (url.SchemeIsHTTPOrHTTPS()) {
      associated_content_info_->hostname = url.host();
      associated_content_info_->url = url;
    }
    associated_content_info_->content_used_percentage =
        GetContentUsedPercentage();
    associated_content_info_->is_content_refined = is_content_refined_;
    associated_content_info_->is_content_association_possible = true;
  } else {
    associated_content_info_->is_content_association_possible = false;
  }
}

void ConversationHandler::OnAssociatedContentInfoChanged() {
  BuildAssociatedContentInfo();
  for (auto& client : conversation_ui_handlers_) {
    client->OnAssociatedContentInfoChanged(associated_content_info_->Clone(),
                                           should_send_page_contents_);
  }
}

void ConversationHandler::OnClientConnectionChanged() {
  for (auto& observer : observers_) {
    observer.OnClientConnectionChanged(this);
  }
}

void ConversationHandler::OnAssociatedContentFaviconImageDataChanged() {
  for (auto& client : conversation_ui_handlers_) {
    client->OnFaviconImageDataChanged();
  }
}

void ConversationHandler::OnSuggestedQuestionsChanged() {
  for (auto& client : conversation_ui_handlers_) {
    client->OnSuggestedQuestionsChanged(suggestions_,
                                        suggestion_generation_status_);
  }
}

void ConversationHandler::OnAPIRequestInProgressChanged() {
  for (auto& client : conversation_ui_handlers_) {
    client->OnAPIRequestInProgress(is_request_in_progress_);
  }
}

}  // namespace ai_chat
