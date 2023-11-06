/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_DRIVER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_DRIVER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/prefs/pref_change_registrar.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

class PrefService;

namespace ai_chat {
class AIChatMetrics;

enum PageContentAssociation {
  HAS_CONTENT,
  NO_CONTENT,
  FETCHING_CONTENT,
};

class ConversationDriver {
 public:
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    virtual void OnHistoryUpdate() {}
    virtual void OnAPIRequestInProgress(bool in_progress) {}
    virtual void OnAPIResponseError(mojom::APIError error) {}
    virtual void OnModelChanged(const std::string& model_key) {}
    virtual void OnSuggestedQuestionsChanged(
        std::vector<std::string> questions,
        bool has_generated,
        mojom::AutoGenerateQuestionsPref auto_generate) {}
    virtual void OnFaviconImageDataChanged() {}
    virtual void OnPageHasContent(bool page_contents_is_truncated) {}
  };

  ConversationDriver(raw_ptr<PrefService> pref_service,
                raw_ptr<AIChatMetrics> ai_chat_metrics,
       std::unique_ptr<AIChatCredentialManager> credential_manager,
       scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  virtual ~ConversationDriver();

  ConversationDriver(const ConversationDriver&) = delete;
  ConversationDriver& operator=(const ConversationDriver&) = delete;

  void ChangeModel(const std::string& model_key);
  const mojom::Model& GetCurrentModel();
  const std::vector<mojom::ConversationTurn>& GetConversationHistory();
  // Whether the UI for this conversation is open or not. Determines
  // whether content is retrieved and queries are sent for the conversation
  // when the page changes.
  void OnConversationActiveChanged(bool is_conversation_active);
  void AddToConversationHistory(mojom::ConversationTurn turn);
  void UpdateOrCreateLastAssistantEntry(std::string text);
  void MakeAPIRequestWithConversationHistoryUpdate(
      mojom::ConversationTurn turn);
  void RetryAPIRequest();
  bool IsRequestInProgress();
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  // On-demand request to fetch questions related to the content. If no content
  // is available for the current page, or if questions
  // are already generated, nothing will happen.
  void GenerateQuestions();
  std::vector<std::string> GetSuggestedQuestions(
      bool& can_generate,
      mojom::AutoGenerateQuestionsPref& auto_generate);
  PageContentAssociation HasPageContent();
  void DisconnectPageContents();
  void ClearConversationHistory();
  mojom::APIError GetCurrentAPIError();
  void GetPremiumStatus(
      mojom::PageHandler::GetPremiumStatusCallback callback);
  bool IsPageContentsTruncated();

 protected:
  virtual GURL GetPageURL() const = 0;
  virtual void GetPageContent(
      base::OnceCallback<void(std::string, bool is_video)> callback) const = 0;
  virtual bool HasPrimaryMainFrame() const = 0;
  virtual bool IsDocumentOnLoadCompletedInPrimaryMainFrame() const = 0;

  virtual void OnFaviconImageDataChanged();

  void MaybeGeneratePageText();
  void CleanUp();

  int64_t GetNavigationId() const;
  void SetNavigationId(int64_t navigation_id);

  bool IsSameDocumentNavigation() const;
  void SetSameDocumentNavigation(bool same_document_navigation);

 private:
  void InitEngine();
  bool HasUserOptedIn();
  void OnUserOptedIn();
  void OnPermissionChangedAutoGenerateQuestions();
  bool MaybePopPendingRequests();
  void MaybeGenerateQuestions();

  void OnPageContentRetrieved(int64_t navigation_id,
                              std::string contents_text,
                              bool is_video = false);
  void OnEngineCompletionDataReceived(int64_t navigation_id,
                                      std::string result);
  void OnEngineCompletionComplete(int64_t navigation_id,
                                  EngineConsumer::GenerationResult result);
  void OnSuggestedQuestionsResponse(int64_t navigation_id,
                                    std::vector<std::string> result);
  void OnSuggestedQuestionsChanged();
  void OnPageHasContentChanged(bool page_contents_is_truncated);
  void OnPremiumStatusReceived(
      mojom::PageHandler::GetPremiumStatusCallback parent_callback,
      mojom::PremiumStatus premium_status);

  mojom::AutoGenerateQuestionsPref GetAutoGeneratePref();
  void SetAPIError(const mojom::APIError& error);

  raw_ptr<PrefService> pref_service_;
  raw_ptr<AIChatMetrics> ai_chat_metrics_;
  std::unique_ptr<AIChatCredentialManager> credential_manager_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<EngineConsumer> engine_ = nullptr;

  PrefChangeRegistrar pref_change_registrar_;
  base::ObserverList<Observer> observers_;

  // TODO(nullhook): Abstract the data model
  std::string model_key_;
  std::vector<mojom::ConversationTurn> chat_history_;
  std::string article_text_;
  bool is_conversation_active_ = false;
  bool is_page_text_fetch_in_progress_ = false;
  bool is_request_in_progress_ = false;
  std::vector<std::string> suggested_questions_;
  bool has_generated_questions_ = false;
  bool is_video_ = false;
  bool should_page_content_be_disconnected_ = false;
  // Store the unique ID for each navigation so that
  // we can ignore API responses for previous navigations.
  int64_t current_navigation_id_;
  bool is_same_document_navigation_ = false;
  mojom::APIError current_error_ = mojom::APIError::None;
  mojom::PremiumStatus last_premium_status_ = mojom::PremiumStatus::Inactive;

  std::unique_ptr<mojom::ConversationTurn> pending_request_;

  base::WeakPtrFactory<ConversationDriver> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_CONVERSATION_DRIVER_H_
