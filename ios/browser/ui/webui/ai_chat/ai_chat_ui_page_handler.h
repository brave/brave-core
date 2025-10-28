// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

class ProfileIOS;

namespace web {
class WebState;
}

namespace ai_chat {

class AIChatUIPageHandler : public mojom::AIChatUIHandler,
                            public AssociatedContentDelegate::Observer {
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
  void OpenMemorySettings() override;
  void OpenConversationFullPage(const std::string& conversation_uuid) override;
  void OpenAIChatAgentProfile() override;
  void OpenURL(const GURL& url) override;
  void OpenStorageSupportUrl() override;
  void OpenModelSupportUrl() override;
  void GoPremium() override;
  void RefreshPremiumSession() override;
  void ManagePremium() override;
  void HandleVoiceRecognition(const std::string& conversation_uuid) override;
  void ShowSoftKeyboard() override;
  void ProcessImageFile(const std::vector<uint8_t>& file_data,
                        const std::string& filename,
                        ProcessImageFileCallback callback) override;
  void UploadFile(bool use_media_capture, UploadFileCallback callback) override;
  void GetPluralString(const std::string& key,
                       int32_t count,
                       GetPluralStringCallback callback) override;
  void CloseUI() override;
  void SetChatUI(mojo::PendingRemote<mojom::ChatUI> chat_ui,
                 SetChatUICallback callback) override;
  void BindRelatedConversation(
      mojo::PendingReceiver<mojom::ConversationHandler> receiver,
      mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler)
      override;
  void AssociateTab(mojom::TabDataPtr tab,
                    const std::string& conversation_uuid) override;
  void AssociateUrlContent(const GURL& url,
                           const std::string& title,
                           const std::string& conversation_uuid) override;
  void DisassociateContent(mojom::AssociatedContentPtr content,
                           const std::string& conversation_uuid) override;
  void NewConversation(
      mojo::PendingReceiver<mojom::ConversationHandler> receiver,
      mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler)
      override;

  void BindParentUIFrameFromChildFrame(
      mojo::PendingReceiver<mojom::ParentUIFrame> receiver);

 private:
  void SubmitVoiceQuery(const std::string& conversation_uuid, NSString* query);

  // AssociatedContentDelegate::Observer
  void OnRequestArchive(AssociatedContentDelegate* delegate) override;

  // AIChatTabHelper::Observer
  raw_ptr<web::WebState> owner_web_state_ = nullptr;
  raw_ptr<ProfileIOS> profile_ = nullptr;

  mojo::Receiver<ai_chat::mojom::AIChatUIHandler> receiver_;
  mojo::Remote<ai_chat::mojom::ChatUI> chat_ui_;

  // Conversations are not content associated either in standalone mode or in
  // global side panel mode, to the owner_web_contents_.
  bool conversations_are_content_associated_ = false;

  data_decoder::DataDecoder data_decoder_;

  base::WeakPtrFactory<AIChatUIPageHandler> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_
