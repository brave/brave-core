/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_FEEDBACK_API_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_FEEDBACK_API_H_

#include <optional>
#include <string>

#include "base/containers/span.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/api_request_helper/api_request_helper.h"

template <class T>
class scoped_refptr;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace ai_chat {

using api_request_helper::APIRequestResult;

class AIChatFeedbackAPI {
 public:
  AIChatFeedbackAPI(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const std::string& channel_name);

  AIChatFeedbackAPI(const AIChatFeedbackAPI&) = delete;
  AIChatFeedbackAPI& operator=(const AIChatFeedbackAPI&) = delete;
  ~AIChatFeedbackAPI();

  void SendRating(bool is_liked,
                  bool is_premium,
                  const base::span<const mojom::ConversationTurnPtr>& history,
                  const std::string& model_name,
                  const std::string& selected_language,
                  api_request_helper::APIRequestHelper::ResultCallback
                      on_complete_callback);

  void SendFeedback(const std::string& category,
                    const std::string& feedback,
                    const std::string& rating_id,
                    const std::optional<std::string>& hostname,
                    const std::string& selected_language,
                    api_request_helper::APIRequestHelper::ResultCallback
                        on_complete_callback);

 private:
  api_request_helper::APIRequestHelper api_request_helper_;
  std::string channel_name_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_FEEDBACK_API_H_
