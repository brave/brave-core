// Copyright (c) 2025 The Brave Authors. All rights reserved.
// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/apple/foundation_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/notimplemented.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/common/ai_chat_urls.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/components/ai_chat/ios/browser/upload_file_helper.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ios/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/ai_chat/tab_data_web_state_observer.h"
#include "brave/ios/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/ios/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_communication_tab_helper+private.h"
#include "components/favicon/core/favicon_service.h"
#include "components/grit/brave_components_webui_strings.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser/browser.h"
#include "ios/chrome/browser/shared/model/browser/browser_list.h"
#include "ios/chrome/browser/shared/model/browser/browser_list_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/web_state_list/web_state_list.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_id.h"
#include "ui/base/l10n/l10n_util.h"
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
  new WaitForCommit(web_state, std::move(on_loaded));

  web_state->GetNavigationManager()->LoadIfNecessary();
}

web::WebState* GetTabFromId(ProfileIOS* profile, int tab_id) {
  return TabDataWebStateObserver::GetTabById(tab_id);
}

}  // namespace

using mojom::CharacterType;
using mojom::ConversationTurn;

AIChatUIPageHandler::ChatContextObserver::ChatContextObserver(
    web::WebState* web_state,
    AIChatUIPageHandler& page_handler)
    : web::WebStateObserver(),
      web_state_(web_state),
      page_handler_(page_handler) {
  web_state->AddObserver(this);
}

AIChatUIPageHandler::ChatContextObserver::~ChatContextObserver() {
  web_state_->RemoveObserver(this);
}

AIChatUIPageHandler::AIChatUIPageHandler(
    web::WebState* owner_web_state,
    web::WebState* chat_context_web_state,
    ProfileIOS* profile,
    mojo::PendingReceiver<ai_chat::mojom::AIChatUIHandler> receiver)
    : owner_web_state_(owner_web_state),
      profile_(profile),
      receiver_(this, std::move(receiver)),
      conversations_are_content_associated_(
          !features::IsAIChatGlobalSidePanelEverywhereEnabled()) {
  // Standalone mode means Chat is opened as its own tab in the tab strip and
  // not a side panel. chat_context_web_state is nullptr in that case
  const bool is_standalone = chat_context_web_state == nullptr;

  auto* profile_metrics =
      misc_metrics::ProfileMiscMetricsServiceFactory::GetForProfile(profile);

  if (profile_metrics) {
    ai_chat_metrics_ = profile_metrics->GetAIChatMetrics();
  }

  if (!is_standalone) {
    active_chat_tab_helper_ =
        ai_chat::AIChatTabHelper::FromWebState(chat_context_web_state);
    associated_content_delegate_observation_.Observe(active_chat_tab_helper_);
    chat_context_observer_ =
        std::make_unique<ChatContextObserver>(chat_context_web_state, *this);
  }
}

AIChatUIPageHandler::~AIChatUIPageHandler() = default;

void AIChatUIPageHandler::HandleVoiceRecognition(
    const std::string& conversation_uuid) {
  auto* web_state_to_navigate = (active_chat_tab_helper_)
                                    ? active_chat_tab_helper_->web_state()
                                    : owner_web_state_.get();

  auto* controller =
      [AIChatCommunicationController fromWebState:web_state_to_navigate];
  if (auto delegate = [controller delegate]) {
    [delegate handleVoiceRecognition:controller
                  withConversationId:base::SysUTF8ToNSString(conversation_uuid)
                          completion:^(NSString* prompt){
                              // TODO: Do something with the string???
                          }];
  }
}

void AIChatUIPageHandler::ShowSoftKeyboard() {}

void AIChatUIPageHandler::ProcessImageFile(
    const std::vector<uint8_t>& file_data,
    const std::string& filename,
    ProcessImageFileCallback callback) {
  ai_chat::UploadFileHelperIOS::ProcessImageData(
      file_data,
      base::BindOnce(
          [](const std::string& filename, ProcessImageFileCallback callback,
             std::optional<std::vector<uint8_t>> processed_data) {
            if (!processed_data) {
              std::move(callback).Run(nullptr);
              return;
            }
            auto uploaded_file = ai_chat::mojom::UploadedFile::New(
                filename, processed_data->size(), *processed_data,
                ai_chat::mojom::UploadedFileType::kImage);
            std::move(callback).Run(std::move(uploaded_file));
          },
          filename, std::move(callback)));
}

void AIChatUIPageHandler::UploadFile(bool use_media_capture,
                                     UploadFileCallback callback) {
  auto* web_state_to_navigate = (active_chat_tab_helper_)
                                    ? active_chat_tab_helper_->web_state()
                                    : owner_web_state_.get();

  auto* controller =
      [AIChatCommunicationController fromWebState:web_state_to_navigate];
  if (auto delegate = [controller delegate]) {
    auto cb = std::make_shared<UploadFileCallback>(std::move(callback));
    [delegate
        fetchImageForChatUpload:controller
                     completion:^(NSURL* url) {
                       if (!url) {
                         std::move(*cb).Run(std::nullopt);
                         return;
                       }

                       ai_chat::UploadFileHelperIOS::ProcessImageFileURL(
                           url,
                           base::BindOnce(
                               [](base::WeakPtr<AIChatUIPageHandler> weak_self,
                                  std::string file_name, UploadFileCallback cb,
                                  std::optional<std::vector<uint8_t>>
                                      image_data) {
                                 if (!weak_self || !image_data) {
                                   std::move(cb).Run(std::nullopt);
                                   return;
                                 }

                                 std::vector<ai_chat::mojom::UploadedFilePtr>
                                     images;
                                 images.push_back(
                                     ai_chat::mojom::UploadedFile::New(
                                         std::move(file_name),
                                         image_data->size(), *image_data,
                                         ai_chat::mojom::UploadedFileType::
                                             kImage));
                                 std::move(cb).Run(std::move(images));
                               },
                               weak_ptr_factory_.GetWeakPtr(),
                               base::apple::NSStringToFilePath([url path])
                                   .BaseName()
                                   .value(),
                               std::move(*cb)));
                     }];
  }
}

void AIChatUIPageHandler::GetPluralString(const std::string& key,
                                          int32_t count,
                                          GetPluralStringCallback callback) {
  auto iter = std::ranges::find(webui::kAiChatStrings, key,
                                &webui::LocalizedString::name);
  CHECK(iter != webui::kAiChatStrings.end());
  std::move(callback).Run(l10n_util::GetPluralStringFUTF8(iter->id, count));
}

void AIChatUIPageHandler::OpenAIChatSettings() {
  auto* web_state_to_navigate = (active_chat_tab_helper_)
                                    ? active_chat_tab_helper_->web_state()
                                    : owner_web_state_.get();

  auto* controller =
      [AIChatCommunicationController fromWebState:web_state_to_navigate];
  if (auto delegate = [controller delegate]) {
    [delegate openSettings:controller];
  }
}

void AIChatUIPageHandler::OpenMemorySettings() {
  NOTIMPLEMENTED();
}

void AIChatUIPageHandler::OpenConversationFullPage(
    const std::string& conversation_uuid) {
  CHECK(ai_chat::features::IsAIChatHistoryEnabled());
  CHECK(active_chat_tab_helper_);

  active_chat_tab_helper_->web_state()->OpenURL({
      ConversationUrl(conversation_uuid),
      web::Referrer(),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui::PAGE_TRANSITION_TYPED,
      false,
  });
}

void AIChatUIPageHandler::OpenAIChatAgentProfile() {
  CHECK(ai_chat::features::IsAIChatAgentProfileEnabled());
}

void AIChatUIPageHandler::OpenURL(const GURL& url) {
  if (!url.SchemeIs(kChromeUIScheme) && !url.SchemeIs(url::kHttpsScheme)) {
    return;
  }

  web::WebState* web_state_to_navigate =
      (active_chat_tab_helper_) ? active_chat_tab_helper_->web_state()
                                : owner_web_state_.get();
  web_state_to_navigate->OpenURL({url, url, web::Referrer(),
                                  WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                  ui::PAGE_TRANSITION_LINK, false});
}

void AIChatUIPageHandler::OpenStorageSupportUrl() {
  OpenURL(GURL(kURLLearnMoreAboutStorage));
}

void AIChatUIPageHandler::GoPremium() {
  auto* web_state_to_navigate = (active_chat_tab_helper_)
                                    ? active_chat_tab_helper_->web_state()
                                    : owner_web_state_.get();

  auto* controller =
      [AIChatCommunicationController fromWebState:web_state_to_navigate];
  if (auto delegate = [controller delegate]) {
    [delegate goPremium:controller];
  }
}

void AIChatUIPageHandler::RefreshPremiumSession() {
  OpenURL(GURL(kURLRefreshPremiumSession));
}

void AIChatUIPageHandler::ManagePremium() {
  auto* web_state_to_navigate = (active_chat_tab_helper_)
                                    ? active_chat_tab_helper_->web_state()
                                    : owner_web_state_.get();

  auto* controller =
      [AIChatCommunicationController fromWebState:web_state_to_navigate];
  if (auto delegate = [controller delegate]) {
    [delegate managePremium:controller];
  }
}

void AIChatUIPageHandler::OpenModelSupportUrl() {
  OpenURL(GURL(kLeoModelSupportUrl));
}

void AIChatUIPageHandler::ChatContextObserver::WebStateDestroyed(
    web::WebState* web_state) {
  page_handler_->HandleWebStateDestroyed();
}

void AIChatUIPageHandler::HandleWebStateDestroyed() {
  active_chat_tab_helper_ = nullptr;
  associated_content_delegate_observation_.Reset();
  chat_context_observer_.reset();
}

void AIChatUIPageHandler::OnRequestArchive(
    AssociatedContentDelegate* delegate) {
  chat_ui_->OnNewDefaultConversation(
      active_chat_tab_helper_
          ? std::make_optional(active_chat_tab_helper_->content_id())
          : std::nullopt);
}

// void AIChatUIPageHandler::OnFilesSelected() {
//   chat_ui_->OnUploadFilesSelected();
// }

void AIChatUIPageHandler::CloseUI() {
  // TODO: Somehow figure out which WebView to close??? Maybe easier to call
  // `window.close()` in Javascript???

  //  auto* web_state_to_navigate = (active_chat_tab_helper_)
  //                                    ? active_chat_tab_helper_->web_state()
  //                                    : owner_web_state_.get();
  //
  //  if (auto* tab_helper =
  //  BraveWebUIMessagingTabHelper::FromWebState(web_state_to_navigate)) {
  //    if (id<BraveWebUIMessagingTabHelperDelegate> delegate =
  //    tab_helper->GetBridgingDelegate()) {
  ////      [delegate aiChatCloseWebUI:nullptr];
  //    }
  //  }
}

void AIChatUIPageHandler::SetChatUI(mojo::PendingRemote<mojom::ChatUI> chat_ui,
                                    SetChatUICallback callback) {
  chat_ui_.Bind(std::move(chat_ui));
  std::move(callback).Run(active_chat_tab_helper_ == nullptr);

  chat_ui_->OnNewDefaultConversation(
      active_chat_tab_helper_
          ? std::make_optional(active_chat_tab_helper_->content_id())
          : std::nullopt);
}

void AIChatUIPageHandler::BindRelatedConversation(
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  if (!active_chat_tab_helper_ || !conversations_are_content_associated_) {
    ConversationHandler* conversation =
        AIChatServiceFactory::GetForProfile(profile_)->CreateConversation();
    conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
    return;
  }

  ConversationHandler* conversation =
      AIChatServiceFactory::GetForProfile(profile_)
          ->GetOrCreateConversationHandlerForContent(
              active_chat_tab_helper_->content_id(),
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

            AIChatServiceFactory::GetForProfile(
                ProfileIOS::FromBrowserState(web_state->GetBrowserState()))
                ->MaybeAssociateContent(tab_helper, conversation_uuid);
          },
          conversation_uuid));
}

void AIChatUIPageHandler::DisassociateContent(
    mojom::AssociatedContentPtr content,
    const std::string& conversation_uuid) {
  auto* service = AIChatServiceFactory::GetForProfile(profile_);
  service->DisassociateContent(content, conversation_uuid);
}

void AIChatUIPageHandler::NewConversation(
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  ConversationHandler* conversation;
  if (active_chat_tab_helper_ && conversations_are_content_associated_) {
    conversation = AIChatServiceFactory::GetForProfile(profile_)
                       ->CreateConversationHandlerForContent(
                           active_chat_tab_helper_->content_id(),
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
