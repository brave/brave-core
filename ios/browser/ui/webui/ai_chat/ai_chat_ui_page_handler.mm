// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/apple/foundation_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/notimplemented.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/ai_chat/core/browser/ai_chat_metrics.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/constants.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/ai_chat_urls.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/ios/common.mojom.objc+private.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "brave/components/ai_chat/ios/browser/ai_chat_tab_helper.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ios/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/ai_chat/ai_chat_ui_handler_bridge.h"
#include "brave/ios/browser/ai_chat/ai_chat_ui_handler_bridge_holder.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"
#include "brave/ios/browser/api/web_view/brave_web_view.h"
#include "brave/ios/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/ios/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/ios/browser/ui/webui/ai_chat/associated_url_content.h"
#include "components/grit/brave_components_webui_strings.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_id.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web_view/internal/cwv_web_view_internal.h"
#include "net/base/apple/url_conversions.h"
#include "skia/ext/skia_utils_ios.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"
#include "ui/gfx/codec/png_codec.h"

namespace ai_chat {

namespace {

void DecodeAndScaleImage(
    const std::vector<uint8_t>& file_data,
    base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> callback) {
  NSData* ns_data = [NSData dataWithBytes:file_data.data()
                                   length:file_data.size()];
  auto encode_image = base::BindOnce(
      [](NSData* data) -> std::optional<std::vector<uint8_t>> {
        CGSize target_size = CGSizeMake(1024, 768);
        UIImage* image = [[UIImage alloc] initWithData:data];
        if (image.size.width > target_size.width &&
            image.size.height > target_size.height) {
          image = [image imageByPreparingThumbnailOfSize:target_size];
        }
        if (!image) {
          return std::nullopt;
        }
        NSData* png_data = UIImagePNGRepresentation(image);
        if (!png_data) {
          return std::nullopt;
        }
        auto png_span = base::apple::NSDataToSpan(png_data);
        return std::vector<uint8_t>(png_span.begin(), png_span.end());
      },
      ns_data);

  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(encode_image),
                                               std::move(callback));
}

}  // namespace

AIChatUIPageHandler::ChatContextObserver::ChatContextObserver(
    web::WebState* web_state,
    AIChatUIPageHandler& page_handler)
    : web_state_(web_state), page_handler_(page_handler) {
  web_state_->AddObserver(this);
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
  if (!is_standalone) {
    ai_chat_tab_helper_ =
        ai_chat::AIChatTabHelper::FromWebState(chat_context_web_state);
    associated_content_delegate_observation_.Observe(ai_chat_tab_helper_);
    chat_context_observer_ =
        std::make_unique<ChatContextObserver>(chat_context_web_state, *this);
  }
}

AIChatUIPageHandler::~AIChatUIPageHandler() = default;

void AIChatUIPageHandler::HandleVoiceRecognition(
    const std::string& conversation_uuid) {
  if (conversation_uuid.empty()) {
    return;
  }
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::FromWebState(owner_web_state_)->bridge();
  auto callback = base::CallbackToBlock(
      base::BindOnce(&AIChatUIPageHandler::SubmitVoiceQuery,
                     weak_ptr_factory_.GetWeakPtr(), conversation_uuid));
  [bridge handleVoiceRecognitionRequest:callback];
}

void AIChatUIPageHandler::SubmitVoiceQuery(const std::string& conversation_uuid,
                                           NSString* query) {
  if (!query || [query length] == 0) {
    return;
  }
  ConversationHandler* conversation =
      AIChatServiceFactory::GetForProfile(profile_)->GetConversation(
          conversation_uuid);
  if (!conversation) {
    return;
  }
  // Send the query
  conversation->MaybeUnlinkAssociatedContent();
  mojom::ConversationTurnPtr turn = mojom::ConversationTurn::New(
      std::nullopt, mojom::CharacterType::HUMAN, mojom::ActionType::QUERY,
      base::SysNSStringToUTF8(query), std::nullopt /* prompt */,
      std::nullopt /* selected_text */, std::nullopt /* events */,
      base::Time::Now(), std::nullopt, std::nullopt /* uploaded images */,
      nullptr /* skill */, false, std::nullopt /* model_key */,
      nullptr /* near_verification_status */);
  conversation->SubmitHumanConversationEntry(std::move(turn));
}

void AIChatUIPageHandler::ShowSoftKeyboard() {}

void AIChatUIPageHandler::ProcessImageFile(
    const std::vector<uint8_t>& file_data,
    const std::string& filename,
    ProcessImageFileCallback callback) {
  DecodeAndScaleImage(
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
                ai_chat::mojom::UploadedFileType::kImage, std::nullopt);
            std::move(callback).Run(std::move(uploaded_file));
          },
          filename, std::move(callback)));
}

void AIChatUIPageHandler::ProcessPdfFile(const std::vector<uint8_t>& file_data,
                                         const std::string& filename,
                                         ProcessPdfFileCallback callback) {
  // iOS does not support background PDF text extraction.
  // Return the raw PDF data without extracted text.
  auto uploaded_file = ai_chat::mojom::UploadedFile::New(
      filename, file_data.size(), file_data,
      ai_chat::mojom::UploadedFileType::kPdf, std::nullopt);
  std::move(callback).Run(std::move(uploaded_file));
}

void AIChatUIPageHandler::ProcessTextFile(const std::vector<uint8_t>& file_data,
                                          const std::string& filename,
                                          ProcessTextFileCallback callback) {
  // iOS does not support background text extraction.
  // Return the raw data without extracted text.
  auto uploaded_file = ai_chat::mojom::UploadedFile::New(
      filename, file_data.size(), file_data,
      ai_chat::mojom::UploadedFileType::kText, std::nullopt);
  std::move(callback).Run(std::move(uploaded_file));
}

void AIChatUIPageHandler::UploadFile(bool use_media_capture,
                                     UploadFileCallback callback) {
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::FromWebState(owner_web_state_)->bridge();
  if (!bridge) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  auto handler = base::CallbackToBlock(base::BindOnce(
      [](UploadFileCallback callback, NSArray<AiChatUploadedFile*>* files) {
        if (!files || [files count] == 0) {
          std::move(callback).Run(std::nullopt);
          return;
        }
        std::vector<ai_chat::mojom::UploadedFilePtr> uploaded_files;
        for (AiChatUploadedFile* file in files) {
          uploaded_files.emplace_back(file.cppObjPtr);
        }
        std::move(callback).Run(std::move(uploaded_files));
      },
      std::move(callback)));
  [bridge handleFileUploadRequest:use_media_capture completionHandler:handler];
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
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::FromWebState(owner_web_state_)->bridge();
  [bridge openAIChatSettings];
}

void AIChatUIPageHandler::OpenMemorySettings() {
  // Memory settings arent supported on mobile
  NOTIMPLEMENTED();
}

void AIChatUIPageHandler::OpenConversationFullPage(
    const std::string& conversation_uuid) {
  // This will only be called when in non-standalone mode
  OpenURL(ConversationUrl(conversation_uuid));
}

void AIChatUIPageHandler::OpenAIChatAgentProfile() {
  CHECK(ai_chat::features::IsAIChatAgentProfileEnabled());
  NOTIMPLEMENTED();
}

void AIChatUIPageHandler::OpenURL(const GURL& url) {
  if (!url.SchemeIs(kChromeUIScheme) && !url.SchemeIs(url::kHttpsScheme)) {
    return;
  }
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::FromWebState(owner_web_state_)->bridge();
  [bridge openURL:net::NSURLWithGURL(url)];
}

void AIChatUIPageHandler::OpenStorageSupportUrl() {
  OpenURL(GURL(kLeoStorageSupportUrl));
}

void AIChatUIPageHandler::GoPremium() {
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::FromWebState(owner_web_state_)->bridge();
  [bridge goPremium];
}

void AIChatUIPageHandler::RefreshPremiumSession() {
  OpenURL(GURL(kLeoRefreshPremiumSessionUrl));
}

void AIChatUIPageHandler::ManagePremium() {
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::FromWebState(owner_web_state_)->bridge();
  [bridge managePremium];
}

void AIChatUIPageHandler::OpenModelSupportUrl() {
  OpenURL(GURL(kLeoModelSupportUrl));
}

void AIChatUIPageHandler::OnRequestArchive(
    AssociatedContentDelegate* delegate) {
  // This is only applicable to content-adjacent UI, e.g. SidePanel on Desktop
  // where it would like to remain associated with the Tab and move away from
  // Conversations of previous navigations. That doens't apply to the standalone
  // UI where it will keep a previous navigation's conversation active.

  chat_ui_->OnNewDefaultConversation(
      ai_chat_tab_helper_
          ? std::make_optional(ai_chat_tab_helper_->content_id())
          : std::nullopt);
}

void AIChatUIPageHandler::CloseUI() {
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::FromWebState(owner_web_state_)->bridge();
  [bridge closeUI];
}

void AIChatUIPageHandler::SetChatUI(mojo::PendingRemote<mojom::ChatUI> chat_ui,
                                    SetChatUICallback callback) {
  chat_ui_.Bind(std::move(chat_ui));
  std::move(callback).Run(true);  // Always run in standalone mode on iOS

  chat_ui_->OnNewDefaultConversation(
      ai_chat_tab_helper_
          ? std::make_optional(ai_chat_tab_helper_->content_id())
          : std::nullopt);
}

void AIChatUIPageHandler::BindRelatedConversation(
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  // For global panel, don't recall conversations by their associated tab
  if (!ai_chat_tab_helper_ || !conversations_are_content_associated_) {
    ConversationHandler* conversation =
        AIChatServiceFactory::GetForProfile(profile_)->CreateConversation();
    conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
    return;
  }

  ConversationHandler* conversation =
      AIChatServiceFactory::GetForProfile(profile_)
          ->GetOrCreateConversationHandlerForContent(
              ai_chat_tab_helper_->content_id(),
              ai_chat_tab_helper_->GetWeakPtr());

  conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
}

void AIChatUIPageHandler::AssociateTab(mojom::TabDataPtr mojom_tab,
                                       const std::string& conversation_uuid) {
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::FromWebState(owner_web_state_)->bridge();
  if (BraveWebView* web_view =
          [bridge webViewForTabWithSessionID:mojom_tab->id]) {
    web::WebState* web_state = web_view.webState;
    auto* tab_helper = ai_chat::AIChatTabHelper::FromWebState(web_state);
    if (!tab_helper) {
      return;
    }

    ProfileIOS* profile =
        ProfileIOS::FromBrowserState(web_state->GetBrowserState());
    AIChatService* service = AIChatServiceFactory::GetForProfile(profile);
    service->MaybeAssociateContent(tab_helper, conversation_uuid);
  }
}

void AIChatUIPageHandler::AssociateUrlContent(
    const GURL& url,
    const std::string& title,
    const std::string& conversation_uuid) {
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::FromWebState(owner_web_state_)->bridge();
  if (!bridge) {
    return;
  }
  ProfileIOS* profile =
      ProfileIOS::FromBrowserState(owner_web_state_->GetBrowserState());
  AIChatService* service = AIChatServiceFactory::GetForProfile(profile);
  ProfileBridgeImpl* profileBridge =
      [[ProfileBridgeImpl alloc] initWithProfile:profile];
  auto callback = base::CallbackToBlock(base::BindOnce(
      &AIChatUIPageHandler::OnFetchContextForAssociatingUrlContent,
      weak_ptr_factory_.GetWeakPtr(), url, base::UTF8ToUTF16(title),
      conversation_uuid, service));
  [bridge contextForAssociatingURLContentForProfile:profileBridge
                                  completionHandler:callback];
}

void AIChatUIPageHandler::OnFetchContextForAssociatingUrlContent(
    const GURL& url,
    const std::u16string title,
    const std::string& conversation_uuid,
    AIChatService* service,
    id<AIChatAssociatedURLContentContext> context) {
  if (!context) {
    return;
  }
  auto content =
      std::make_unique<ai_chat::AssociatedURLContent>(url, title, context);
  service->AssociateOwnedContent(std::move(content), conversation_uuid);
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
  // For standalone or global panel, don't recall conversations by their
  // associated tab.
  if (ai_chat_tab_helper_ && conversations_are_content_associated_) {
    conversation = AIChatServiceFactory::GetForProfile(profile_)
                       ->CreateConversationHandlerForContent(
                           ai_chat_tab_helper_->content_id(),
                           ai_chat_tab_helper_->GetWeakPtr());
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

void AIChatUIPageHandler::ChatContextObserver::WebStateDestroyed(
    web::WebState* web_state) {
  page_handler_->HandleWebStateDestroyed();
}

void AIChatUIPageHandler::HandleWebStateDestroyed() {
  ai_chat_tab_helper_ = nullptr;
  associated_content_delegate_observation_.Reset();
  chat_context_observer_.reset();
}

}  // namespace ai_chat
