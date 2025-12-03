// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_ui_page_handler.h"

#import <Foundation/Foundation.h>

#include <memory>
#include <string>
#include <vector>

#include "base/apple/foundation_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/notimplemented.h"
#include "base/strings/sys_string_conversions.h"
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
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ios/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/ios/browser/ai_chat/ai_chat_ui_handler_bridge.h"
#include "brave/ios/browser/ai_chat/ai_chat_ui_handler_bridge_holder.h"
#include "brave/ios/browser/misc_metrics/profile_misc_metrics_service.h"
#include "brave/ios/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "components/grit/brave_components_webui_strings.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/web/public/navigation/navigation_context.h"
#include "ios/web/public/navigation/navigation_manager.h"
#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_id.h"
#include "net/base/apple/url_conversions.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/mojom/data_decoder_service.mojom.h"
#include "services/data_decoder/public/mojom/image_decoder.mojom.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"
#include "ui/gfx/codec/png_codec.h"

namespace ai_chat {

namespace {

void OnImageDecoded(
    base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> callback,
    const SkBitmap& decoded_bitmap) {
  if (decoded_bitmap.drawsNothing()) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  auto encode_image = base::BindOnce(
      [](const SkBitmap& decoded_bitmap) {
        return gfx::PNGCodec::EncodeBGRASkBitmap(
            ScaleDownBitmap(decoded_bitmap), false);
      },
      decoded_bitmap);
  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(encode_image),
                                               std::move(callback));
}

// A constant defined in services/data_decoder/public/cpp/decode_image.h
const uint64_t kDataDecoderDefaultMaxSizeInBytes = 128 * 1024 * 1024;

// iOS can't access the standard cpp image decoder in the data_decoder as its
// only compiled with use_blink=true, so we must use the mojom API instead to
// use the in-process service
//
// The implementation of this method matches the one in
// services/data_decoder/public/cpp/decode_image.cc
void DecodeImageUsingServiceProcess(
    data_decoder::DataDecoder* data_decoder,
    base::span<const uint8_t> encoded_bytes,
    data_decoder::mojom::ImageCodec codec,
    bool shrink_to_fit,
    uint64_t max_size_in_bytes,
    const gfx::Size& desired_image_frame_size,
    base::OnceCallback<void(const SkBitmap& decoded_bitmap)> callback) {
  mojo::Remote<data_decoder::mojom::ImageDecoder> decoder;
  data_decoder->GetService()->BindImageDecoder(
      decoder.BindNewPipeAndPassReceiver());

  // `callback` will be run exactly once. Disconnect implies no response, and
  // OnDecodeImage promptly discards the decoder preventing further disconnect
  // calls.
  auto callback_pair = base::SplitOnceCallback(std::move(callback));
  decoder.set_disconnect_handler(
      base::BindOnce(std::move(callback_pair.first), SkBitmap()));

  data_decoder::mojom::ImageDecoder* raw_decoder = decoder.get();
  raw_decoder->DecodeImage(
      encoded_bytes, codec, shrink_to_fit, max_size_in_bytes,
      desired_image_frame_size,
      base::IgnoreArgs<base::TimeDelta>(std::move(callback_pair.second)));
}

}  // namespace

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
    // TODO: https://github.com/brave/brave-browser/issues/49451 Handle
    // non-standalone Leo AI
    // Typically this would create a tab helper and associate it with the chat
    // context WebState so we can create a new conversation on navigation
  }
}

AIChatUIPageHandler::~AIChatUIPageHandler() = default;

void AIChatUIPageHandler::HandleVoiceRecognition(
    const std::string& conversation_uuid) {
  if (conversation_uuid.empty()) {
    return;
  }
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::GetOrCreateForWebState(owner_web_state_)->bridge();
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
  DecodeImageUsingServiceProcess(
      &data_decoder_, file_data, data_decoder::mojom::ImageCodec::kDefault,
      /*shrink_to_fit=*/true, kDataDecoderDefaultMaxSizeInBytes, gfx::Size(),
      base::BindOnce(
          &OnImageDecoded,
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
              filename, std::move(callback))));
}

void AIChatUIPageHandler::UploadFile(bool use_media_capture,
                                     UploadFileCallback callback) {
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::GetOrCreateForWebState(owner_web_state_)->bridge();
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
      UIHandlerBridgeHolder::GetOrCreateForWebState(owner_web_state_)->bridge();
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
      UIHandlerBridgeHolder::GetOrCreateForWebState(owner_web_state_)->bridge();
  [bridge openURL:net::NSURLWithGURL(url)];
}

void AIChatUIPageHandler::OpenStorageSupportUrl() {
  OpenURL(GURL(kLeoStorageSupportUrl));
}

void AIChatUIPageHandler::GoPremium() {
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::GetOrCreateForWebState(owner_web_state_)->bridge();
  [bridge goPremium];
}

void AIChatUIPageHandler::RefreshPremiumSession() {
  OpenURL(GURL(kLeoRefreshPremiumSessionUrl));
}

void AIChatUIPageHandler::ManagePremium() {
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::GetOrCreateForWebState(owner_web_state_)->bridge();
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

  // TODO: https://github.com/brave/brave-browser/issues/49451 Handle
  // non-standalone Leo AI
  NOTIMPLEMENTED();
}

void AIChatUIPageHandler::CloseUI() {
  id<AIChatUIHandlerBridge> bridge =
      UIHandlerBridgeHolder::GetOrCreateForWebState(owner_web_state_)->bridge();
  [bridge closeUI];
}

void AIChatUIPageHandler::SetChatUI(mojo::PendingRemote<mojom::ChatUI> chat_ui,
                                    SetChatUICallback callback) {
  // TODO: https://github.com/brave/brave-browser/issues/49451 Handle
  // non-standalone Leo AI
  chat_ui_.Bind(std::move(chat_ui));
  std::move(callback).Run(true);
  chat_ui_->OnNewDefaultConversation(std::nullopt);
}

void AIChatUIPageHandler::BindRelatedConversation(
    mojo::PendingReceiver<mojom::ConversationHandler> receiver,
    mojo::PendingRemote<mojom::ConversationUI> conversation_ui_handler) {
  // For global panel, don't recall conversations by their associated tab
  // TODO: https://github.com/brave/brave-browser/issues/49451 Handle
  // non-standalone Leo AI and call GetOrCreateConversationHandlerForContent if
  // there is tab associated with this
  ConversationHandler* conversation =
      AIChatServiceFactory::GetForProfile(profile_)->CreateConversation();
  conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
}

void AIChatUIPageHandler::AssociateTab(mojom::TabDataPtr mojom_tab,
                                       const std::string& conversation_uuid) {
  // TODO: https://github.com/brave/brave-browser/issues/50196 Add webpage
  // context support
  NOTIMPLEMENTED();
}

void AIChatUIPageHandler::AssociateUrlContent(
    const GURL& url,
    const std::string& title,
    const std::string& conversation_uuid) {
  // TODO: https://github.com/brave/brave-browser/issues/50196 Add webpage
  // context support
  NOTIMPLEMENTED();
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
  // associated tas.
  // TODO: https://github.com/brave/brave-browser/issues/49451 Handle
  // non-standalone Leo AI and call CreateConversationHandlerForContent if there
  // is tab associated with this
  conversation =
      AIChatServiceFactory::GetForProfile(profile_)->CreateConversation();
  conversation->Bind(std::move(receiver), std::move(conversation_ui_handler));
}

void AIChatUIPageHandler::BindParentUIFrameFromChildFrame(
    mojo::PendingReceiver<mojom::ParentUIFrame> receiver) {
  chat_ui_->OnChildFrameBound(std::move(receiver));
}

}  // namespace ai_chat
