// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ios/browser/api/ai_chat/ai_chat_service_factory.h"
#include "components/favicon/core/favicon_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser/browser.h"
#include "ios/chrome/browser/shared/model/browser/browser_list.h"
#include "ios/chrome/browser/shared/model/browser/browser_list_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_id.h"
#include "ui/base/page_transition_types.h"

namespace {
constexpr char kURLRefreshPremiumSession[] =
    "https://account.brave.com/?intent=recover&product=leo";
constexpr char kURLLearnMoreAboutStorage[] =
    "https://support.brave.com/hc/en-us/articles/"
    "32663367857549-How-do-I-use-Chat-History-in-Brave-Leo";
}  // namespace

namespace ai_chat {

namespace {

// Invokes a callback when the WebState has finished loading. Note: If the
// WebState is destroyed before loading is completed, the callback will not
// be invoked.
// The lifetime of this class is tied to the WebState it is observing - it
// will be destroyed when |WebWebStateDestroyed| is called, or when the
// Navigation finishes, whichever happens first.
class WaitForCommit : public web::WebStateObserver {
 public:
  WaitForCommit(web::WebState* web_state,
                base::OnceCallback<void(web::WebState* web_state)> on_loaded)
      : web::WebStateObserver(), on_loaded_(std::move(on_loaded)) {
    web_state->AddObserver(this);
  }
  ~WaitForCommit() override = default;

  void DidFinishNavigation(
      web::WebState* web_state,
      web::NavigationContext* navigation_context) override {
    if (navigation_context->HasCommitted()) {  // TODO: Not sure how to check
                                               // main frame here...
      std::move(on_loaded_).Run(web_state);
      delete this;
    }
  }

  void WebStateDestroyed(web::WebState* web_state) override { delete this; }

 private:
  base::OnceCallback<void(web::WebState* web_state)> on_loaded_;
};

// Note: we need to ensure the WebState is loaded before associating content
// with a conversation.
void EnsureWebStateLoaded(
    web::WebState* web_state,
    base::OnceCallback<void(web::WebState* web_state)> on_loaded) {
  if (!web_state->IsLoading()) {
    std::move(on_loaded).Run(web_state);
    return;
  }

  // Deletes when the load completes or the WebContents is destroyed
  new WaitForCommit(
      web_state,
      std::move(
          on_loaded));  // Maybe we should use WebStateObserver::DidStartLoading
                        // to determine when the load has finished :S
  //  contents->GetController().LoadIfNecessary();   // Can't be done on iOS???
}

web::WebState* GetTabFromId(ProfileIOS* profile, int tab_id) {
  BrowserList* browser_list =
      BrowserListFactory::GetForProfile(profile);

  for (Browser* browser : browser_list->BrowsersOfType(
           BrowserList::BrowserType::kRegular)) {  // kAll ???
    WebStateList* web_state_list = browser->GetWebStateList();
   if (web_state_list->count() > tab_id) {
     return web_state_list->GetWebStateAt(tab_id);
   }
             
//    for (int i = 0; i < web_state_list->count(); ++i) {
//      auto* tab = web_state_list->GetWebStateAt(i);
//      if (tab_id == tab->GetUniqueIdentifier()) {
//        return tab;
//      }
    }
  return nullptr;
}

}  // namespace

using mojom::CharacterType;
using mojom::ConversationTurn;

AIChatUIPageHandler::ChatContextObserver::ChatContextObserver(
    web::WebState* web_state,
    AIChatUIPageHandler& page_handler)
    : web::WebStateObserver(), page_handler_(page_handler) {
  web_state->AddObserver(this);
}

AIChatUIPageHandler::ChatContextObserver::~ChatContextObserver() = default;

AIChatUIPageHandler::AIChatUIPageHandler(
    web::WebState* owner_web_state,
    web::WebState* chat_context_web_state,
    ProfileIOS* profile,
    mojo::PendingReceiver<ai_chat::mojom::AIChatUIHandler> receiver)
    : owner_web_state_(owner_web_state),
      profile_(profile),
      ai_chat_metrics_(std::make_unique<AIChatMetrics>(
          GetApplicationContext()->GetLocalState(),
          nullptr)),
      receiver_(this, std::move(receiver)) {
  // Standalone mode means Chat is opened as its own tab in the tab strip and
  // not a side panel. chat_context_web_state is nullptr in that case
  const bool is_standalone = chat_context_web_state == nullptr;

  if (!is_standalone) {
    active_chat_tab_helper_ =
        ai_chat::AIChatTabHelper::FromWebState(chat_context_web_state);
    chat_tab_helper_observation_.Observe(active_chat_tab_helper_);
    chat_context_observer_ =
        std::make_unique<ChatContextObserver>(chat_context_web_state, *this);
  }
}

AIChatUIPageHandler::~AIChatUIPageHandler() = default;

void AIChatUIPageHandler::HandleVoiceRecognition(
    const std::string& conversation_uuid) {
  //  ai_chat::HandleVoiceRecognition(owner_web_state_.get(),
  //  conversation_uuid);
}

void AIChatUIPageHandler::ShowSoftKeyboard() {
  //  ai_chat::HandleShowSoftKeyboard(owner_web_state_.get());  // TODO: Fix
}

void AIChatUIPageHandler::UploadImage(bool use_media_capture,
                                      UploadImageCallback callback) {}

void AIChatUIPageHandler::OpenAIChatSettings() {
  // TODO: Fix
//  web::WebState* web_state_to_navigate =
//      (active_chat_tab_helper_) ? active_chat_tab_helper_->web_state()
//                                : owner_web_state_.get();
//  ai_chat::ShowBraveLeoSettings(web_state_to_navigate);
}

void AIChatUIPageHandler::OpenConversationFullPage(
    const std::string& conversation_uuid) {
  CHECK(ai_chat::features::IsAIChatHistoryEnabled());
  CHECK(active_chat_tab_helper_);

  // TODO: Fix
  
//  active_chat_tab_helper_->web_state()->OpenURL(
//      {
//          ConversationUrl(conversation_uuid),
//          web::Referrer(),
//          WindowOpenDisposition::NEW_FOREGROUND_TAB,
//          ui::PAGE_TRANSITION_TYPED,
//          false,
//      },
//      {});
}

void AIChatUIPageHandler::OpenURL(const GURL& url) {
  if (!url.SchemeIs(kChromeUIScheme) &&
      !url.SchemeIs(url::kHttpsScheme)) {
    return;
  }

  web::WebState* web_state_to_navigate =
      (active_chat_tab_helper_) ? active_chat_tab_helper_->web_state()
                                : owner_web_state_.get();
  web_state_to_navigate->OpenURL(
      {url, url, web::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
       ui::PAGE_TRANSITION_LINK, false});
}

void AIChatUIPageHandler::OpenStorageSupportUrl() {
  OpenURL(GURL(kURLLearnMoreAboutStorage));
}

void AIChatUIPageHandler::GoPremium() {
  // TODO: Fix
//  auto* web_state_to_navigate = (active_chat_tab_helper_)
//                                    ? active_chat_tab_helper_->web_state()
//                                    : owner_web_state_.get();
//  ai_chat::GoPremium(web_state_to_navigate);
}

void AIChatUIPageHandler::RefreshPremiumSession() {
  OpenURL(GURL(kURLRefreshPremiumSession));
}

void AIChatUIPageHandler::ManagePremium() {
//  auto* web_state_to_navigate = (active_chat_tab_helper_)
//                                    ? active_chat_tab_helper_->web_state()
//                                    : owner_web_state_.get();
//  ai_chat::ManagePremium(web_state_to_navigate);
}

void AIChatUIPageHandler::OpenModelSupportUrl() {
//  OpenURL(GURL(kLeoModelSupportUrl));
}

void AIChatUIPageHandler::ChatContextObserver::WebStateDestroyed(
    web::WebState* web_state) {
  page_handler_->HandleWebStateDestroyed();
}

void AIChatUIPageHandler::HandleWebStateDestroyed() {
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
  // TODO: Fix
//  ai_chat::CloseActivity(owner_web_state_);
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
        AIChatServiceFactory::GetForProfile(profile_)->CreateConversation();
    conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
    return;
  }

  ConversationHandler* conversation =
      AIChatServiceFactory::GetForProfile(profile_)
          ->GetOrCreateConversationHandlerForContent(
              active_chat_tab_helper_->GetContentId(),
              active_chat_tab_helper_->GetWeakPtr());

  conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
}

void AIChatUIPageHandler::AssociateTab(mojom::TabDataPtr mojom_tab,
                                       const std::string& conversation_uuid) {
  auto* web_state = GetTabFromId(profile_, mojom_tab->id);
  
  if (!web_state) {
    return;
  }

  EnsureWebStateLoaded(
      web_state,
      base::BindOnce(
          [](const std::string& conversation_uuid, web::WebState* web_state) {
            auto* tab_helper =
                ai_chat::AIChatTabHelper::FromWebState(web_state);
            if (!tab_helper) {
              return;
            }

            AIChatServiceFactory::GetForProfile(ProfileIOS::FromBrowserState(web_state->GetBrowserState()))
                ->AssociateContent(tab_helper, conversation_uuid);
          },
          conversation_uuid));
}

void AIChatUIPageHandler::NewConversation(
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  ConversationHandler* conversation;
  if (active_chat_tab_helper_) {
    conversation = AIChatServiceFactory::GetForProfile(profile_)
                       ->CreateConversationHandlerForContent(
                           active_chat_tab_helper_->GetContentId(),
                           active_chat_tab_helper_->GetWeakPtr());
  } else {
    conversation =
        AIChatServiceFactory::GetForProfile(profile_)->CreateConversation();
  }

  conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
}

void AIChatUIPageHandler::BindParentUIFrameFromChildFrame(
    mojo::PendingReceiver<mojom::ParentUIFrame> receiver) {
  chat_ui_->OnChildFrameBound(std::move(receiver));
}

}  // namespace ai_chat
