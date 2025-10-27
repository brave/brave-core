// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/conversation_api_v2_client.h"

#include "base/check.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat", R"(
      semantics {
        sender: "AI Chat"
        description:
          "This is used to communicate with Brave's AI Conversation API"
          "on behalf of the user interacting with different browser AI"
          "features."
        trigger:
          "Triggered by user interactions such as submitting an AI Chat"
          "conversation message, or requesting a text rewrite."
        data:
          "Conversational messages input by the user as well as associated"
          "content or user text to be rewritten. Can contain PII."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace

ConversationAPIV2Client::ConversationAPIV2Client(
    const std::string& model_name,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    AIChatCredentialManager* credential_manager,
    ModelService* model_service)
    : model_name_(model_name),
      credential_manager_(credential_manager),
      model_service_(model_service) {
  CHECK(!model_name_.empty());
  api_request_helper_ = std::make_unique<api_request_helper::APIRequestHelper>(
      GetNetworkTrafficAnnotationTag(), url_loader_factory);
}

ConversationAPIV2Client::~ConversationAPIV2Client() = default;

void ConversationAPIV2Client::ClearAllQueries() {
  api_request_helper_->CancelAll();
}

}  // namespace ai_chat
