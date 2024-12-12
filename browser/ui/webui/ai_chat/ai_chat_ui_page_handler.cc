// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/browser/ai_chat/ai_chat_urls.h"
#include "brave/browser/ui/side_panel/ai_chat/ai_chat_side_panel_utils.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "components/favicon/core/favicon_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/url_constants.h"
#include "ui/base/page_transition_types.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/ui/android/ai_chat/brave_leo_settings_launcher_helper.h"
#endif

namespace {
constexpr uint32_t kDesiredFaviconSizePixels = 32;
constexpr char kURLRefreshPremiumSession[] =
    "https://account.brave.com/?intent=recover&product=leo";

#if !BUILDFLAG(IS_ANDROID)
constexpr char kURLGoPremium[] =
    "https://account.brave.com/account/?intent=checkout&product=leo";
constexpr char kURLManagePremium[] = "https://account.brave.com/";
#endif
}  // namespace

namespace ai_chat {

using mojom::CharacterType;
using mojom::ConversationTurn;
using mojom::ConversationTurnVisibility;

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
  favicon_service_ = FaviconServiceFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);
  const bool is_standalone = chat_context_web_contents == nullptr;
  if (!is_standalone) {
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

#if !BUILDFLAG(IS_ANDROID)
  content::WebContents* contents_to_navigate =
      (active_chat_tab_helper_) ? active_chat_tab_helper_->web_contents()
                                : owner_web_contents_.get();
  contents_to_navigate->OpenURL(
      {url, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
       ui::PAGE_TRANSITION_LINK, false},
      /*navigation_handle_callback=*/{});
#else
  // We handle open link different on Android as we need to close the chat
  // window because it's always full screen
  ai_chat::OpenURL(url.spec());
#endif
}

void AIChatUIPageHandler::GoPremium() {
#if !BUILDFLAG(IS_ANDROID)
  OpenURL(GURL(kURLGoPremium));
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

void AIChatUIPageHandler::GetFaviconImageData(
    const std::string& conversation_id,
    GetFaviconImageDataCallback callback) {
  ConversationHandler* conversation =
      AIChatServiceFactory::GetForBrowserContext(profile_)->GetConversation(
          conversation_id);
  if (!conversation) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  conversation->GetAssociatedContentInfo(base::BindOnce(
      &AIChatUIPageHandler::GetFaviconImageDataForAssociatedContent,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void AIChatUIPageHandler::BindParentUIFrameFromChildFrame(
    mojo::PendingReceiver<mojom::ParentUIFrame> receiver) {
  chat_ui_->OnChildFrameBound(std::move(receiver));
}

void AIChatUIPageHandler::GetFaviconImageDataForAssociatedContent(
    GetFaviconImageDataCallback callback,
    mojom::SiteInfoPtr content_info,
    bool should_send_page_contents) {
  if (!content_info->is_content_association_possible ||
      !content_info->url.has_value() || !content_info->url->is_valid()) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  favicon_base::IconTypeSet icon_types{favicon_base::IconType::kFavicon,
                                       favicon_base::IconType::kTouchIcon};

  auto on_favicon_available =
      [](GetFaviconImageDataCallback callback,
         const favicon_base::FaviconRawBitmapResult& result) {
        if (!result.is_valid()) {
          std::move(callback).Run(std::nullopt);
          return;
        }

        std::vector<uint8_t> bytes(result.bitmap_data->begin(),
                                   result.bitmap_data->end());
        std::move(callback).Run(std::move(bytes));
      };

  favicon_service_->GetRawFaviconForPageURL(
      content_info->url.value(), icon_types, kDesiredFaviconSizePixels, true,
      base::BindOnce(on_favicon_available, std::move(callback)),
      &favicon_task_tracker_);
}

}  // namespace ai_chat
