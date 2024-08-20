// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/ai_chat/conversation_driver_ios.h"

#import "ai_chat.mojom.objc+private.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/base/mac/conversions.h"
#include "brave/components/ai_chat/core/browser/conversation_driver.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/ios/browser/api/ai_chat/ai_chat_delegate.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "net/base/apple/url_conversions.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace ai_chat {

ConversationDriverIOS::ConversationDriverIOS(
    PrefService* profile_prefs,
    PrefService* local_state_prefs,
    ModelService* model_service,
    AIChatMetrics* ai_chat_metrics,
    base::RepeatingCallback<mojo::PendingRemote<skus::mojom::SkusService>()>
        skus_service_getter,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::string_view channel_string,
    id<AIChatDelegate> delegate)
    : ConversationDriver(profile_prefs,
                         local_state_prefs,
                         model_service,
                         ai_chat_metrics,
                         skus_service_getter,
                         url_loader_factory,
                         channel_string),
      bridge_(delegate) {
  chat_driver_observation_.Observe(this);
}

ConversationDriverIOS::ConversationDriverIOS(
    PrefService* profile_prefs,
    PrefService* local_state_prefs,
    ModelService* model_service,
    AIChatMetrics* ai_chat_metrics,
    std::unique_ptr<AIChatCredentialManager> credential_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::string_view channel_string,
    id<AIChatDelegate> delegate)
    : ConversationDriver(profile_prefs,
                         local_state_prefs,
                         model_service,
                         std::move(ai_chat_metrics),
                         std::move(credential_manager),
                         url_loader_factory,
                         channel_string),
      bridge_(delegate) {
  chat_driver_observation_.Observe(this);
}

ConversationDriverIOS::~ConversationDriverIOS() = default;

std::u16string ConversationDriverIOS::GetPageTitle() const {
  NSString* title = [bridge_ getPageTitle];
  return title ? base::SysNSStringToUTF16(title) : std::u16string();
}

GURL ConversationDriverIOS::GetPageURL() const {
  return net::GURLWithNSURL([bridge_ getLastCommittedURL]);
}

void ConversationDriverIOS::GetPageContent(
    ConversationDriver::GetPageContentCallback callback,
    std::string_view invalidation_token) {
  [bridge_
      getPageContentWithCompletion:[callback =
                                        std::make_shared<decltype(callback)>(
                                            std::move(callback))](
                                       NSString* content, bool isVideo) {
        if (callback) {
          std::move(*callback).Run(
              content ? base::SysNSStringToUTF8(content) : std::string(),
              isVideo, std::string());
        }
      }];
}

void ConversationDriverIOS::PrintPreviewFallback(
    ConversationDriver::GetPageContentCallback callback) {
  std::move(callback).Run(std::string(), false, std::string());
}

// MARK: - ConversationDriver::Observer

void ConversationDriverIOS::OnHistoryUpdate() {
  [bridge_ onHistoryUpdate];
}

void ConversationDriverIOS::OnAPIRequestInProgress(bool in_progress) {
  [bridge_ onAPIRequestInProgress:in_progress];
}

void ConversationDriverIOS::OnAPIResponseError(ai_chat::mojom::APIError error) {
  [bridge_ onAPIResponseError:(AiChatAPIError)error];
}

void ConversationDriverIOS::OnModelDataChanged(
    const std::string& model_key,
    const std::vector<ai_chat::mojom::ModelPtr>& model_list) {
  NSMutableArray* list =
      [[NSMutableArray alloc] initWithCapacity:model_list.size()];
  for (auto& model : model_list) {
    [list addObject:[[AiChatModel alloc] initWithModelPtr:model->Clone()]];
  }
  [bridge_ onModelChanged:base::SysUTF8ToNSString(model_key)
                modelList:[list copy]];
}

void ConversationDriverIOS::OnSuggestedQuestionsChanged(
    std::vector<std::string> questions,
    ai_chat::mojom::SuggestionGenerationStatus status) {
  [bridge_
      onSuggestedQuestionsChanged:brave::vector_to_ns(questions)
                           status:(AiChatSuggestionGenerationStatus)status];
}

void ConversationDriverIOS::OnPageHasContent(
    ai_chat::mojom::SiteInfoPtr site_info) {
  [bridge_ onPageHasContent:[[AiChatSiteInfo alloc]
                                initWithSiteInfoPtr:std::move(site_info)]];
}

}  // namespace ai_chat
