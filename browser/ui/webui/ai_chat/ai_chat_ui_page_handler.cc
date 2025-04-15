// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ai_chat/ai_chat_urls.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "chrome/browser/ui/tabs/public/tab_interface.h"
#include "components/favicon/core/favicon_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/url_constants.h"
#include "ui/base/page_transition_types.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/android/ai_chat/brave_leo_settings_launcher_helper.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/tab_model/tab_model.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#endif

namespace {
constexpr char kURLRefreshPremiumSession[] =
    "https://account.brave.com/?intent=recover&product=leo";
constexpr char kURLLearnMoreAboutStorage[] =
    "https://support.brave.com/hc/en-us/articles/"
    "32663367857549-How-do-I-use-Chat-History-in-Brave-Leo";

#if !BUILDFLAG(IS_ANDROID)
constexpr char kURLManagePremium[] = "https://account.brave.com/";
#endif
}  // namespace

namespace ai_chat {

namespace {

// Invokes a callback when the WebContents has finished loading. Note: If the
// WebContents is destroyed before loading is completed, the callback will not
// be invoked.
// The lifetime of this class is tied to the WebContents it is observing - it
// will be destroyed when |WebContentsDestroyed| is called, or when the
// Navigation finishes, whichever happens first.
class WaitForCommit : public content::WebContentsObserver {
 public:
  WaitForCommit(
      content::WebContents* contents,
      base::OnceCallback<void(content::WebContents* contents)> on_loaded)
      : WebContentsObserver(contents), on_loaded_(std::move(on_loaded)) {}
  ~WaitForCommit() override = default;

  void DidFinishNavigation(content::NavigationHandle* handle) override {
    if (handle->IsInMainFrame() && handle->HasCommitted()) {
      std::move(on_loaded_).Run(web_contents());
      delete this;
    }
  }

  void WebContentsDestroyed() override { delete this; }

 private:
  base::OnceCallback<void(content::WebContents* contents)> on_loaded_;
};

// Note: we need to ensure the WebContents is loaded before associating content
// with a conversation.
void EnsureWebContentsLoaded(
    content::WebContents* contents,
    base::OnceCallback<void(content::WebContents* contents)> on_loaded) {
  if (!contents->GetController().NeedsReload()) {
    std::move(on_loaded).Run(contents);
    return;
  }

  // Deletes when the load completes or the WebContents is destroyed
  new WaitForCommit(contents, std::move(on_loaded));
  contents->GetController().LoadIfNecessary();
}

#if BUILDFLAG(IS_ANDROID)
TabAndroid* GetAndroidTabFromId(int32_t tab_id) {
  for (TabModel* model : TabModelList::models()) {
    const size_t tab_count = model->GetTabCount();
    for (size_t index = 0; index < tab_count; index++) {
      auto* tab = model->GetTabAt(index);
      if (tab_id == tab->GetAndroidId()) {
        return tab;
      }
    }
  }
  return nullptr;
}
#endif

}  // namespace

using mojom::CharacterType;
using mojom::ConversationTurn;

AIChatUIPageHandler::ChatContextObserver::ChatContextObserver(
    content::WebContents* web_contents,
    AIChatUIPageHandler& page_handler)
    : content::WebContentsObserver(web_contents), page_handler_(page_handler) {}

AIChatUIPageHandler::ChatContextObserver::~ChatContextObserver() = default;

AIChatUIPageHandler::AIChatUIPageHandler(
    content::WebContents* owner_web_contents,
    content::WebContents* chat_context_web_contents,
    Profile* profile,
    mojo::PendingReceiver<ai_chat::mojom::AIChatUIHandler> receiver)
    : owner_web_contents_(owner_web_contents),
      profile_(profile),
      receiver_(this, std::move(receiver)) {
  // Standalone mode means Chat is opened as its own tab in the tab strip and
  // not a side panel. chat_context_web_contents is nullptr in that case
  const bool is_standalone = chat_context_web_contents == nullptr;
  auto* profile_metrics =
      misc_metrics::ProfileMiscMetricsServiceFactory::GetServiceForContext(
          profile);
  if (profile_metrics) {
    ai_chat_metrics_ = profile_metrics->GetAIChatMetrics();
  }
  if (!is_standalone) {
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
    if (ai_chat_metrics_) {
      ai_chat_metrics_->RecordSidebarUsage();
    }
#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
    active_chat_tab_helper_ =
        ai_chat::AIChatTabHelper::FromWebContents(chat_context_web_contents);
    chat_tab_helper_observation_.Observe(active_chat_tab_helper_);
    chat_context_observer_ =
        std::make_unique<ChatContextObserver>(chat_context_web_contents, *this);
  }
}

AIChatUIPageHandler::~AIChatUIPageHandler() = default;

void AIChatUIPageHandler::HandleVoiceRecognition(
    const std::string& conversation_uuid) {
#if BUILDFLAG(IS_ANDROID)
  ai_chat::HandleVoiceRecognition(owner_web_contents_.get(), conversation_uuid);
#endif
}

void AIChatUIPageHandler::ShowSoftKeyboard() {
#if BUILDFLAG(IS_ANDROID)
  ai_chat::HandleShowSoftKeyboard(owner_web_contents_.get());
#endif
}

void AIChatUIPageHandler::UploadImage(bool use_media_capture,
                                      UploadImageCallback callback) {
  if (!upload_file_helper_) {
    upload_file_helper_ =
        std::make_unique<UploadFileHelper>(owner_web_contents_, profile_);
  }
  upload_file_helper_->UploadImage(
      std::make_unique<ChromeSelectFilePolicy>(owner_web_contents_),
#if BUILDFLAG(IS_ANDROID)
      use_media_capture,
#endif
      std::move(callback));
}

void AIChatUIPageHandler::OpenAIChatSettings() {
  content::WebContents* contents_to_navigate =
      (active_chat_tab_helper_) ? active_chat_tab_helper_->web_contents()
                                : owner_web_contents_.get();
#if !BUILDFLAG(IS_ANDROID)
  const GURL url("brave://settings/leo-ai");
  if (auto* browser = chrome::FindBrowserWithTab(contents_to_navigate)) {
    ShowSingletonTab(browser, url);
  } else {
    contents_to_navigate->OpenURL(
        {url, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
         ui::PAGE_TRANSITION_LINK, false},
        /*navigation_handle_callback=*/{});
  }
#else
  ai_chat::ShowBraveLeoSettings(contents_to_navigate);
#endif
}

void AIChatUIPageHandler::OpenConversationFullPage(
    const std::string& conversation_uuid) {
  CHECK(ai_chat::features::IsAIChatHistoryEnabled());
  CHECK(active_chat_tab_helper_);
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  if (ai_chat_metrics_) {
    ai_chat_metrics_->RecordFullPageSwitch();
  }
#endif
  active_chat_tab_helper_->web_contents()->OpenURL(
      {
          ConversationUrl(conversation_uuid),
          content::Referrer(),
          WindowOpenDisposition::NEW_FOREGROUND_TAB,
          ui::PAGE_TRANSITION_TYPED,
          false,
      },
      {});
}

void AIChatUIPageHandler::OpenURL(const GURL& url) {
  if (!url.SchemeIs(content::kChromeUIScheme) &&
      !url.SchemeIs(url::kHttpsScheme)) {
    return;
  }

  content::WebContents* contents_to_navigate =
      (active_chat_tab_helper_) ? active_chat_tab_helper_->web_contents()
                                : owner_web_contents_.get();
  contents_to_navigate->OpenURL(
      {url, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
       ui::PAGE_TRANSITION_LINK, false},
      /*navigation_handle_callback=*/{});
}

void AIChatUIPageHandler::OpenStorageSupportUrl() {
  OpenURL(GURL(kURLLearnMoreAboutStorage));
}

void AIChatUIPageHandler::GoPremium() {
#if !BUILDFLAG(IS_ANDROID)
  OpenURL(GURL(kLeoGoPremiumUrl));
#else
  auto* contents_to_navigate = (active_chat_tab_helper_)
                                   ? active_chat_tab_helper_->web_contents()
                                   : owner_web_contents_.get();
  ai_chat::GoPremium(contents_to_navigate);
#endif
}

void AIChatUIPageHandler::RefreshPremiumSession() {
  OpenURL(GURL(kURLRefreshPremiumSession));
}

void AIChatUIPageHandler::ManagePremium() {
#if !BUILDFLAG(IS_ANDROID)
  OpenURL(GURL(kURLManagePremium));
#else
  auto* contents_to_navigate = (active_chat_tab_helper_)
                                   ? active_chat_tab_helper_->web_contents()
                                   : owner_web_contents_.get();
  ai_chat::ManagePremium(contents_to_navigate);
#endif
}

void AIChatUIPageHandler::OpenModelSupportUrl() {
  OpenURL(GURL(kLeoModelSupportUrl));
}

void AIChatUIPageHandler::ChatContextObserver::WebContentsDestroyed() {
  page_handler_->HandleWebContentsDestroyed();
}

void AIChatUIPageHandler::HandleWebContentsDestroyed() {
  active_chat_tab_helper_ = nullptr;
  chat_tab_helper_observation_.Reset();
  chat_context_observer_.reset();
}

void AIChatUIPageHandler::OnAssociatedContentNavigated(int new_navigation_id) {
  // This is only applicable to content-adjacent UI, e.g. SidePanel on Desktop
  // where it would like to remain associated with the Tab and move away from
  // Conversations of previous navigations. That doens't apply to the standalone
  // UI where it will keep a previous navigation's conversation active.
  chat_ui_->OnNewDefaultConversation();
}
void AIChatUIPageHandler::CloseUI() {
#if !BUILDFLAG(IS_ANDROID)
  ai_chat::ClosePanel(owner_web_contents_);
#else
  ai_chat::CloseActivity(owner_web_contents_);
#endif
}

void AIChatUIPageHandler::SetChatUI(mojo::PendingRemote<mojom::ChatUI> chat_ui,
                                    SetChatUICallback callback) {
  chat_ui_.Bind(std::move(chat_ui));
  std::move(callback).Run(active_chat_tab_helper_ == nullptr);
}

void AIChatUIPageHandler::BindRelatedConversation(
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  if (!active_chat_tab_helper_) {
    ConversationHandler* conversation =
        AIChatServiceFactory::GetForBrowserContext(profile_)
            ->CreateConversation();
    conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
    return;
  }

  ConversationHandler* conversation =
      AIChatServiceFactory::GetForBrowserContext(profile_)
          ->GetOrCreateConversationHandlerForContent(
              active_chat_tab_helper_->GetContentId(),
              active_chat_tab_helper_->GetWeakPtr());

  conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
}

void AIChatUIPageHandler::AssociateTab(mojom::TabDataPtr mojom_tab,
                                       const std::string& conversation_uuid) {
#if BUILDFLAG(IS_ANDROID)
  auto* tab = GetAndroidTabFromId(mojom_tab->id);
  if (!tab) {
    return;
  }

  auto* contents = tab->web_contents();
#else
  const auto* tab = tabs::TabInterface::Handle(mojom_tab->id).Get();
  if (!tab) {
    return;
  }
  auto* contents = tab->GetContents();
#endif

  if (!contents) {
    return;
  }

  EnsureWebContentsLoaded(
      contents, base::BindOnce(
                    [](const std::string& conversation_uuid,
                       content::WebContents* contents) {
                      auto* tab_helper =
                          ai_chat::AIChatTabHelper::FromWebContents(contents);
                      if (!tab_helper) {
                        return;
                      }

                      AIChatServiceFactory::GetForBrowserContext(
                          contents->GetBrowserContext())
                          ->AssociateContent(tab_helper, conversation_uuid);
                    },
                    conversation_uuid));
}

void AIChatUIPageHandler::NewConversation(
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  ConversationHandler* conversation;
  if (active_chat_tab_helper_) {
    conversation = AIChatServiceFactory::GetForBrowserContext(profile_)
                       ->CreateConversationHandlerForContent(
                           active_chat_tab_helper_->GetContentId(),
                           active_chat_tab_helper_->GetWeakPtr());
  } else {
    conversation = AIChatServiceFactory::GetForBrowserContext(profile_)
                       ->CreateConversation();
  }

  conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
}

void AIChatUIPageHandler::BindParentUIFrameFromChildFrame(
    mojo::PendingReceiver<mojom::ParentUIFrame> receiver) {
  chat_ui_->OnChildFrameBound(std::move(receiver));
}

}  // namespace ai_chat
