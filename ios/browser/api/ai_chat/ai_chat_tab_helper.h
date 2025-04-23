/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_TAB_HELPER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/ios/browser/api/ai_chat/associated_content_driver_ios.h"
#include "brave/ios/browser/api/ai_chat/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "ios/web/public/web_state_user_data.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "url/gurl.h"

namespace gfx {
class Image;
}  // namespace gfx
namespace mojo {
template <typename T>
class PendingAssociatedReceiver;
}  // namespace mojo

namespace ai_chat {
class AIChatMetrics;

// Provides context to an AI Chat conversation in the form of the Tab's content
class AIChatTabHelper : public web::WebStateObserver,
                        public web::WebStateUserData<AIChatTabHelper>,
                        public mojom::PageContentExtractorHost,
                        public AssociatedContentDriverIOS {
 public:
  using GetPageContentCallback = ConversationHandler::GetPageContentCallback;

  static void BindPageContentExtractorHost(
      content::RenderFrameHost* rfh,
      mojo::PendingAssociatedReceiver<mojom::PageContentExtractorHost>
          receiver);

  AIChatTabHelper(const AIChatTabHelper&) = delete;
  AIChatTabHelper& operator=(const AIChatTabHelper&) = delete;
  ~AIChatTabHelper() override;

  // mojom::PageContentExtractorHost
  void OnInterceptedPageContentChanged() override;

  void GetOpenAIChatButtonNonce(
      mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback);

  class PageContentFetcherDelegate {
   public:
    using FetchPageContentCallback =
        base::OnceCallback<void(std::string page_content,
                                bool is_video,
                                std::string invalidation_token)>;

    virtual ~PageContentFetcherDelegate() = default;

    // Gets text of the page content, making an attempt
    // to only consider the main content of the page.
    virtual void FetchPageContent(std::string_view invalidation_token,
                                  FetchPageContentCallback callback) = 0;

    // Attempts to find a search summarizer key for the page.
    virtual void GetSearchSummarizerKey(
        GetSearchSummarizerKeyCallback callback) = 0;

    // Fetches the nonce for the OpenLeo button from the page HTML and validate
    // if it matches the href URL and the passed in nonce.
    virtual void GetOpenAIChatButtonNonce(
        mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback
            callback) = 0;
  };

 private:
  friend class web::WebStateUserData<AIChatTabHelper>;

  // PrintPreviewExtractionDelegate is provided as it's implementation is
  // in a different layer.
  AIChatTabHelper(web::WebState* web_state,
                  std::unique_ptr<PrintPreviewExtractionDelegate>
                      print_preview_extraction_delegate);

  // web::WebStateObserver
  void DidStartNavigation(WebState* web_state,
                          NavigationContext* navigation_context) override;
                          
  void DidRedirectNavigation(WebState* web_state,
                             NavigationContext* navigation_context) override;
                          
  void DidFinishNavigation(WebState* web_state,
                           NavigationContext* navigation_context) override;

  void PageLoaded(WebState* web_state,
                  PageLoadCompletionStatus load_completion_status) override;

  void LoadProgressChanged(WebState* web_state, double progress) override;

  void DidChangeBackForwardState(WebState* web_state) override;

  void TitleWasSet(WebState* web_state) override;

  void FaviconUrlUpdated(WebState* web_state,
                         const std::vector<FaviconURL>& candidates) override;

  // ai_chat::AssociatedContentDriver
  GURL GetPageURL() const override;
  void GetPageContent(GetPageContentCallback callback,
                      std::string_view invalidation_token) override;
  std::u16string GetPageTitle() const override;
  void OnNewPage(int64_t navigation_id) override;

  // Called when an event of significance occurs that, if the page is a
  // same-document navigation, should result in that previous navigation
  // being considered as a new page.
  void MaybeSameDocumentIsNewPage();

  void GetSearchSummarizerKey(GetSearchSummarizerKeyCallback callback) override;

  bool HasOpenAIChatPermission() const override;

  void OnFetchPageContentComplete(GetPageContentCallback callback,
                                  std::string content,
                                  bool is_video,
                                  std::string invalidation_token);

  void OnExtractPrintPreviewContentComplete(GetPageContentCallback callback,
                                            std::string content);

  void BindPageContentExtractorReceiver(
      mojo::PendingAssociatedReceiver<mojom::PageContentExtractorHost>
          receiver);

  void SetPendingGetContentCallback(GetPageContentCallback callback);

  raw_ptr<AIChatMetrics> ai_chat_metrics_;

  bool is_same_document_navigation_ = false;
  int pending_navigation_id_;
  std::u16string previous_page_title_;
  bool is_pdf_a11y_info_loaded_ = false;
  uint8_t check_pdf_a11y_tree_attempts_ = 0;
  bool is_page_loaded_ = false;

  // TODO(petemill): Use signal to allow for multiple callbacks
  GetPageContentCallback pending_get_page_content_callback_;

  std::unique_ptr<PrintPreviewExtractionDelegate>
      print_preview_extraction_delegate_;
  std::unique_ptr<PageContentFetcherDelegate> page_content_fetcher_delegate_;
  std::unique_ptr<PDFA11yInfoLoadObserver> pdf_load_observer_;
  base::OnceClosure on_pdf_a11y_info_loaded_cb_;

  // A scoper only used for PDF viewing.
  std::unique_ptr<content::ScopedAccessibilityMode> scoped_accessibility_mode_;

  mojo::AssociatedReceiver<mojom::PageContentExtractorHost>
      page_content_extractor_receiver_{this};

  base::WeakPtrFactory<AIChatTabHelper> weak_ptr_factory_{this};
                          
  WEB_STATE_USER_DATA_KEY_DECL();
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_API_AI_CHAT_AI_CHAT_TAB_HELPER_H_
