/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_TAB_HELPER_H_
#define BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_TAB_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/observer_list.h"
#include "brave/components/ai_chat/ai_chat.mojom.h"
#include "brave/components/ai_chat/ai_chat_api.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class PrefService;

namespace network {
class SimpleURLLoader;
}  // namespace network

namespace ui {
class AXTree;
}  // namespace ui

class AIChatTabHelper : public content::WebContentsObserver,
                        public content::WebContentsUserData<AIChatTabHelper> {
 public:
  class Observer : public base::CheckedObserver {
   public:
    ~Observer() override {}

    virtual void OnHistoryUpdate() {}
    // We are on a page where we can read the content, so we can perform
    // page-specific actions.
    virtual void OnPageTextIsAvailable(bool is_available) {}
    virtual void OnAPIRequestInProgress(bool in_progress) {}
    virtual void OnSuggestedQuestionsChanged(std::vector<std::string> questions,
                                             bool has_generated,
                                             bool auto_generate) {}
  };

  AIChatTabHelper(const AIChatTabHelper&) = delete;
  AIChatTabHelper& operator=(const AIChatTabHelper&) = delete;
  ~AIChatTabHelper() override;

  const std::vector<ai_chat::mojom::ConversationTurn>& GetConversationHistory();
  void AddToConversationHistory(const ai_chat::mojom::ConversationTurn& turn);
  void UpdateOrCreateLastAssistantEntry(const std::string& text);
  void MakeAPIRequestWithConversationHistoryUpdate(
      const ai_chat::mojom::ConversationTurn& turn);
  bool IsRequestInProgress();
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  // On-demand request to fetch questions related to the content. If no content
  // is available for the current page, or if questions
  // are already generated, nothing will happen.
  void GenerateQuestions();
  std::vector<std::string> GetSuggestedQuestions(bool& can_generate,
                                                 bool& auto_generate);

 private:
  using OnArticleSummaryCallback =
      base::OnceCallback<void(const std::u16string& summary,
                              bool is_from_cache)>;
  using SimpleURLLoaderList = std::list<std::unique_ptr<network::SimpleURLLoader>>;

  friend class content::WebContentsUserData<AIChatTabHelper>;

  explicit AIChatTabHelper(content::WebContents* web_contents);

  bool HasUserOptedIn();
  const std::string& GetConversationHistoryString();
  void GeneratePageText();
  void OnSnapshotFinished(const ui::AXTreeUpdate& result);
  void DistillViaAlgorithm(const ui::AXTree& tree);
  void CleanUp();
  void OnAPIStreamDataReceived(data_decoder::DataDecoder::ValueOrError result);
  void OnAPIStreamDataComplete(api_request_helper::APIRequestResult result);
  void OnSuggestedQuestionsChanged();

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;
  void DocumentOnLoadCompletedInPrimaryMainFrame() override;
  void WebContentsDestroyed() override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  raw_ref<PrefService> pref_service_;
  std::unique_ptr<AIChatAPI> ai_chat_api_ = nullptr;
  SimpleURLLoaderList url_loaders_;

  base::ObserverList<Observer> observers_;

  // TODO(nullhook): Abstract the data model
  std::vector<ai_chat::mojom::ConversationTurn> chat_history_;
  std::string article_text_;
  std::string history_text_;
  bool is_request_in_progress_ = false;
  std::vector<std::string> suggested_questions_;
  bool has_generated_questions_ = false;
  // Store the unique ID for each navigation so that
  // we can ignore API responses for previous navigations.
  int64_t current_navigation_id_;


  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_TAB_HELPER_H_
