// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_ENDPOINTS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_ENDPOINTS_H_

#include <optional>

#include "base/values.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/endpoint_client/request_types.h"
#include "brave/components/endpoint_client/response.h"
#include "brave/components/endpoint_client/stream.h"
#include "url/gurl.h"

namespace ai_chat {

// https://github.com/brave/aichat/blob/8fc09e023e8674e1069b7c1c30f848c74c4c1154/aichat/serve/open_ai_api.py#L47
inline constexpr char kConversationAPIPath[] = "v1/chat/completions";

// The request body, built by ConversationAPIClient.
struct ConversationAPIRequestBody {
  base::DictValue ToValue() const { return dict.Clone(); }

  base::DictValue dict;
};

// Any JSON value, e.g. an OAI completion or an error.
struct ConversationAPIResponseBody {
  static std::optional<ConversationAPIResponseBody> FromValue(
      const base::Value& value) {
    return ConversationAPIResponseBody{value.Clone()};
  }

  base::Value value;
};

// The URL is that of the non-premium endpoint. Requests replace it with the
// one GetEndpointUrl() returns for them.
struct ConversationAPIEndpoint {
  using Request = endpoint_client::Post<ConversationAPIRequestBody>;
  using Response = endpoint_client::Response<ConversationAPIResponseBody,
                                             ConversationAPIResponseBody>;

  static GURL URL() {
    return GetEndpointUrl(/*premium=*/false, kConversationAPIPath);
  }
};

struct ConversationAPIStreamEndpoint {
  using Request = ConversationAPIEndpoint::Request;
  using Response = endpoint_client::Stream<ConversationAPIResponseBody>;

  static GURL URL() { return ConversationAPIEndpoint::URL(); }
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_ENGINE_CONVERSATION_API_ENDPOINTS_H_
