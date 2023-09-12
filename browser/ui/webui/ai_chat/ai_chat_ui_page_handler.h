// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/components/ai_chat/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class TabStripModel;

namespace content {
class WebContents;
}

namespace favicon {
class FaviconService;
}  // namespace favicon

namespace ai_chat {
class AIChatUIPageHandler : public ai_chat::mojom::PageHandler,
                            public TabStripModelObserver,
                            public AIChatTabHelper::Observer,
                            public content::WebContentsObserver {
 public:
  AIChatUIPageHandler(
      content::WebContents* owner_web_contents,
      TabStripModel* tab_strip_model,
      Profile* profile,
      mojo::PendingReceiver<ai_chat::mojom::PageHandler> receiver,
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter);

  AIChatUIPageHandler(const AIChatUIPageHandler&) = delete;
  AIChatUIPageHandler& operator=(const AIChatUIPageHandler&) = delete;

  ~AIChatUIPageHandler() override;

  // ai_chat::mojom::PageHandler:
  void SetClientPage(
      mojo::PendingRemote<ai_chat::mojom::ChatUIPage> page) override;
  void SubmitHumanConversationEntry(const std::string& input) override;
  void GetConversationHistory(GetConversationHistoryCallback callback) override;
  void MarkAgreementAccepted() override;
  void GetSuggestedQuestions(GetSuggestedQuestionsCallback callback) override;
  void GenerateQuestions() override;
  void SetAutoGenerateQuestions(bool can_auto_generate_questions) override;
  void GetSiteInfo(GetSiteInfoCallback callback) override;
  void OpenBraveLeoSettings() override;
  void OpenBraveLeoWiki() override;
  void DisconnectPageContents() override;
  void ClearConversationHistory() override;
  void RetryAPIRequest() override;
  void GetAPIResponseError(GetAPIResponseErrorCallback callback) override;
  void UserHasValidPremiumCredentials(
      UserHasValidPremiumCredentialsCallback callback) override;

  // content::WebContentsObserver:
  void OnVisibilityChanged(content::Visibility visibility) override;

 private:
  // ChatTabHelper::Observer
  void OnHistoryUpdate() override;
  void OnAPIRequestInProgress(bool in_progress) override;
  void OnAPIResponseError(mojom::APIError error) override;
  void OnSuggestedQuestionsChanged(
      std::vector<std::string> questions,
      bool has_generated,
      mojom::AutoGenerateQuestionsPref auto_generate) override;
  void OnFaviconImageDataChanged() override;
  void OnPageHasContent() override;

  // TabStripModelObserver
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  void GetFaviconImageData(GetFaviconImageDataCallback callback) override;
  absl::optional<mojom::SiteInfo> BuildSiteInfo();

  void EnsureMojoConnected();
  void OnMojoConnectionError();
  void OnCredentialSummary(UserHasValidPremiumCredentialsCallback callback,
                           const std::string& domain,
                           const std::string& summary_string);

  mojo::Remote<ai_chat::mojom::ChatUIPage> page_;

  raw_ptr<AIChatTabHelper> active_chat_tab_helper_ = nullptr;
  raw_ptr<favicon::FaviconService> favicon_service_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;

  base::CancelableTaskTracker favicon_task_tracker_;

  base::ScopedObservation<AIChatTabHelper, AIChatTabHelper::Observer>
      chat_tab_helper_observation_{this};

  mojo::Receiver<ai_chat::mojom::PageHandler> receiver_;

  base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
      skus_service_getter_;
  mojo::Remote<skus::mojom::SkusService> skus_service_;

  base::WeakPtrFactory<AIChatUIPageHandler> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_
