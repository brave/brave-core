// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace content {
class WebContents;
}

namespace favicon {
class FaviconService;
}  // namespace favicon

namespace ai_chat {
class AIChatUIPageHandler : public mojom::AIChatUIHandler,
                            public AIChatTabHelper::Observer {
 public:
  AIChatUIPageHandler(
      content::WebContents* owner_web_contents,
      content::WebContents* chat_context_web_contents,
      Profile* profile,
      mojo::PendingReceiver<ai_chat::mojom::AIChatUIHandler> receiver);

  AIChatUIPageHandler(const AIChatUIPageHandler&) = delete;
  AIChatUIPageHandler& operator=(const AIChatUIPageHandler&) = delete;

  ~AIChatUIPageHandler() override;

  // mojom::AIChatUIHandler
  void OpenAIChatSettings() override;
  void OpenConversationFullPage(const std::string& conversation_uuid) override;
  void OpenURL(const GURL& url) override;
  void OpenLearnMoreAboutBraveSearchWithLeo() override;
  void OpenModelSupportUrl() override;
  void GoPremium() override;
  void RefreshPremiumSession() override;
  void ManagePremium() override;
  void HandleVoiceRecognition(const std::string& conversation_uuid) override;
  void CloseUI() override;
  void SetChatUI(mojo::PendingRemote<mojom::ChatUI> chat_ui) override;
  void BindRelatedConversation(
      mojo::PendingReceiver<mojom::ConversationHandler> receiver,
      mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler)
      override;
  void NewConversation(
      mojo::PendingReceiver<mojom::ConversationHandler> receiver,
      mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler)
      override;
  void GetFaviconImageData(const std::string& conversation_id,
                           GetFaviconImageDataCallback callback) override;

 private:
  class ChatContextObserver : public content::WebContentsObserver {
   public:
    explicit ChatContextObserver(content::WebContents* web_contents,
                                 AIChatUIPageHandler& page_handler);
    ~ChatContextObserver() override;

   private:
    // content::WebContentsObserver
    void WebContentsDestroyed() override;
    raw_ref<AIChatUIPageHandler> page_handler_;
  };

  void HandleWebContentsDestroyed();

  // AIChatTabHelper::Observer
  void OnAssociatedContentNavigated(int new_navigation_id) override;

  void GetFaviconImageDataForAssociatedContent(
      GetFaviconImageDataCallback callback,
      mojom::SiteInfoPtr content_info,
      bool should_send_page_contents);

  raw_ptr<AIChatTabHelper> active_chat_tab_helper_ = nullptr;
  raw_ptr<content::WebContents> owner_web_contents_ = nullptr;
  raw_ptr<favicon::FaviconService> favicon_service_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;

  base::CancelableTaskTracker favicon_task_tracker_;

  base::ScopedObservation<AIChatTabHelper, AIChatTabHelper::Observer>
      chat_tab_helper_observation_{this};
  std::unique_ptr<ChatContextObserver> chat_context_observer_;

  mojo::Receiver<ai_chat::mojom::AIChatUIHandler> receiver_;
  mojo::Remote<ai_chat::mojom::ChatUI> chat_ui_;

  base::WeakPtrFactory<AIChatUIPageHandler> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_
