// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_HANDLER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_HANDLER_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "base/scoped_observation.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/types.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class AIChatUIBrowserTest;
namespace mojo {
template <typename Interface>
class PendingRemote;
template <typename T>
class PendingReceiver;
}  // namespace mojo

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

class AIChatFeedbackAPI;
class AIChatService;
class AIChatCredentialManager;
class AssociatedContentManager;

// Performs all conversation-related operations, responsible for sending
// messages to the conversation engine, handling the responses, and owning
// the in-memory conversation history.
class ConversationHandler : public mojom::ConversationHandler,
                            public mojom::UntrustedConversationHandler,
                            public ModelService::Observer,
                            public ConversationHandlerForMetrics {
 public:
  using GeneratedTextCallback =
      base::RepeatingCallback<void(const std::string& text)>;

  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    // Called when the conversation history changess
    virtual void OnRequestInProgressChanged(ConversationHandler* handler,
                                            bool in_progress) {}
    virtual void OnConversationEntryAdded(
        ConversationHandler* handler,
        mojom::ConversationTurnPtr& entry,
        std::optional<std::vector<std::string_view>> associated_content_value) {
    }
    virtual void OnConversationEntryRemoved(ConversationHandler* handler,
                                            std::string turn_uuid) {}

    // Called when a mojo client connects or disconnects
    virtual void OnClientConnectionChanged(ConversationHandler* handler) {}
    virtual void OnConversationTitleChanged(
        const std::string& conversation_uuid,
        const std::string& title) {}
    virtual void OnConversationTokenInfoChanged(
        const std::string& conversation_uuid,
        uint64_t total_tokens,
        uint64_t trimmed_tokens) {}
    virtual void OnSelectedLanguageChanged(
        ConversationHandler* handler,
        const std::string& selected_language) {}
    virtual void OnAssociatedContentUpdated(ConversationHandler* handler) {}
  };

  ConversationHandler(
      mojom::Conversation* conversation,
      AIChatService* ai_chat_service,
      ModelService* model_service,
      AIChatCredentialManager* credential_manager,
      AIChatFeedbackAPI* feedback_api,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  ConversationHandler(
      mojom::Conversation* conversation,
      AIChatService* ai_chat_service,
      ModelService* model_service,
      AIChatCredentialManager* credential_manager,
      AIChatFeedbackAPI* feedback_api,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::optional<mojom::ConversationArchivePtr> initial_state);

  ~ConversationHandler() override;
  ConversationHandler(const ConversationHandler&) = delete;
  ConversationHandler& operator=(const ConversationHandler&) = delete;

  void Bind(mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler);
  void Bind(mojo::PendingReceiver<mojom::ConversationHandler> receiver,
            mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler);
  void Bind(
      mojo::PendingReceiver<mojom::UntrustedConversationHandler> receiver);
  void BindUntrustedConversationUI(
      mojo::PendingRemote<mojom::UntrustedConversationUI>
          untrusted_conversation_ui_handler,
      BindUntrustedConversationUICallback callback) override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void OnArchiveContentUpdated(mojom::ConversationArchivePtr conversation_data);

  bool IsAnyClientConnected();
  bool HasAnyHistory();
  bool IsRequestInProgress();

  // Returns true if the conversation has associated content that is non-archive
  bool IsAssociatedContentAlive();

  const mojom::Model& GetCurrentModel();
  const std::vector<mojom::ConversationTurnPtr>& GetConversationHistory() const;

  // mojom::ConversationHandler
  void GetState(GetStateCallback callback) override;
  void GetConversationHistory(GetConversationHistoryCallback callback) override;
  void SetTemporary(bool temporary) override;
  void RateMessage(bool is_liked,
                   const std::string& turn_uuid,
                   RateMessageCallback callback) override;
  void SendFeedback(const std::string& category,
                    const std::string& feedback,
                    const std::string& rating_id,
                    bool send_hostname,
                    SendFeedbackCallback callback) override;
  void GetConversationUuid(GetConversationUuidCallback) override;
  void GetModels(GetModelsCallback callback) override;
  void ChangeModel(const std::string& model_key) override;
  void GetIsRequestInProgress(GetIsRequestInProgressCallback callback) override;
  void SubmitHumanConversationEntry(
      const std::string& input,
      std::optional<std::vector<mojom::UploadedFilePtr>> uploaded_files)
      override;
  void SubmitHumanConversationEntry(mojom::ConversationTurnPtr turn);
  void SubmitHumanConversationEntryWithAction(
      const std::string& input,
      mojom::ActionType action_type) override;
  void ModifyConversation(uint32_t turn_index,
                          const std::string& new_text) override;
  void RegenerateAnswer(const std::string& turn_uuid,
                        const std::string& model_key) override;
  void SubmitSummarizationRequest() override;
  void SubmitSuggestion(const std::string& suggestion_title) override;
  std::vector<std::string> GetSuggestedQuestionsForTest();
  void SetSuggestedQuestionForTest(std::string title, std::string prompt);
  void GenerateQuestions() override;
  void GetAssociatedContentInfo(
      GetAssociatedContentInfoCallback callback) override;
  void SetShouldSendPageContents(bool should_send) override;
  void RetryAPIRequest() override;
  void GetAPIResponseError(GetAPIResponseErrorCallback callback) override;
  void ClearErrorAndGetFailedMessage(
      ClearErrorAndGetFailedMessageCallback callback) override;
  void StopGenerationAndMaybeGetHumanEntry(
      StopGenerationAndMaybeGetHumanEntryCallback callback) override;

  void SubmitSelectedText(const std::string& selected_text,
                          mojom::ActionType action_type);
  void SubmitSelectedTextWithQuestion(const std::string& selected_text,
                                      const std::string& question,
                                      mojom::ActionType action_type);
  bool MaybePopPendingRequests();
  void MaybeUnlinkAssociatedContent();
  void AddSubmitSelectedTextError(const std::string& selected_text,
                                  mojom::ActionType action_type,
                                  mojom::APIError error);
  void OnAssociatedContentUpdated();

  void OnUserOptedIn();
  size_t GetConversationHistorySize() override;
  void GetScreenshots(GetScreenshotsCallback callback) override;

  // Some associated content may provide some conversation that the user wants
  // to continue, e.g. Brave Search.
  void MaybeFetchOrClearContentStagedConversation();

  base::WeakPtr<ConversationHandler> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  bool GetIsTemporary() const { return metadata_->temporary; }

  std::string get_conversation_uuid() const { return metadata_->uuid; }

  bool should_send_page_contents() const override;

  mojom::APIError current_error() const override;

  void SetEngineForTesting(std::unique_ptr<EngineConsumer> engine_for_testing) {
    engine_ = std::move(engine_for_testing);
  }
  EngineConsumer* GetEngineForTesting() { return engine_.get(); }

  void SetChatHistoryForTesting(
      std::vector<mojom::ConversationTurnPtr> history) {
    chat_history_ = std::move(history);
    for (auto& entry : chat_history_) {
      OnConversationEntryAdded(entry);
    }
  }

  AssociatedContentManager* associated_content_manager() {
    return associated_content_manager_.get();
  }

  void SetRequestInProgressForTesting(bool in_progress) {
    is_request_in_progress_ = in_progress;
  }

 protected:
  // ModelService::Observer
  void OnModelListUpdated() override;
  void OnDefaultModelChanged(const std::string& old_key,
                             const std::string& new_key) override;
  void OnModelRemoved(const std::string& removed_key) override;

 private:
  friend class ::AIChatUIBrowserTest;
  FRIEND_TEST_ALL_PREFIXES(AIChatServiceUnitTest, DeleteAssociatedWebContent);
  FRIEND_TEST_ALL_PREFIXES(ConversationHandlerUnitTest,
                           UpdateOrCreateLastAssistantEntry_Delta);
  FRIEND_TEST_ALL_PREFIXES(ConversationHandlerUnitTest,
                           UpdateOrCreateLastAssistantEntry_DeltaWithSearch);
  FRIEND_TEST_ALL_PREFIXES(ConversationHandlerUnitTest,
                           UpdateOrCreateLastAssistantEntry_NotDelta);
  FRIEND_TEST_ALL_PREFIXES(ConversationHandlerUnitTest,
                           UpdateOrCreateLastAssistantEntry_NotDeltaWithSearch);
  FRIEND_TEST_ALL_PREFIXES(ConversationHandlerUnitTest,
                           OnGetStagedEntriesFromContent);
  FRIEND_TEST_ALL_PREFIXES(ConversationHandlerUnitTest,
                           OnGetStagedEntriesFromContent_FailedChecks);
  FRIEND_TEST_ALL_PREFIXES(ConversationHandlerUnitTest_NoAssociatedContent,
                           SelectedLanguage);
  FRIEND_TEST_ALL_PREFIXES(ConversationHandlerUnitTest_NoAssociatedContent,
                           ContentReceipt);

  struct Suggestion {
    std::string title;
    std::optional<std::string> prompt;
    mojom::ActionType action_type = mojom::ActionType::SUGGESTION;

    explicit Suggestion(std::string title);
    Suggestion(std::string title, std::string prompt);
    Suggestion(std::string title,
               std::string prompt,
               mojom::ActionType action_type);
    Suggestion(const Suggestion&) = delete;
    Suggestion& operator=(const Suggestion&) = delete;
    Suggestion(Suggestion&&);
    Suggestion& operator=(Suggestion&&);
    ~Suggestion();
  };

  void InitEngine();
  void UpdateAssociatedContentInfo();
  mojom::ConversationEntriesStatePtr GetStateForConversationEntries();
  void AddToConversationHistory(mojom::ConversationTurnPtr turn);
  void PerformAssistantGeneration(std::string page_content = "",
                                  bool is_video = false,
                                  std::string invalidation_token = "");
  void SetAPIError(const mojom::APIError& error);
  void UpdateOrCreateLastAssistantEntry(
      EngineConsumer::GenerationResultData result);
  void MaybeSeedOrClearSuggestions();
  void PerformQuestionGeneration(std::string page_content,
                                 bool is_video,
                                 std::string invalidation_token);

  void OnGetStagedEntriesFromContent(
      const std::optional<std::vector<SearchQuerySummary>>& entries);

  void GeneratePageContent(GetPageContentCallback callback);
  // This method is the same as |GeneratePageContent| but without DCHECKs.
  // Its used in tests.
  void GeneratePageContentInternal(GetPageContentCallback callback);
  void OnGeneratePageContentComplete(GetPageContentCallback callback,
                                     std::string previous_content);
  void OnEngineCompletionDataReceived(
      EngineConsumer::GenerationResultData result);
  void OnEngineCompletionComplete(EngineConsumer::GenerationResult result);
  void OnSuggestedQuestionsResponse(
      EngineConsumer::SuggestedQuestionResult result);

  void OnModelDataChanged();
  void OnConversationDeleted();
  void OnHistoryUpdate(mojom::ConversationTurnPtr entry);
  void OnConversationEntryAdded(mojom::ConversationTurnPtr& entry);
  void OnConversationEntryRemoved(std::optional<std::string> turn_id);
  void OnSuggestedQuestionsChanged();
  void OnClientConnectionChanged();
  void OnConversationTitleChanged(std::string_view title);
  void OnConversationTokenInfoChanged(uint64_t total_tokens,
                                      uint64_t trimmed_tokens);
  void OnConversationUIConnectionChanged(mojo::RemoteSetElementId id);
  void OnSelectedLanguageChanged(const std::string& selected_language);
  void OnAPIRequestInProgressChanged();
  void OnStateForConversationEntriesChanged();

  std::unique_ptr<AssociatedContentManager> associated_content_manager_;

  std::string model_key_;
  // Chat conversation entries
  std::vector<mojom::ConversationTurnPtr> chat_history_;
  mojom::ConversationTurnPtr pending_conversation_entry_;
  // Any previously-generated suggested questions
  std::vector<Suggestion> suggestions_;
  std::string selected_language_;

  // Is a conversation engine request in progress (does not include
  // non-conversation engine requests.
  bool is_request_in_progress_ = false;

  // TODO(petemill): Tracking whether the UI is open
  // for a conversation might not be neccessary anymore as there
  // are no automatic actions that occur anymore now that content
  // fetching is on-deman.
  // bool is_conversation_active_ = false;

  // Keep track of whether we've generated suggested questions for the current
  // context. We cannot rely on counting the questions in |suggested_questions_|
  // since they get removed when used, or we might not have received any
  // successfully.
  mojom::SuggestionGenerationStatus suggestion_generation_status_ =
      mojom::SuggestionGenerationStatus::None;

  // When this is true, the most recent content retrieval was different to the
  // previous one.
  bool is_content_different_ = true;

  bool is_print_preview_fallback_requested_ = false;

  std::unique_ptr<EngineConsumer> engine_ = nullptr;
  mojom::APIError current_error_ = mojom::APIError::None;

  // Data store UUID for conversation
  raw_ptr<mojom::Conversation> metadata_;
  raw_ptr<AIChatService, DanglingUntriaged> ai_chat_service_;
  raw_ptr<ModelService> model_service_;
  raw_ptr<AIChatCredentialManager, DanglingUntriaged> credential_manager_;
  raw_ptr<AIChatFeedbackAPI, DanglingUntriaged> feedback_api_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  base::ScopedObservation<ModelService, ModelService::Observer>
      models_observer_{this};

  base::ObserverList<Observer> observers_;
  mojo::ReceiverSet<mojom::ConversationHandler> receivers_;
  mojo::ReceiverSet<mojom::UntrustedConversationHandler> untrusted_receivers_;
  mojo::RemoteSet<mojom::ConversationUI> conversation_ui_handlers_;
  mojo::RemoteSet<mojom::UntrustedConversationUI>
      untrusted_conversation_ui_handlers_;

  base::WeakPtrFactory<ConversationHandler> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_HANDLER_H_
