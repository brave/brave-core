// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_UI_PAGE_HANDLER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_UI_PAGE_HANDLER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/browser/ai_chat/upload_file_helper.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/ios/browser/api/ai_chat/ai_chat_tab_helper.h"
#include "ios/web/public/web_state_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class ProfileIOS;

namespace favicon {
class FaviconService;
}  // namespace favicon

namespace ai_chat {
class AIChatUIPageHandler : public mojom::AIChatUIHandler,
                            public AIChatTabHelper::Observer {
 public:
  AIChatUIPageHandler(
      web::WebState* owner_web_state,
      web::WebState* chat_web_state,
      ProfileIOS* profile,
      mojo::PendingReceiver<ai_chat::mojom::AIChatUIHandler> receiver);

  AIChatUIPageHandler(const AIChatUIPageHandler&) = delete;
  AIChatUIPageHandler& operator=(const AIChatUIPageHandler&) = delete;

  ~AIChatUIPageHandler() override;

  // mojom::AIChatUIHandler
  void OpenAIChatSettings() override;
  void OpenConversationFullPage(const std::string& conversation_uuid) override;
  void OpenURL(const GURL& url) override;
  void OpenStorageSupportUrl() override;
  void OpenModelSupportUrl() override;
  void GoPremium() override;
  void RefreshPremiumSession() override;
  void ManagePremium() override;
  void HandleVoiceRecognition(const std::string& conversation_uuid) override;
  void ShowSoftKeyboard() override;
  void UploadImage(bool use_media_capture,
                   UploadImageCallback callback) override;
  void CloseUI() override;
  void SetChatUI(mojo::PendingRemote<mojom::ChatUI> chat_ui,
                 SetChatUICallback callback) override;
  void BindRelatedConversation(
      mojo::PendingReceiver<mojom::ConversationHandler> receiver,
      mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler)
      override;
  void AssociateTab(mojom::TabDataPtr tab,
                    const std::string& conversation_uuid) override;
  void DisassociateTab(::ai_chat::mojom::TabDataPtr tab,
                       const std::string& conversation_uuid) override;
  void NewConversation(
      mojo::PendingReceiver<mojom::ConversationHandler> receiver,
      mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler)
      override;

  void BindParentUIFrameFromChildFrame(
      mojo::PendingReceiver<mojom::ParentUIFrame> receiver);

 private:
  class ChatContextObserver : public web::WebStateObserver {
   public:
    explicit ChatContextObserver(web::WebState* web_state,
                                 AIChatUIPageHandler& page_handler);
    ~ChatContextObserver() override;

   private:
    raw_ptr<web::WebState> web_state_;

    // web::WebStateObserver
    void WebStateDestroyed(web::WebState* web_state) override;
    raw_ref<AIChatUIPageHandler> page_handler_;
  };

  void HandleWebStateDestroyed();

  // AIChatTabHelper::Observer
  raw_ptr<AIChatTabHelper> active_chat_tab_helper_ = nullptr;
  raw_ptr<web::WebState> owner_web_state_ = nullptr;
  raw_ptr<ProfileIOS> profile_ = nullptr;
  std::unique_ptr<AIChatMetrics> ai_chat_metrics_;

  base::ScopedObservation<AIChatTabHelper, AIChatTabHelper::Observer>
      chat_tab_helper_observation_{this};
  std::unique_ptr<ChatContextObserver> chat_context_observer_;

  std::unique_ptr<UploadFileHelper> upload_file_helper_;

  mojo::Receiver<ai_chat::mojom::AIChatUIHandler> receiver_;
  mojo::Remote<ai_chat::mojom::ChatUI> chat_ui_;

  base::WeakPtrFactory<AIChatUIPageHandler> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_UI_PAGE_HANDLER_H_
