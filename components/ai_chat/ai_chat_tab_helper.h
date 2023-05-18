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
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

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
    virtual void OnAPIRequestInProgress(bool in_progress) {}
    virtual void OnRequestSummaryFailed() {}
  };

  AIChatTabHelper(const AIChatTabHelper&) = delete;
  AIChatTabHelper& operator=(const AIChatTabHelper&) = delete;
  ~AIChatTabHelper() override;

  const std::vector<ai_chat::mojom::ConversationTurn>& GetConversationHistory();
  void AddToConversationHistory(const ai_chat::mojom::ConversationTurn& turn);
  void MakeAPIRequestWithConversationHistoryUpdate(
      const ai_chat::mojom::ConversationTurn& turn);
  bool IsRequestInProgress() { return is_request_in_progress_; }
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Retrieves the AXTree of the mainframe and sends it to
  // the AIChat API for summarization
  void RequestSummary();

 private:
  using OnArticleSummaryCallback =
      base::OnceCallback<void(const std::u16string& summary,
                              bool is_from_cache)>;

  friend class content::WebContentsUserData<AIChatTabHelper>;

  explicit AIChatTabHelper(content::WebContents* web_contents);

  const std::string& GetConversationHistoryString();
  void OnSnapshotFinished(const ui::AXTreeUpdate& result);
  void DistillViaAlgorithm(const ui::AXTree& tree);
  void SetArticleSummaryString(const std::string& text);
  void CleanUp();
  void OnAPIResponse(bool contains_summary,
                     const std::string& assistant_input,
                     bool success);
  void SetRequestInProgress(bool in_progress);

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;
  void WebContentsDestroyed() override;

  std::unique_ptr<AIChatAPI> ai_chat_api_ = nullptr;

  base::ObserverList<Observer> observers_;

  // TODO(nullhook): Abstract the data model
  std::vector<ai_chat::mojom::ConversationTurn> chat_history_;
  std::string article_text_;
  std::string history_text_;
  std::string article_summary_;
  bool is_request_in_progress_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_COMPONENTS_AI_CHAT_AI_CHAT_TAB_HELPER_H_
