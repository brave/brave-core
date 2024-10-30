// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_AI_CHAT_CONVERSATION_CLIENT_H_
#define BRAVE_IOS_BROWSER_API_AI_CHAT_CONVERSATION_CLIENT_H_

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

@protocol AIChatDelegate;

namespace ai_chat {

class ConversationHandler;

// TODO(petemill): Have AIChatViewModel.swift (aka AIChatDelegate) implement
// mojom::ConversationUI and bind directly to ConversationHandler via ai_chat.mm
// so that this proxy isn't neccessary.
class ConversationClient : public mojom::ConversationUI {
 public:
  ConversationClient(ConversationHandler* conversation,
                     id<AIChatDelegate> bridge);
  ~ConversationClient() override;

 protected:
  // mojom::ConversationUI
  void OnConversationHistoryUpdate() override;
  void OnAPIRequestInProgress(bool is_request_in_progress) override;
  void OnAPIResponseError(mojom::APIError error) override;
  void OnModelDataChanged(
      const std::string& model_key,
      std::vector<ai_chat::mojom::ModelPtr> model_list) override;
  void OnSuggestedQuestionsChanged(
      const std::vector<std::string>& questions,
      mojom::SuggestionGenerationStatus status) override;
  void OnAssociatedContentInfoChanged(const mojom::SiteInfoPtr site_info,
                                      bool should_send_content) override;
  void OnFaviconImageDataChanged() override;
  void OnConversationDeleted() override;

 private:
  // The actual UI
  __weak id<AIChatDelegate> bridge_;

  mojo::Receiver<mojom::ConversationUI> receiver_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_API_AI_CHAT_CONVERSATION_CLIENT_H_
