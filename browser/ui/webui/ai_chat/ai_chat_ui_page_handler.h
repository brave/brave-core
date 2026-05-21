// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "base/task/cancelable_task_tracker.h"
#include "brave/browser/ai_chat/file_text_extractor_base.h"
#include "build/build_config.h"
#include "pdf/buildflags.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/ai_chat/text_file_extractor.h"
#endif

#if BUILDFLAG(ENABLE_PDF)
#include "brave/browser/ai_chat/pdf_text_extractor.h"
#endif
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/core/browser/associated_content_driver.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "build/build_config.h"
#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "url/gurl.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#endif

namespace content {
class WebContents;
}

#if !BUILDFLAG(IS_ANDROID)
class TabStripModel;
#endif

namespace favicon {
class FaviconService;
}  // namespace favicon

namespace ai_chat {
class AIChatUIPageHandler : public mojom::AIChatUIHandler,
                            public AssociatedContentDelegate::Observer
#if !BUILDFLAG(IS_ANDROID)
    ,
                            public TabStripModelObserver
#endif
{
 public:
  AIChatUIPageHandler(
      content::WebContents* owner_web_contents,
      content::WebContents* chat_context_web_contents,
      Profile* profile,
      mojo::PendingReceiver<ai_chat::mojom::AIChatUIHandler> receiver
#if !BUILDFLAG(IS_ANDROID)
      ,
      TabStripModel* tab_strip_model = nullptr
#endif
  );

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
  void ProcessPdfFile(const std::vector<uint8_t>& file_data,
                      const std::string& filename,
                      ProcessPdfFileCallback callback) override;
  void ProcessTextFile(const std::vector<uint8_t>& file_data,
                       const std::string& filename,
                       ProcessTextFileCallback callback) override;
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

  // AssociatedContentDelegate::Observer
  void OnRequestArchive(AssociatedContentDelegate* delegate) override;
  void OnNewPage(AssociatedContentDelegate* delegate) override;

#if !BUILDFLAG(IS_ANDROID)
  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
#endif

  void NotifyNewDefaultConversation();

  // Shared helper for ProcessTextFile / ProcessPdfFile.
  void ExtractAndProcessFile(
      std::unique_ptr<FileTextExtractorBase> extractor,
      const std::vector<uint8_t>& file_data,
      const base::FilePath::StringType& extension,
      const std::string& filename,
      mojom::UploadedFileType file_type,
      base::OnceCallback<void(mojom::UploadedFilePtr)> callback);
  void OnFileExtracted(
      FileTextExtractorBase* extractor_ptr,
      std::string filename,
      std::vector<uint8_t> file_data,
      mojom::UploadedFileType file_type,
      base::OnceCallback<void(mojom::UploadedFilePtr)> callback,
      std::optional<std::string> extracted_text);

  raw_ptr<AIChatTabHelper> active_chat_tab_helper_ = nullptr;
  raw_ptr<content::WebContents> owner_web_contents_ = nullptr;
  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<AIChatMetrics> ai_chat_metrics_;

  base::ScopedObservation<AssociatedContentDelegate,
                          AssociatedContentDelegate::Observer>
      associated_content_delegate_observation_{this};
  std::unique_ptr<ChatContextObserver> chat_context_observer_;

  // DataDecoder instance for processing image data
  data_decoder::DataDecoder data_decoder_;

  // Active file extractors (owned until extraction completes)
  std::vector<std::unique_ptr<FileTextExtractorBase>> extractors_;

  mojo::Receiver<ai_chat::mojom::AIChatUIHandler> receiver_;
  mojo::Remote<ai_chat::mojom::ChatUI> chat_ui_;

  // Conversations are not content associated either in standalone mode or in
  // global side panel mode, to the owner_web_contents_.
  bool conversations_are_content_associated_ = false;

  base::WeakPtrFactory<AIChatUIPageHandler> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_UI_PAGE_HANDLER_H_
