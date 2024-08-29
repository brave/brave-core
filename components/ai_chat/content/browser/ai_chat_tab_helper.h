/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "brave/components/ai_chat/core/browser/conversation_driver.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "components/favicon/core/favicon_driver_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

class PrefService;

namespace content {
class ScopedAccessibilityMode;
}

class AIChatUIBrowserTest;
namespace ai_chat {
class AIChatMetrics;

// Provides context to an AI Chat conversation in the form of the Tab's content
class AIChatTabHelper : public content::WebContentsObserver,
                        public content::WebContentsUserData<AIChatTabHelper>,
                        public mojom::PageContentExtractorHost,
                        public favicon::FaviconDriverObserver,
                        public ConversationDriver {
 public:
  static void BindPageContentExtractorHost(
      content::RenderFrameHost* rfh,
      mojo::PendingAssociatedReceiver<mojom::PageContentExtractorHost>
          receiver);

  // Maximum length can be exceeded, the length will determine that no more
  // pages will be processed in print preview. Ex. if we reach the maximum
  // length in the middle of the 5th page, the 5th page will be the last page
  // and the rest of the pages will be ignored.
  static void SetMaxContentLengthForTesting(std::optional<uint32_t> max_length);

  AIChatTabHelper(const AIChatTabHelper&) = delete;
  AIChatTabHelper& operator=(const AIChatTabHelper&) = delete;
  ~AIChatTabHelper() override;

  void SetOnPDFA11yInfoLoadedCallbackForTesting(base::OnceClosure cb);

  // mojom::PageContentExtractorHost
  void OnInterceptedPageContentChanged() override;

  // This will be called when print preview has been composited into image per
  // page and finish OCR.
  void OnPreviewTextReady(std::string ocr_text);

  uint32_t GetMaxPageContentLength();

 private:
  friend class content::WebContentsUserData<AIChatTabHelper>;
  friend class ::AIChatUIBrowserTest;

  // To observe PDF InnerWebContents for "Finished loading PDF" event which
  // means PDF content has been loaded to an accessibility tree.
  class PDFA11yInfoLoadObserver : public content::WebContentsObserver {
   public:
    ~PDFA11yInfoLoadObserver() override;
    explicit PDFA11yInfoLoadObserver(content::WebContents* web_contents,
                                     AIChatTabHelper* helper);

   private:
    void AccessibilityEventReceived(
        const ui::AXUpdatesAndEvents& details) override;
    raw_ptr<AIChatTabHelper> helper_;
  };

  AIChatTabHelper(
      content::WebContents* web_contents,
      AIChatMetrics* ai_chat_metrics,
      base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
          skus_service_getter,
      PrefService* local_state_prefs,
      const std::string& channel_name);

  void OnPDFA11yInfoLoaded();

  // content::WebContentsObserver
  void WebContentsDestroyed() override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void TitleWasSet(content::NavigationEntry* entry) override;
  void InnerWebContentsAttached(content::WebContents* inner_web_contents,
                                content::RenderFrameHost* render_frame_host,
                                bool is_full_page) override;
  void OnWebContentsFocused(
      content::RenderWidgetHost* render_widget_host) override;
  void OnWebContentsLostFocus(
      content::RenderWidgetHost* render_widget_host) override;

  // favicon::FaviconDriverObserver
  void OnFaviconUpdated(favicon::FaviconDriver* favicon_driver,
                        NotificationIconType notification_icon_type,
                        const GURL& icon_url,
                        bool icon_url_changed,
                        const gfx::Image& image) override;

  // ai_chat::ConversationDriver
  GURL GetPageURL() const override;
  void GetPageContent(GetPageContentCallback callback,
                      std::string_view invalidation_token) override;
  void PrintPreviewFallback(GetPageContentCallback callback) override;
  std::u16string GetPageTitle() const override;
  void FetchSearchQuerySummary(
      FetchSearchQuerySummaryCallback callback) override;

  void OnSearchSummarizerKeyFetched(FetchSearchQuerySummaryCallback callback,
                                    const std::optional<std::string>& key);
  void OnSearchQuerySummaryFetched(FetchSearchQuerySummaryCallback callback,
                                   api_request_helper::APIRequestResult result);

  void BindPageContentExtractorReceiver(
      mojo::PendingAssociatedReceiver<mojom::PageContentExtractorHost>
          receiver);

  // Traverse through a11y tree to check existence of status node.
  void CheckPDFA11yTree();

  raw_ptr<AIChatMetrics> ai_chat_metrics_;

  bool is_same_document_navigation_ = false;
  int64_t pending_navigation_id_;
  bool is_pdf_a11y_info_loaded_ = false;
  uint8_t check_pdf_a11y_tree_attempts_ = 0;
  GetPageContentCallback pending_get_page_content_callback_;

  std::unique_ptr<PDFA11yInfoLoadObserver> pdf_load_observer_;
  base::OnceClosure on_pdf_a11y_info_loaded_cb_;

  // A scoper only used for PDF viewing.
  std::unique_ptr<content::ScopedAccessibilityMode> scoped_accessibility_mode_;

  // Used for fetching search query summary.
  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;

  mojo::AssociatedReceiver<mojom::PageContentExtractorHost>
      page_content_extractor_receiver_{this};

  base::WeakPtrFactory<AIChatTabHelper> weak_ptr_factory_{this};
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_
