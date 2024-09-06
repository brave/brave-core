/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/conversation_driver.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/debug/crash_logging.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/notimplemented.h"
#include "base/notreached.h"
#include "base/one_shot_event.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/brave_search_responses.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_claude.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_conversation_api.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_llama.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer_oai.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "net/base/url_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/l10n/l10n_util.h"

using ai_chat::mojom::CharacterType;
using ai_chat::mojom::ConversationTurn;
using ai_chat::mojom::ConversationTurnVisibility;

namespace ai_chat {

namespace {

static const auto kAllowedSchemes = base::MakeFixedFlatSet<std::string_view>(
    {url::kHttpsScheme, url::kHttpScheme, url::kFileScheme, url::kDataScheme});

bool IsPremiumStatus(mojom::PremiumStatus status) {
  return status == mojom::PremiumStatus::Active ||
         status == mojom::PremiumStatus::ActiveDisconnected;
}

const base::flat_map<mojom::ActionType, std::string>&
GetActionTypeQuestionMap() {
  static const base::NoDestructor<
      base::flat_map<mojom::ActionType, std::string>>
      map({{mojom::ActionType::SUMMARIZE_PAGE,
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
  return *map;
}

const std::string& GetActionTypeQuestion(mojom::ActionType action_type) {
  const auto& map = GetActionTypeQuestionMap();
  auto iter = map.find(action_type);
  CHECK(iter != map.end());
  return iter->second;
}

net::NetworkTrafficAnnotationTag
GetSearchQuerySummaryNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat_tab_helper", R"(
      semantics {
        sender: "Brave Leo AI Chat"
        description:
          "This sender is used to get search query summary from Brave search."
        trigger:
          "Triggered by uses of Brave Leo AI Chat on Brave Search SERP."
        data:
          "User's search query and the corresponding summary."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace

ConversationDriver::ConversationDriver(
    PrefService* profile_prefs,
    PrefService* local_state_prefs,
    ModelService* model_service,
    AIChatMetrics* ai_chat_metrics,
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::string_view channel_string)
    : ConversationDriver(profile_prefs,
                         local_state_prefs,
                         model_service,
                         ai_chat_metrics,
                         std::make_unique<AIChatCredentialManager>(
                             std::move(skus_service_getter),
                             local_state_prefs),
                         url_loader_factory,
                         channel_string) {}

ConversationDriver::ConversationDriver(
    PrefService* profile_prefs,
    PrefService* local_state_prefs,
    ModelService* model_service,
    AIChatMetrics* ai_chat_metrics,
    std::unique_ptr<AIChatCredentialManager> credential_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::string_view channel_string)
    : pref_service_(profile_prefs),
      ai_chat_metrics_(ai_chat_metrics),
      credential_manager_(std::move(credential_manager)),
      feedback_api_(std::make_unique<ai_chat::AIChatFeedbackAPI>(
          url_loader_factory,
          std::string(channel_string))),
      url_loader_factory_(url_loader_factory),
      model_service_(model_service),
      on_page_text_fetch_complete_(new base::OneShotEvent()),
      text_embedder_(nullptr, base::OnTaskRunnerDeleter(nullptr)) {
  DCHECK(pref_service_);

  models_observer_.Observe(model_service_.get());

  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      prefs::kLastAcceptedDisclaimer,
      base::BindRepeating(&ConversationDriver::OnUserOptedIn,
                          weak_ptr_factory_.GetWeakPtr()));

  // Model choice names is selectable per conversation, not global.
  // Start with default from pref value if set. If user is premium and premium
  // model is different to non-premium default, and user hasn't customized the
  // model pref, then switch the user to the premium default.
  // TODO(petemill): When we have an event for premium status changed, and a
  // profile service for AIChat, then we can call
  // |pref_service_->SetDefaultPrefValue| when the user becomes premium. With
  // that, we'll be able to simply call GetString(prefs::kDefaultModelKey) and
  // not have to fetch premium status.
  const std::string& default_model_user_pref =
      model_service_->GetDefaultModelKey();
  if (!default_model_user_pref.empty() &&
      features::kAIModelsPremiumDefaultKey.Get() !=
          features::kAIModelsDefaultKey.Get()) {
    credential_manager_->GetPremiumStatus(base::BindOnce(
        [](ConversationDriver* instance, mojom::PremiumStatus status,
           mojom::PremiumInfoPtr) {
          instance->last_premium_status_ = status;
          if (!IsPremiumStatus(status)) {
            // Not premium
            return;
          }
          // Use default premium model for this instance
          instance->ChangeModel(features::kAIModelsPremiumDefaultKey.Get());
          // Make sure default model reflects premium status
          const auto& current_default =
              instance->model_service_->GetDefaultModelKey();

          if (current_default != features::kAIModelsPremiumDefaultKey.Get()) {
            instance->model_service_->SetDefaultModelKey(
                features::kAIModelsPremiumDefaultKey.Get());
          }
        },
        // Unretained is ok as credential manager is owned by this class,
        // and it owns the mojo binding that is used to make async call in
        // |GetPremiumStatus|.
        base::Unretained(this)));
  } else if (default_model_user_pref == "chat-claude-instant") {
    // 2024-05 Migration for old "claude instant" model
    // The migration is performed here instead of
    // ai_chat::prefs::MigrateProfilePrefs because the migration requires
    // knowing about premium status.
    credential_manager_->GetPremiumStatus(base::BindOnce(
        [](ConversationDriver* instance, mojom::PremiumStatus status,
           mojom::PremiumInfoPtr) {
          instance->last_premium_status_ = status;
          const std::string model_key = IsPremiumStatus(status)
                                            ? "chat-claude-sonnet"
                                            : "chat-claude-haiku";
          instance->model_service_->SetDefaultModelKey(model_key);
          instance->ChangeModel(model_key);
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
    model_key_ = model_service_->GetDefaultModelKey();
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

ConversationDriver::~ConversationDriver() {
  models_observer_.Reset();
}

void ConversationDriver::ChangeModel(const std::string& model_key) {
  DCHECK(!model_key.empty());
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

std::string ConversationDriver::GetDefaultModel() {
  return model_service_->GetDefaultModelKey();
}

void ConversationDriver::SetDefaultModel(const std::string& model_key) {
  DCHECK(!model_key.empty());
  // Check that the key exists
  auto* new_model = model_service_->GetModel(model_key);
  if (!new_model) {
    NOTREACHED_IN_MIGRATION()
        << "No matching model found for key: " << model_key;
    return;
  }

  model_service_->SetDefaultModelKey(model_key);
}

const mojom::Model& ConversationDriver::GetCurrentModel() const {
  const mojom::Model* model = model_service_->GetModel(model_key_);
  DCHECK(model);
  return *model;
}

uint32_t ConversationDriver::GetMaxPageContentLength() const {
  const auto& model = GetCurrentModel();
  return model.options->is_custom_model_options()
             ? kCustomModelMaxPageContentLength
             : model.options->get_leo_model_options()->max_page_content_length;
}

const std::vector<mojom::ModelPtr>& ConversationDriver::GetModels() {
  return model_service_->GetModels();
}

const std::vector<mojom::ConversationTurnPtr>&
ConversationDriver::GetConversationHistory() {
  return chat_history_;
}

std::vector<mojom::ConversationTurnPtr>
ConversationDriver::GetVisibleConversationHistory() {
  // Remove conversations that are meant to be hidden from the user
  std::vector<ai_chat::mojom::ConversationTurnPtr> list;
  for (const auto& turn : GetConversationHistory()) {
    if (turn->visibility != ConversationTurnVisibility::HIDDEN) {
      list.push_back(turn.Clone());
    }
  }
  if (pending_conversation_entry_ && pending_conversation_entry_->visibility !=
                                         ConversationTurnVisibility::HIDDEN) {
    list.push_back(pending_conversation_entry_->Clone());
  }
  return list;
}

void ConversationDriver::OnConversationActiveChanged(
    bool is_conversation_active) {
  if (is_conversation_active == is_conversation_active_) {
    return;
  }

  is_conversation_active_ = is_conversation_active;
  DVLOG(3) << "Conversation active changed: " << is_conversation_active;

  MaybeSeedOrClearSuggestions();
  MaybePopPendingRequests();
  MaybeFetchOrClearSearchQuerySummary();
}

void ConversationDriver::ClearSearchQuerySummary() {
  if (chat_history_.empty()) {
    return;
  }

  const auto& last_turn = chat_history_.back();
  if (last_turn->from_brave_search_SERP) {
    chat_history_.clear();  // Clear staged queries and answers.
    for (auto& obs : observers_) {
      obs.OnHistoryUpdate();
    }
  }
}

bool ConversationDriver::ShouldFetchSearchQuerySummary() {
  return HasUserOptedIn() && IsBraveSearchSERP(GetPageURL()) &&
         should_send_page_contents_;
}

void ConversationDriver::MaybeFetchOrClearSearchQuerySummary(
    FetchSearchQuerySummaryCallback callback) {
  // Only have search query summary if:
  // 1) user has opted in
  // 2) current page is a Brave Search SERP
  // 3) page content is linked
  // Clear existing search query summary if any of the requirements are not met.
  if (!ShouldFetchSearchQuerySummary()) {
    ClearSearchQuerySummary();
    std::move(callback).Run(std::nullopt);
    return;
  }

  // Existing search query summary will be used when conversation becomes
  // active again.
  if (!is_conversation_active_) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  // Currently only have search query summary at the start of a conversation.
  if (!chat_history_.empty()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  GetSearchSummarizerKey(
      base::BindOnce(&ConversationDriver::OnSearchSummarizerKeyFetched,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     current_navigation_id_));
}

void ConversationDriver::OnSearchSummarizerKeyFetched(
    FetchSearchQuerySummaryCallback callback,
    int64_t navigation_id,
    const std::optional<std::string>& key) {
  if (!key || key->empty() || navigation_id != current_navigation_id_ ||
      !chat_history_.empty()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  // Check if all requirements are still met.
  if (!ShouldFetchSearchQuerySummary()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  if (!api_request_helper_) {
    api_request_helper_ =
        std::make_unique<api_request_helper::APIRequestHelper>(
            GetSearchQuerySummaryNetworkTrafficAnnotationTag(),
            url_loader_factory_);
  }

  // https://search.brave.com/api/chatllm/raw_data?key=<key>
  GURL base_url(
      base::StrCat({url::kHttpsScheme, url::kStandardSchemeSeparator,
                    brave_domains::GetServicesDomain(kBraveSearchURLPrefix),
                    "/api/chatllm/raw_data"}));
  CHECK(base_url.is_valid());
  GURL url = net::AppendQueryParameter(base_url, "key", *key);

  api_request_helper_->Request(
      "GET", url, "", "application/json",
      base::BindOnce(&ConversationDriver::OnSearchQuerySummaryFetched,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     navigation_id),
      {}, {});
}

void ConversationDriver::OnSearchQuerySummaryFetched(
    FetchSearchQuerySummaryCallback callback,
    int64_t navigation_id,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode() || navigation_id != current_navigation_id_ ||
      !chat_history_.empty()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  // Check if all requirements are still met.
  if (!ShouldFetchSearchQuerySummary()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  auto entries = ParseSearchQuerySummaryResponse(result.value_body());
  if (!entries) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  // Add the query & summary pairs to the conversation history and call
  // OnHistoryUpdate to update UI.
  for (const auto& entry : *entries) {
    chat_history_.push_back(mojom::ConversationTurn::New(
        CharacterType::HUMAN, mojom::ActionType::QUERY,
        ConversationTurnVisibility::VISIBLE, entry.query, std::nullopt,
        std::nullopt, base::Time::Now(), std::nullopt, true));
    std::vector<mojom::ConversationEntryEventPtr> events;
    events.push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
        mojom::CompletionEvent::New(entry.summary)));
    chat_history_.push_back(mojom::ConversationTurn::New(
        CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
        ConversationTurnVisibility::VISIBLE, entry.summary, std::nullopt,
        std::move(events), base::Time::Now(), std::nullopt, true));
  }

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }

  std::move(callback).Run(entries);
}

void ConversationDriver::GetSearchSummarizerKey(
    mojom::PageContentExtractor::GetSearchSummarizerKeyCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(std::nullopt);
}

// static
std::optional<std::vector<SearchQuerySummary>>
ConversationDriver::ParseSearchQuerySummaryResponse(const base::Value& value) {
  auto search_query_response =
      brave_search_responses::QuerySummaryResponse::FromValue(value);
  if (!search_query_response || search_query_response->conversation.empty()) {
    return std::nullopt;
  }

  std::vector<SearchQuerySummary> entries;
  for (const auto& entry : search_query_response->conversation) {
    if (entry.answer.empty()) {
      continue;
    }

    // Only support one answer for each query for now.
    entries.push_back(SearchQuerySummary(entry.query, entry.answer[0].text));
  }

  return entries;
}

void ConversationDriver::InitEngine() {
  DCHECK(!model_key_.empty());
  const mojom::Model* model = model_service_->GetModel(model_key_);
  // Make sure we get a valid model, defaulting to static default or first.
  if (!model) {
    // It is unexpected that we get here. Dump a call stack
    // to help figure out why it happens.
    SCOPED_CRASH_KEY_STRING1024("BraveAIChatModel", "key", model_key_);
    base::debug::DumpWithoutCrashing();
    // Use default
    model = model_service_->GetModel(features::kAIModelsDefaultKey.Get());
    DCHECK(model);
    if (!model) {
      const auto& all_models = GetModels();
      // Use first if given bad default value
      model = all_models.at(0).get();
    }
  }

  // Model's key might not be the same as what we asked for (e.g. if the model
  // no longer exists).
  model_key_ = model->key;

  // Only LeoModels are passed to the following engines.
  if (model->options->is_leo_model_options()) {
    auto& leo_model_opts = model->options->get_leo_model_options();

    // Engine enum on model to decide which one
    if (leo_model_opts->engine_type ==
        mojom::ModelEngineType::BRAVE_CONVERSATION_API) {
      DVLOG(1) << "Started AI engine: conversation api";
      engine_ = std::make_unique<EngineConsumerConversationAPI>(
          *leo_model_opts, url_loader_factory_, credential_manager_.get());
    } else if (leo_model_opts->engine_type ==
               mojom::ModelEngineType::LLAMA_REMOTE) {
      DVLOG(1) << "Started AI engine: llama";
      engine_ = std::make_unique<EngineConsumerLlamaRemote>(
          *leo_model_opts, url_loader_factory_, credential_manager_.get());
    } else {
      DVLOG(1) << "Started AI engine: claude";
      engine_ = std::make_unique<EngineConsumerClaudeRemote>(
          *leo_model_opts, url_loader_factory_, credential_manager_.get());
    }
  }

  if (model->options->is_custom_model_options()) {
    auto& custom_model_opts = model->options->get_custom_model_options();
    DVLOG(1) << "Started AI engine: custom";
    engine_ = std::make_unique<EngineConsumerOAIRemote>(*custom_model_opts,
                                                        url_loader_factory_);
  }

  // Pending requests have been deleted along with the model engine
  is_request_in_progress_ = false;
  for (auto& obs : observers_) {
    obs.OnModelDataChanged(model_key_, GetModels());
    obs.OnAPIRequestInProgress(false);
  }

  // When the model changes, the content truncation might be different,
  // and the UI needs to know.
  if (!article_text_.empty()) {
    OnPageHasContentChanged(BuildSiteInfo());
  }
}

bool ConversationDriver::HasUserOptedIn() {
  return ::ai_chat::HasUserOptedIn(pref_service_);
}

void ConversationDriver::SetUserOptedIn(bool user_opted_in) {
  ::ai_chat::SetUserOptedIn(pref_service_, user_opted_in);
}

void ConversationDriver::OnUserOptedIn() {
  MaybePopPendingRequests();
  MaybeFetchOrClearSearchQuerySummary();

  if (ai_chat_metrics_ != nullptr && HasUserOptedIn()) {
    ai_chat_metrics_->RecordEnabled(true, true, {});
  }
}

void ConversationDriver::AddToConversationHistory(
    mojom::ConversationTurnPtr turn) {
  if (!turn) {
    return;
  }

  if (ai_chat_metrics_ != nullptr) {
    if (chat_history_.size() == 0) {
      ai_chat_metrics_->RecordNewChat();
    }
    if (turn->character_type == CharacterType::HUMAN) {
      ai_chat_metrics_->RecordNewPrompt();
    }
  }

  chat_history_.push_back(std::move(turn));

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }
}

void ConversationDriver::UpdateOrCreateLastAssistantEntry(
    mojom::ConversationEntryEventPtr event) {
  if (chat_history_.empty() ||
      chat_history_.back()->character_type != CharacterType::ASSISTANT) {
    mojom::ConversationTurnPtr entry = mojom::ConversationTurn::New(
        CharacterType::ASSISTANT, mojom::ActionType::RESPONSE,
        ConversationTurnVisibility::VISIBLE, "", std::nullopt,
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
  if (is_page_text_fetch_in_progress_) {
    return false;
  }

  mojom::ConversationTurnPtr request = std::move(pending_conversation_entry_);
  pending_conversation_entry_.reset();
  SubmitHumanConversationEntry(std::move(request));
  return true;
}

void ConversationDriver::MaybeSeedOrClearSuggestions() {
  if (!is_conversation_active_) {
    return;
  }

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
          is_video_ ? l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_VIDEO)
                    : l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE));
    }
    suggestion_generation_status_ =
        mojom::SuggestionGenerationStatus::CanGenerate;
    OnSuggestedQuestionsChanged();
  }
}

void ConversationDriver::GeneratePageContent(GetPageContentCallback callback) {
  VLOG(1) << __func__;
  DCHECK(should_send_page_contents_);
  DCHECK(IsContentAssociationPossible())
      << "Shouldn't have been asked to generate page text when "
      << "|IsContentAssociationPossible()| is false.";
  DCHECK(!is_page_text_fetch_in_progress_)
      << "UI shouldn't allow multiple operations at the same time";

  // Make sure user is opted in since this may make a network request
  // for more page content (e.g. video transcript).
  DCHECK(HasUserOptedIn())
      << "UI shouldn't allow operations before user has accepted agreement";

  // Perf: make sure we're not doing this when the feature
  // won't be used (e.g. no active conversation).
  DCHECK(is_conversation_active_)
      << "UI shouldn't allow operations for an inactive conversation";

  // Only perform a fetch once at a time, and then use the results from
  // an in-progress operation.
  if (is_page_text_fetch_in_progress_) {
    VLOG(1) << "A page content fetch is in progress, waiting for the existing "
               "operation to complete";
    auto handle_existing_fetch_complete = base::BindOnce(
        &ConversationDriver::OnExistingGeneratePageContentComplete,
        weak_ptr_factory_.GetWeakPtr(), std::move(callback));
    on_page_text_fetch_complete_->Post(
        FROM_HERE, std::move(handle_existing_fetch_complete));
    return;
  }

  is_page_text_fetch_in_progress_ = true;
  // Update fetching status
  OnPageHasContentChanged(BuildSiteInfo());

  GetPageContent(
      base::BindOnce(&ConversationDriver::OnGeneratePageContentComplete,
                     weak_ptr_factory_.GetWeakPtr(), current_navigation_id_,
                     std::move(callback)),
      content_invalidation_token_);
}

void ConversationDriver::OnGeneratePageContentComplete(
    int64_t navigation_id,
    GetPageContentCallback callback,
    std::string contents_text,
    bool is_video,
    std::string invalidation_token) {
  DVLOG(1) << "OnGeneratePageContentComplete";
  DVLOG(4) << "Contents(is_video=" << is_video
           << ", invalidation_token=" << invalidation_token
           << "): " << contents_text;
  if (navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }

  // Ignore if we received content from observer in the meantime
  if (!is_page_text_fetch_in_progress_) {
    DVLOG(1) << __func__
             << " but already received contents from observer. Ignoring.";
    return;
  }

  if (base::CollapseWhitespaceASCII(contents_text, true).empty() &&
      !is_print_preview_fallback_requested_ && !is_video &&
      // Don't fallback again for failed print preview retrieval.
      !base::Contains(kPrintPreviewRetrievalHosts, GetPageURL().host_piece())) {
    DVLOG(1) << "Initiating print preview fallback";
    is_print_preview_fallback_requested_ = true;
    PrintPreviewFallback(
        base::BindOnce(&ConversationDriver::OnGeneratePageContentComplete,
                       weak_ptr_factory_.GetWeakPtr(), current_navigation_id_,
                       std::move(callback)));
    return;
  }
  is_print_preview_fallback_requested_ = false;

  OnPageContentUpdated(contents_text, is_video, invalidation_token);

  std::move(callback).Run(article_text_, is_video_,
                          content_invalidation_token_);
}

void ConversationDriver::OnExistingGeneratePageContentComplete(
    GetPageContentCallback callback) {
  // Don't need to check navigation ID since existing event will be
  // deleted when there's a new conversation.
  DVLOG(1) << "Existing page content fetch completed, proceeding with "
              "the results of that operation.";
  std::move(callback).Run(article_text_, is_video_,
                          content_invalidation_token_);
}

void ConversationDriver::OnPageContentUpdated(std::string contents_text,
                                              bool is_video,
                                              std::string invalidation_token) {
  is_page_text_fetch_in_progress_ = false;
  // If invalidation token matches existing token, then
  // content was not re-fetched and we can use our existing cache.
  if (!invalidation_token.empty() &&
      (invalidation_token == content_invalidation_token_)) {
    contents_text = article_text_;
  } else {
    is_video_ = is_video;
    // Cache page content on instance so we don't always have to re-fetch
    // if the content fetcher knows the content won't have changed and the fetch
    // operation is expensive (e.g. network).
    article_text_ = contents_text;
    content_invalidation_token_ = invalidation_token;
    engine_->SanitizeInput(article_text_);
    // Update completion status
    OnPageHasContentChanged(BuildSiteInfo());
  }

  if (contents_text.empty()) {
    VLOG(1) << __func__ << ": No data";
  }

  on_page_text_fetch_complete_->Signal();
  on_page_text_fetch_complete_ = std::make_unique<base::OneShotEvent>();
}

void ConversationDriver::OnNewPage(int64_t navigation_id) {
  current_navigation_id_ = navigation_id;
  CleanUp();
}

void ConversationDriver::NotifyPrintPreviewRequested(bool is_pdf) {
  for (auto& obs : observers_) {
    obs.OnPrintPreviewRequested(is_pdf);
  }
}

void ConversationDriver::CleanUp() {
  DVLOG(1) << __func__;
  chat_history_.clear();
  article_text_.clear();
  is_content_refined_ = false;
  content_invalidation_token_.clear();
  on_page_text_fetch_complete_ = std::make_unique<base::OneShotEvent>();
  is_video_ = false;
  suggestions_.clear();
  pending_conversation_entry_.reset();
  is_page_text_fetch_in_progress_ = false;
  is_print_preview_fallback_requested_ = false;
  is_request_in_progress_ = false;
  suggestion_generation_status_ = mojom::SuggestionGenerationStatus::None;
  should_send_page_contents_ = true;
  OnSuggestedQuestionsChanged();
  SetAPIError(mojom::APIError::None);
  engine_->ClearAllQueries();
  if (text_embedder_) {
    text_embedder_->CancelAllTasks();
    text_embedder_.reset();
  }
  api_request_helper_.reset();

  MaybeSeedOrClearSuggestions();
  MaybeFetchOrClearSearchQuerySummary();

  // Trigger an observer update to refresh the UI.
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
    obs.OnAPIRequestInProgress(false);
    obs.OnPageHasContent(BuildSiteInfo());
  }
}

void ConversationDriver::OnModelListUpdated() {
  for (auto& obs : observers_) {
    obs.OnModelDataChanged(model_key_, GetModels());
  }

  const mojom::Model* model = model_service_->GetModel(model_key_);

  if (model && engine_) {
    engine_->UpdateModelOptions(*model->options);
  }
}

void ConversationDriver::OnModelRemoved(const std::string& removed_key) {
  // Any current model is removed, switch to default
  if (model_key_ == removed_key) {
    // TODO(nullhook): Inform the UI that the model has been removed, so it can
    // show a message
    model_key_ = features::kAIModelsDefaultKey.Get();
  }

  // Update the engine and fetch the new models
  InitEngine();
}

std::vector<std::string> ConversationDriver::GetSuggestedQuestions(
    mojom::SuggestionGenerationStatus& suggestion_status) {
  // Can we get suggested questions
  suggestion_status = suggestion_generation_status_;
  return suggestions_;
}

void ConversationDriver::SetShouldSendPageContents(bool should_send) {
  if (should_send_page_contents_ == should_send) {
    return;
  }
  should_send_page_contents_ = should_send;

  MaybeSeedOrClearSuggestions();
  MaybeFetchOrClearSearchQuerySummary();
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

mojom::ConversationTurnPtr ConversationDriver::ClearErrorAndGetFailedMessage() {
  DCHECK(!chat_history_.empty());

  SetAPIError(mojom::APIError::None);
  mojom::ConversationTurnPtr turn = chat_history_.back().Clone();
  chat_history_.pop_back();

  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }

  return turn;
}

void ConversationDriver::GenerateQuestions() {
  DVLOG(1) << __func__;
  // This function should not be presented in the UI if the user has not
  // opted-in yet.
  if (!HasUserOptedIn()) {
    NOTREACHED_IN_MIGRATION()
        << "GenerateQuestions should not be called before user is "
        << "opted in to AI Chat";
    return;
  }
  DCHECK(should_send_page_contents_)
      << "Cannot get suggestions when not associated with content.";
  DCHECK(IsContentAssociationPossible())
      << "Should not be associated with content when not allowed to be";
  // We're not expecting to call this if the UI is not active for this
  // conversation.
  DCHECK(is_conversation_active_);
  // We're not expecting to already have generated suggestions
  DCHECK_LE(suggestions_.size(), 1u);

  if (suggestion_generation_status_ ==
          mojom::SuggestionGenerationStatus::IsGenerating ||
      suggestion_generation_status_ ==
          mojom::SuggestionGenerationStatus::HasGenerated) {
    NOTREACHED_IN_MIGRATION()
        << "UI should not allow GenerateQuestions to be called more "
        << "than once";
    return;
  }

  suggestion_generation_status_ =
      mojom::SuggestionGenerationStatus::IsGenerating;
  OnSuggestedQuestionsChanged();
  // Make API request for questions but first get page content.
  // Do not call SetRequestInProgress, this progress
  // does not need to be shown to the UI.
  auto on_content_retrieved = [](ConversationDriver* instance,
                                 int64_t navigation_id,
                                 std::string page_content, bool is_video,
                                 std::string invalidation_token) {
    instance->engine_->GenerateQuestionSuggestions(
        is_video, page_content,
        base::BindOnce(&ConversationDriver::OnSuggestedQuestionsResponse,
                       instance->weak_ptr_factory_.GetWeakPtr(),
                       std::move(navigation_id)));
  };
  GeneratePageContent(base::BindOnce(std::move(on_content_retrieved),
                                     base::Unretained(this),
                                     current_navigation_id_));
}

void ConversationDriver::OnSuggestedQuestionsResponse(
    int64_t navigation_id,
    EngineConsumer::SuggestedQuestionResult result) {
  // We might have navigated away whilst this async operation is in
  // progress, so check if we're the same navigation.
  if (navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }

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

void ConversationDriver::MaybeUnlinkPageContent() {
  // Only unlink if panel is closed and there is no conversation history.
  // When panel is open or has existing conversation, do not change the state.
  if (!is_conversation_active_ && chat_history_.empty()) {
    SetShouldSendPageContents(false);
  }
}

void ConversationDriver::AddSubmitSelectedTextError(
    const std::string& selected_text,
    mojom::ActionType action_type,
    mojom::APIError error) {
  if (error == mojom::APIError::None) {
    return;
  }
  const std::string& question = GetActionTypeQuestion(action_type);
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      CharacterType::HUMAN, action_type, ConversationTurnVisibility::VISIBLE,
      question, selected_text, std::nullopt, base::Time::Now(), std::nullopt,
      false);
  AddToConversationHistory(std::move(turn));
  SetAPIError(error);
}

void ConversationDriver::SubmitSelectedText(
    const std::string& selected_text,
    mojom::ActionType action_type,
    GeneratedTextCallback received_callback,
    EngineConsumer::GenerationCompletedCallback completed_callback) {
  const std::string& question = GetActionTypeQuestion(action_type);
  SubmitSelectedTextWithQuestion(selected_text, question, action_type,
                                 std::move(received_callback),
                                 std::move(completed_callback));
}

void ConversationDriver::SubmitSelectedTextWithQuestion(
    const std::string& selected_text,
    const std::string& question,
    mojom::ActionType action_type,
    GeneratedTextCallback received_callback,
    EngineConsumer::GenerationCompletedCallback completed_callback) {
  if (received_callback && completed_callback) {
    // Start a one-off request and replace in-place with the result.
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
        CharacterType::HUMAN, action_type, ConversationTurnVisibility::VISIBLE,
        question, selected_text, std::nullopt, base::Time::Now(), std::nullopt,
        false);

    SubmitHumanConversationEntry(std::move(turn));
  } else {
    NOTREACHED_NORETURN() << "Both callbacks must be set or unset";
  }
}

void ConversationDriver::SubmitHumanConversationEntry(
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
  if (!is_conversation_active_ || !HasUserOptedIn() ||
      is_request_in_progress_) {
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
    for (auto& obs : observers_) {
      obs.OnHistoryUpdate();
    }
    return;
  }

  DCHECK(latest_turn->character_type == CharacterType::HUMAN);

  is_request_in_progress_ = true;
  for (auto& obs : observers_) {
    obs.OnAPIRequestInProgress(IsRequestInProgress());
  }

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
    GeneratePageContent(base::BindOnce(
        &ConversationDriver::PerformAssistantGeneration,
        weak_ptr_factory_.GetWeakPtr(), question_part, current_navigation_id_));
  } else {
    // Now the conversation is committed, we can remove some unneccessary data
    // if we're not associated with a page.
    article_text_.clear();
    suggestions_.clear();
    OnSuggestedQuestionsChanged();
    // Perform generation immediately
    PerformAssistantGeneration(question_part, current_navigation_id_);
  }
}

void ConversationDriver::PerformAssistantGeneration(
    const std::string& input,
    int64_t current_navigation_id,
    std::string page_content,
    bool is_video,
    std::string invalidation_token) {
  auto data_received_callback = base::BindRepeating(
      &ConversationDriver::OnEngineCompletionDataReceived,
      weak_ptr_factory_.GetWeakPtr(), current_navigation_id);

  auto data_completed_callback =
      base::BindOnce(&ConversationDriver::OnEngineCompletionComplete,
                     weak_ptr_factory_.GetWeakPtr(), current_navigation_id);
  bool should_refine_page_content =
      features::IsPageContentRefineEnabled() &&
      page_content.length() > GetMaxPageContentLength() &&
      input != l10n_util::GetStringUTF8(IDS_AI_CHAT_QUESTION_SUMMARIZE_PAGE);
  if (!text_embedder_ && should_refine_page_content) {
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
  }
  if (text_embedder_ && should_refine_page_content) {
    if (text_embedder_->IsInitialized()) {
      text_embedder_->GetTopSimilarityWithPromptTilContextLimit(
          input, page_content, GetMaxPageContentLength(),
          base::BindOnce(&ConversationDriver::OnGetRefinedPageContent,
                         weak_ptr_factory_.GetWeakPtr(), input,
                         std::move(data_received_callback),
                         std::move(data_completed_callback), page_content,
                         is_video));
    } else {
      text_embedder_->Initialize(base::BindOnce(
          &ConversationDriver::OnTextEmbedderInitialized,
          weak_ptr_factory_.GetWeakPtr(), input,
          std::move(data_received_callback), std::move(data_completed_callback),
          std::move(page_content), is_video));
    }
  } else {
    engine_->GenerateAssistantResponse(is_video, page_content, chat_history_,
                                       input, std::move(data_received_callback),
                                       std::move(data_completed_callback));
  }
}

void ConversationDriver::OnTextEmbedderInitialized(
    const std::string& input,
    EngineConsumer::GenerationDataCallback data_received_callback,
    EngineConsumer::GenerationCompletedCallback data_completed_callback,
    std::string page_content,
    bool is_video,
    bool initialized) {
  if (initialized) {
    text_embedder_->GetTopSimilarityWithPromptTilContextLimit(
        input, page_content, GetMaxPageContentLength(),
        base::BindOnce(&ConversationDriver::OnGetRefinedPageContent,
                       weak_ptr_factory_.GetWeakPtr(), input,
                       std::move(data_received_callback),
                       std::move(data_completed_callback), page_content,
                       is_video));
  } else {
    VLOG(1) << "Failed to initialize TextEmbedder";
    engine_->GenerateAssistantResponse(
        is_video, std::move(page_content), chat_history_, input,
        std::move(data_received_callback), std::move(data_completed_callback));
  }
}
void ConversationDriver::OnGetRefinedPageContent(
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
    OnPageHasContentChanged(BuildSiteInfo());
  } else {
    VLOG(1) << "Failed to get refined page content: "
            << refined_page_content.error();
  }
  engine_->GenerateAssistantResponse(
      is_video, page_content_to_use, chat_history_, input,
      std::move(data_received_callback), std::move(data_completed_callback));
}

void ConversationDriver::RetryAPIRequest() {
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

bool ConversationDriver::IsRequestInProgress() {
  return is_request_in_progress_;
}

void ConversationDriver::OnEngineCompletionDataReceived(
    int64_t navigation_id,
    mojom::ConversationEntryEventPtr result) {
  if (navigation_id != current_navigation_id_) {
    VLOG(1) << __func__ << " for a different navigation. Ignoring.";
    return;
  }

  UpdateOrCreateLastAssistantEntry(std::move(result));

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
      UpdateOrCreateLastAssistantEntry(
          mojom::ConversationEntryEvent::NewCompletionEvent(
              mojom::CompletionEvent::New(*result)));
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
  return !pending_conversation_entry_.is_null();
}

int ConversationDriver::GetContentUsedPercentage() {
  const auto max_page_content_length = GetMaxPageContentLength();

  if (max_page_content_length > static_cast<uint32_t>(article_text_.length())) {
    return 100;
  }

  // Convert to float to avoid integer division, which truncates towards zero
  // and could lead to inaccurate results before multiplication.
  float pct = static_cast<float>(max_page_content_length) /
              static_cast<float>(article_text_.length()) * 100;

  return base::ClampRound(pct);
}

void ConversationDriver::SubmitSummarizationRequest() {
  DCHECK(IsContentAssociationPossible())
      << "This conversation request is not associated with content\n";
  DCHECK(should_send_page_contents_)
      << "This conversation request should send page contents\n";

  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      CharacterType::HUMAN, mojom::ActionType::SUMMARIZE_PAGE,
      ConversationTurnVisibility::VISIBLE,
      l10n_util::GetStringUTF8(IDS_CHAT_UI_SUMMARIZE_PAGE), std::nullopt,
      std::nullopt, base::Time::Now(), std::nullopt, false);
  SubmitHumanConversationEntry(std::move(turn));
}

mojom::SiteInfoPtr ConversationDriver::BuildSiteInfo() {
  mojom::SiteInfoPtr site_info = mojom::SiteInfo::New();
  site_info->title = base::UTF16ToUTF8(GetPageTitle());
  site_info->content_used_percentage = GetContentUsedPercentage();
  site_info->is_content_association_possible = IsContentAssociationPossible();
  site_info->is_content_refined = is_content_refined_;
  const GURL url = GetPageURL();

  if (url.SchemeIsHTTPOrHTTPS()) {
    site_info->hostname = url.host();
  }

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
  // Maybe switch to premium model when user is newly premium and on a basic
  // model
  const auto& model = GetCurrentModel();

  const bool should_switch_model =
      model.options->is_leo_model_options() &&
      features::kFreemiumAvailable.Get() &&
      // This isn't the first retrieval (that's handled in the constructor)
      last_premium_status_ != mojom::PremiumStatus::Unknown &&
      last_premium_status_ != premium_status &&
      premium_status == mojom::PremiumStatus::Active &&
      model.options->get_leo_model_options()->access ==
          mojom::ModelAccess::BASIC;

  if (should_switch_model) {
    ChangeModel(features::kAIModelsPremiumDefaultKey.Get());
  }

  last_premium_status_ = premium_status;
  if (HasUserOptedIn()) {
    ai_chat_metrics_->OnPremiumStatusUpdated(false, premium_status,
                                             std::move(premium_info));
  }
  std::move(parent_callback).Run(premium_status, std::move(premium_info));
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
  auto& model = GetCurrentModel();

  // We only allow Leo models to be rated.
  CHECK(model.options->is_leo_model_options());

  const std::vector<mojom::ConversationTurnPtr>& history =
      GetConversationHistory();

  auto on_complete = base::BindOnce(
      [](mojom::PageHandler::RateMessageCallback callback,
         APIRequestResult result) {
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
        is_liked, IsPremiumStatus(last_premium_status_), history_slice,
        model.options->get_leo_model_options()->name, std::move(on_complete));

    return;
  }

  std::move(callback).Run(std::nullopt);
}

void ConversationDriver::SendFeedback(
    const std::string& category,
    const std::string& feedback,
    const std::string& rating_id,
    bool send_hostname,
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

  const GURL page_url = GetPageURL();

  feedback_api_->SendFeedback(category, feedback, rating_id,
                              (send_hostname && page_url.SchemeIsHTTPOrHTTPS())
                                  ? std::optional<std::string>(page_url.host())
                                  : std::nullopt,
                              std::move(on_complete));
}

void ConversationDriver::ModifyConversation(uint32_t turn_index,
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
        base::Time::Now(), std::nullopt /* edits */,
        turn->from_brave_search_SERP);
    edited_turn->events->at(*completion_event_index)
        ->get_completion_event()
        ->completion = trimmed_input;

    if (!turn->edits) {
      turn->edits.emplace();
    }
    turn->edits->emplace_back(std::move(edited_turn));

    for (auto& obs : observers_) {
      obs.OnHistoryUpdate();
    }

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
      turn->from_brave_search_SERP);
  if (!turn->edits) {
    turn->edits.emplace();
  }
  turn->edits->emplace_back(std::move(edited_turn));

  auto new_turn = std::move(chat_history_.at(turn_index));
  chat_history_.erase(chat_history_.begin() + turn_index, chat_history_.end());
  for (auto& obs : observers_) {
    obs.OnHistoryUpdate();
  }

  SubmitHumanConversationEntry(std::move(new_turn));
}

}  // namespace ai_chat
