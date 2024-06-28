/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_DRIVER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_DRIVER_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/one_shot_event.h"
#include "base/scoped_observation.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/ai_chat_feedback_api.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/model_service.h"
#include "brave/components/ai_chat/core/browser/text_embedder.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

class PrefService;

FORWARD_DECLARE_TEST(AIChatUIBrowserTest, PrintPreviewFallback);
namespace ai_chat {
class AIChatMetrics;

class ConversationDriver : public ModelService::Observer {
 public:
  using GeneratedTextCallback =
      base::RepeatingCallback<void(const std::string& text)>;
  // |invalidation_token| is an optional parameter that will be passed back on
  // the next call to |GetPageContent| so that the implementer may determine if
  // the page content is static or if it needs to be fetched again. Most page
  // content should be fetched again, but some pages are known to be static
  // during their lifetime and may have expensive content fetching, e.g. videos
  // with transcripts fetched over the network.
  using GetPageContentCallback = base::OnceCallback<
      void(std::string content, bool is_video, std::string invalidation_token)>;

  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    virtual void OnHistoryUpdate() {}
    virtual void OnAPIRequestInProgress(bool in_progress) {}
    virtual void OnAPIResponseError(mojom::APIError error) {}
    virtual void OnModelDataChanged(
        const std::string& model_key,
        const std::vector<mojom::ModelPtr>& model_list) {}
    virtual void OnSuggestedQuestionsChanged(
        std::vector<std::string> questions,
        mojom::SuggestionGenerationStatus suggestion_generation_status) {}
    virtual void OnFaviconImageDataChanged() {}
    virtual void OnPageHasContent(mojom::SiteInfoPtr site_info) {}
    virtual void OnPrintPreviewRequested(bool is_pdf) {}
  };

  ConversationDriver(
      PrefService* profile_prefs,
      PrefService* local_state,
      ModelService* model_service,
      AIChatMetrics* ai_chat_metrics,
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::string_view channel_string);
  ConversationDriver(
      PrefService* profile_prefs,
      PrefService* local_state,
      ModelService* model_service,
      AIChatMetrics* ai_chat_metrics,
      std::unique_ptr<AIChatCredentialManager> credential_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::string_view channel_string);
  ~ConversationDriver() override;

  ConversationDriver(const ConversationDriver&) = delete;
  ConversationDriver& operator=(const ConversationDriver&) = delete;

  void ChangeModel(const std::string& model_key);
  std::string GetDefaultModel();
  void SetDefaultModel(const std::string& model_key);
  const mojom::Model& GetCurrentModel();
  const std::vector<mojom::ModelPtr>& GetModels();
  const std::vector<mojom::ConversationTurnPtr>& GetConversationHistory();
  std::vector<mojom::ConversationTurnPtr> GetVisibleConversationHistory();
  // Whether the UI for this conversation is open or not. Determines
  // whether content is retrieved and queries are sent for the conversation
  // when the page changes.
  void OnConversationActiveChanged(bool is_conversation_active);
  void AddToConversationHistory(mojom::ConversationTurnPtr turn);
  void SubmitHumanConversationEntry(mojom::ConversationTurnPtr turn);
  void RetryAPIRequest();
  bool IsRequestInProgress();
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  // On-demand request to fetch questions related to the content. If no content
  // is available for the current page, or if questions
  // are already generated, nothing will happen.
  void GenerateQuestions();
  std::vector<std::string> GetSuggestedQuestions(
      mojom::SuggestionGenerationStatus& suggestion_status);
  void DisconnectPageContents();
  void SetShouldSendPageContents(bool should_send);
  bool GetShouldSendPageContents();
  void ClearConversationHistory();
  mojom::APIError GetCurrentAPIError();
  mojom::ConversationTurnPtr ClearErrorAndGetFailedMessage();
  void GetPremiumStatus(mojom::PageHandler::GetPremiumStatusCallback callback);
  bool GetCanShowPremium();
  void DismissPremiumPrompt();
  bool HasUserOptedIn();
  void SetUserOptedIn(bool user_opted_in);
  int GetContentUsedPercentage();
  void SubmitSummarizationRequest();
  mojom::SiteInfoPtr BuildSiteInfo();
  bool HasPendingConversationEntry();

  void AddSubmitSelectedTextError(const std::string& selected_text,
                                  mojom::ActionType action_type,
                                  mojom::APIError error);

  void SubmitSelectedText(
      const std::string& selected_text,
      mojom::ActionType action_type,
      GeneratedTextCallback received_callback = base::NullCallback(),
      EngineConsumer::GenerationCompletedCallback completed_callback =
          base::NullCallback());

  void SubmitSelectedTextWithQuestion(
      const std::string& selected_text,
      const std::string& question,
      mojom::ActionType action_type,
      GeneratedTextCallback received_callback,
      EngineConsumer::GenerationCompletedCallback completed_callback);

  void ModifyConversation(uint32_t turn_index, const std::string& new_text);

  void RateMessage(bool is_liked,
                   uint32_t turn_id,
                   mojom::PageHandler::RateMessageCallback callback);

  void SendFeedback(const std::string& category,
                    const std::string& feedback,
                    const std::string& rating_id,
                    bool send_hostname,
                    mojom::PageHandler::SendFeedbackCallback callback);

  // Used to determine whether the page content should be unlinked when
  // triggering from outside of the side panel, such as context menu or
  // location bar, and unlink it if so. If the panel is not open and there is
  // no existing chat history, the page content should not be linked.
  void MaybeUnlinkPageContent();

  const std::string& GetArticleTextForTesting() const { return article_text_; }
  bool IsSuggestionsEmptyForTesting() const { return suggestions_.empty(); }

  void SetEngineForTesting(std::unique_ptr<EngineConsumer> engine_for_testing) {
    engine_ = std::move(engine_for_testing);
  }
  EngineConsumer* GetEngineForTesting() { return engine_.get(); }

  void SetChatHistoryForTesting(
      std::vector<mojom::ConversationTurnPtr> history) {
    chat_history_ = std::move(history);
  }

 protected:
  virtual GURL GetPageURL() const = 0;
  virtual std::u16string GetPageTitle() const = 0;

  // Implementer should fetch content from the "page" associated with this
  // conversation.
  // |is_video| lets the conversation know that the content is focused on video
  // content so that various UI language can be adapted.
  // |invalidation_token| is an optional parameter received in a prior callback
  // response of this function against the same page. See GetPageContentCallback
  // for explanation.
  virtual void GetPageContent(GetPageContentCallback callback,
                              std::string_view invalidation_token) = 0;

  virtual void PrintPreviewFallback(GetPageContentCallback callback) = 0;

  virtual void OnFaviconImageDataChanged();

  // Implementer should call this when the content is updated in a way that
  // will not be detected by the on-demand techniques used by GetPageContent.
  // For example for sites where GetPageContent does not read the live DOM but
  // reads static JS from HTML that doesn't change for same-page navigation and
  // we need to intercept new JS data from subresource loads.
  void OnPageContentUpdated(std::string content,
                            bool is_video,
                            std::string invalidation_token);

  // To be called when a page navigation is detected and a new conversation
  // is expected.
  void OnNewPage(int64_t navigation_id);

  void NotifyPrintPreviewRequested(bool is_pdf);

 private:
  FRIEND_TEST_ALL_PREFIXES(::AIChatUIBrowserTest, PrintPreviewFallback);
  FRIEND_TEST_ALL_PREFIXES(ConversationDriverUnitTest,
                           UpdateOrCreateLastAssistantEntry_Delta);
  FRIEND_TEST_ALL_PREFIXES(ConversationDriverUnitTest,
                           UpdateOrCreateLastAssistantEntry_DeltaWithSearch);
  FRIEND_TEST_ALL_PREFIXES(ConversationDriverUnitTest,
                           UpdateOrCreateLastAssistantEntry_NotDelta);
  FRIEND_TEST_ALL_PREFIXES(ConversationDriverUnitTest,
                           UpdateOrCreateLastAssistantEntry_NotDeltaWithSearch);

  void InitEngine();
  void OnUserOptedIn();
  bool MaybePopPendingRequests();
  void MaybeSeedOrClearSuggestions();

  void PerformAssistantGeneration(const std::string& input,
                                  int64_t current_navigation_id,
                                  std::string page_content = "",
                                  bool is_video = false,
                                  std::string invalidation_token = "");
  void OnGetRefinedPageContent(
      const std::string& input,
      EngineConsumer::GenerationDataCallback data_received_callback,
      EngineConsumer::GenerationCompletedCallback data_completed_callback,
      std::string page_content,
      bool is_video,
      base::expected<std::string, std::string> refined_page_content);

  void GeneratePageContent(GetPageContentCallback callback);
  void OnGeneratePageContentComplete(int64_t navigation_id,
                                     GetPageContentCallback callback,
                                     std::string contents_text,
                                     bool is_video,
                                     std::string invalidation_token);
  void OnExistingGeneratePageContentComplete(GetPageContentCallback callback);

  void OnEngineCompletionDataReceived(int64_t navigation_id,
                                      mojom::ConversationEntryEventPtr result);
  void OnEngineCompletionComplete(int64_t navigation_id,
                                  EngineConsumer::GenerationResult result);
  void OnSuggestedQuestionsResponse(
      int64_t navigation_id,
      EngineConsumer::SuggestedQuestionResult result);
  void OnSuggestedQuestionsChanged();
  void OnPageHasContentChanged(mojom::SiteInfoPtr site_info);
  void OnPremiumStatusReceived(
      mojom::PageHandler::GetPremiumStatusCallback parent_callback,
      mojom::PremiumStatus premium_status,
      mojom::PremiumInfoPtr premium_info);
  void UpdateOrCreateLastAssistantEntry(mojom::ConversationEntryEventPtr text);
  void SetAPIError(const mojom::APIError& error);
  bool IsContentAssociationPossible();

  void CleanUp();

  // ModelService::Observer
  void OnModelListUpdated() override;
  void OnModelRemoved(const std::string& removed_key) override;

  raw_ptr<PrefService> pref_service_;
  raw_ptr<AIChatMetrics> ai_chat_metrics_;
  std::unique_ptr<AIChatCredentialManager> credential_manager_;
  std::unique_ptr<ai_chat::AIChatFeedbackAPI> feedback_api_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<EngineConsumer> engine_ = nullptr;

  PrefChangeRegistrar pref_change_registrar_;
  base::ObserverList<Observer> observers_;
  base::ScopedObservation<ModelService, ModelService::Observer>
      models_observer_{this};

  // TODO(nullhook): Abstract the data model
  std::string model_key_;
  raw_ptr<ModelService> model_service_;
  std::vector<mojom::ConversationTurnPtr> chat_history_;
  bool is_conversation_active_ = false;

  // Page content
  std::string article_text_;
  bool is_content_refined_ = false;
  std::string content_invalidation_token_;
  bool is_page_text_fetch_in_progress_ = false;
  bool is_print_preview_fallback_requested_ = false;
  std::unique_ptr<base::OneShotEvent> on_page_text_fetch_complete_;

  bool is_request_in_progress_ = false;
  std::vector<std::string> suggestions_;
  // Keep track of whether we've generated suggested questions for the current
  // context. We cannot rely on counting the questions in |suggested_questions_|
  // since they get removed when used, or we might not have received any
  // successfully.
  mojom::SuggestionGenerationStatus suggestion_generation_status_ =
      mojom::SuggestionGenerationStatus::None;
  bool is_video_ = false;
  bool should_send_page_contents_ = true;

  // Store the unique ID for each "page" so that
  // we can ignore API async responses against any navigated-away-from
  // documents.
  int64_t current_navigation_id_;

  mojom::APIError current_error_ = mojom::APIError::None;
  mojom::PremiumStatus last_premium_status_ = mojom::PremiumStatus::Unknown;

  mojom::ConversationTurnPtr pending_conversation_entry_;

  std::unique_ptr<TextEmbedder> text_embedder_;

  base::WeakPtrFactory<ConversationDriver> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_DRIVER_H_
