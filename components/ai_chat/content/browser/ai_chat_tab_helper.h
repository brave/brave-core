/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "brave/components/ai_chat/core/browser/conversation_driver.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "components/favicon/core/favicon_driver_observer.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

class PrefService;

namespace ai_chat {
class AIChatMetrics;

// Provides context to an AI Chat conversation in the form of the Tab's content
class AIChatTabHelper : public content::WebContentsObserver,
                        public content::WebContentsUserData<AIChatTabHelper>,
                        public favicon::FaviconDriverObserver,
                        public ConversationDriver {
 public:
  AIChatTabHelper(const AIChatTabHelper&) = delete;
  AIChatTabHelper& operator=(const AIChatTabHelper&) = delete;
  ~AIChatTabHelper() override;

  void SetOnPDFA11yInfoLoadedCallbackForTesting(base::OnceClosure cb);

 private:
  friend class content::WebContentsUserData<AIChatTabHelper>;

  // To observe PDF InnerWebContents for "Finished loading PDF" event which
  // means PDF content has been loaded to an accessibility tree.
  class PDFA11yInfoLoadObserver : public content::WebContentsObserver {
   public:
    ~PDFA11yInfoLoadObserver() override;
    explicit PDFA11yInfoLoadObserver(content::WebContents* web_contents,
                                     AIChatTabHelper* helper);

   private:
    void AccessibilityEventReceived(
        const content::AXEventNotificationDetails& details) override;
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
  std::u16string GetPageTitle() const override;

  raw_ptr<AIChatMetrics> ai_chat_metrics_;

  bool is_same_document_navigation_ = false;
  int64_t pending_navigation_id_;
  bool is_pdf_a11y_info_loaded_ = false;
  GetPageContentCallback pending_get_page_content_callback_;

  std::unique_ptr<PDFA11yInfoLoadObserver> pdf_load_observer_;
  base::OnceClosure on_pdf_a11y_info_loaded_cb_;

  base::WeakPtrFactory<AIChatTabHelper> weak_ptr_factory_{this};
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_AI_CHAT_TAB_HELPER_H_
