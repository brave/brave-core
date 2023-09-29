/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_TAB_HELPER_H_
#define BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_TAB_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "brave/components/ai_chat/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"
#include "components/favicon/core/favicon_driver_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

class PrefService;

namespace network {
class SimpleURLLoader;
}  // namespace network

namespace ui {
class AXTree;
}  // namespace ui

namespace ai_chat {

class AIChatMetrics;

// Provides context to an AI Chat conversation in the form of the Tab's content
class AIChatTabHelper : public content::WebContentsObserver,
                        public content::WebContentsUserData<AIChatTabHelper>,
                        public favicon::FaviconDriverObserver {
 public:
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    virtual void OnHistoryUpdate() {}
    // We are on a page where we can read the content, so we can perform
    // page-specific actions.
    virtual void OnAPIRequestInProgress(bool in_progress) {}
    virtual void OnAPIResponseError(mojom::APIError error) {}
    virtual void OnSuggestedQuestionsChanged(
        std::vector<std::string> questions,
        bool has_generated,
        mojom::AutoGenerateQuestionsPref auto_generate) {}
    virtual void OnFaviconImageDataChanged() {}
    virtual void OnPageHasContent() {}
  };

  AIChatTabHelper(const AIChatTabHelper&) = delete;
  AIChatTabHelper& operator=(const AIChatTabHelper&) = delete;
  ~AIChatTabHelper() override;

  void ChangelModel(const std::string& model_key);
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
  bool HasPageContent();
  void DisconnectPageContents();
  void ClearConversationHistory();
  mojom::APIError GetCurrentAPIError();

 private:
  friend class content::WebContentsUserData<AIChatTabHelper>;

  AIChatTabHelper(content::WebContents* web_contents,
                  AIChatMetrics* ai_chat_metrics);

  void InitEngine();
  bool HasUserOptedIn();
  void OnUserOptedIn();
  void OnPermissionChangedAutoGenerateQuestions();
  std::string GetConversationHistoryString();
  bool MaybePopPendingRequests();
  void MaybeGeneratePageText();
  void MaybeGenerateQuestions();
  void CleanUp();
  void OnTabContentRetrieved(int64_t for_navigation_id,
                             std::string contents_text,
                             bool is_video = false);
  void OnEngineCompletionDataReceived(int64_t for_navigation_id,
                                      std::string result);
  void OnEngineCompletionComplete(int64_t for_navigation_id,
                                  EngineConsumer::GenerationResult result);
  void OnSuggestedQuestionsResponse(int64_t for_navigation_id,
                                    std::vector<std::string> result);
  void OnSuggestedQuestionsChanged();
  void OnPageHasContentChanged();

  // content::WebContentsObserver:
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void WebContentsDestroyed() override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void TitleWasSet(content::NavigationEntry* entry) override;

  // favicon::FaviconDriverObserver
  void OnFaviconUpdated(favicon::FaviconDriver* favicon_driver,
                        NotificationIconType notification_icon_type,
                        const GURL& icon_url,
                        bool icon_url_changed,
                        const gfx::Image& image) override;

  mojom::AutoGenerateQuestionsPref GetAutoGeneratePref();
  void SetAPIError(const mojom::APIError& error);

  raw_ptr<PrefService> pref_service_;
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

  raw_ptr<AIChatMetrics> ai_chat_metrics_;

  std::unique_ptr<mojom::ConversationTurn> pending_request_;

  base::WeakPtrFactory<AIChatTabHelper> weak_ptr_factory_{this};
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_BROWSER_AI_CHAT_TAB_HELPER_H_
