// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/ai_chat/conversation_client.h"

#include "ai_chat.mojom.objc+private.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/base/mac/conversions.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-shared.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/ios/browser/api/ai_chat/ai_chat_delegate.h"
#include "ios/chrome/common/channel_info.h"

namespace ai_chat {

ConversationClient::ConversationClient(ConversationHandler* conversation,
                                       id<AIChatDelegate> bridge)
    : bridge_(bridge) {
  conversation->Bind(receiver_.BindNewPipeAndPassRemote());
}

ConversationClient::~ConversationClient() = default;

// MARK: - mojom::ConversationUI

void ConversationClient::OnConversationHistoryUpdate() {
  [bridge_ onHistoryUpdate];
}

void ConversationClient::OnAPIRequestInProgress(bool in_progress) {
  [bridge_ onAPIRequestInProgress:in_progress];
}

void ConversationClient::OnAPIResponseError(mojom::APIError error) {
  [bridge_ onAPIResponseError:(AiChatAPIError)error];
}

void ConversationClient::OnModelDataChanged(
    const std::string& model_key,
    std::vector<mojom::ModelPtr> model_list) {
  NSMutableArray* models =
      [[NSMutableArray alloc] initWithCapacity:model_list.size()];

  for (auto& model : model_list) {
    [models addObject:[[AiChatModel alloc] initWithModelPtr:model->Clone()]];
  }

  [bridge_ onModelChanged:base::SysUTF8ToNSString(model_key) modelList:models];
}

void ConversationClient::OnSuggestedQuestionsChanged(
    const std::vector<std::string>& questions,
    mojom::SuggestionGenerationStatus status) {
  [bridge_
      onSuggestedQuestionsChanged:brave::vector_to_ns(questions)
                           status:(AiChatSuggestionGenerationStatus)status];
}

void ConversationClient::OnAssociatedContentInfoChanged(
    const mojom::SiteInfoPtr site_info,
    bool should_send_content) {
  [bridge_ onPageHasContent:[[AiChatSiteInfo alloc]
                                initWithSiteInfoPtr:site_info->Clone()]
          shouldSendContent:should_send_content];
}

void ConversationClient::OnFaviconImageDataChanged() {}

void ConversationClient::OnConversationDeleted() {
  // TODO(petemill): UI should bind to a new conversation. This only
  // needs to be handled when the AIChatStorage feature is enabled, which
  // allows deletion.
}

}  // namespace ai_chat
