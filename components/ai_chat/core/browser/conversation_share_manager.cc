// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_share_manager.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/brave_service_keys/brave_service_key_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace ai_chat {

namespace {

constexpr char kSharePath[] = "v1/share";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("ai_chat", R"(
      semantics {
        sender: "AI Chat"
        description:
          "Uploads an end-to-end encrypted AI Chat conversation to Brave's"
          "sharing service so the user can share a link to view it. The"
          "service cannot read the conversation because the decryption key"
          "never leaves the client."
        trigger:
          "Triggered by the user choosing to share a conversation from the"
          "conversation header."
        data:
          "An encrypted blob of the conversation data. The encryption key is"
          "not included."
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

ConversationShareManager::ConversationShareManager(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(
          std::make_unique<api_request_helper::APIRequestHelper>(
              GetNetworkTrafficAnnotationTag(),
              std::move(url_loader_factory))) {}

ConversationShareManager::~ConversationShareManager() = default;

void ConversationShareManager::ShareConversation(
    const std::string& encrypted_contents,
    ShareConversationCallback callback) {
  const GURL api_url = GetEndpointUrl(/*premium=*/false, kSharePath);
  if (!api_url.is_valid()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  base::DictValue dict;
  dict.Set("ciphertext", encrypted_contents);
  std::string request_body;
  base::JSONWriter::Write(dict, &request_body);

  // Use the same relay authentication as the conversation API so the request
  // reaches the same host.
  auto headers = GetBraveHeaders(std::nullopt);
  const auto digest_header = brave_service_keys::GetDigestHeader(request_body);
  headers.emplace(digest_header.first, digest_header.second);
  auto auth_header = brave_service_keys::GetAuthorizationHeader(
      BUILDFLAG(SERVICE_KEY_AICHAT), headers, api_url,
      net::HttpRequestHeaders::kPostMethod, {"digest"});
  headers.emplace(auth_header.first, auth_header.second);
  headers.emplace("Accept", "application/json");

  api_request_helper_->Request(
      net::HttpRequestHeaders::kPostMethod, api_url, request_body,
      "application/json",
      base::BindOnce(&ConversationShareManager::OnShareCompleted,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)),
      headers, {});
}

void ConversationShareManager::OnShareCompleted(
    ShareConversationCallback callback,
    api_request_helper::APIRequestResult result) {
  if (!result.Is2XXResponseCode() || !result.value_body().is_dict()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  const base::DictValue& response = result.value_body().GetDict();
  const std::string* share_id = response.FindString("share_id");
  if (!share_id || share_id->empty()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  GURL shared_conversation_viewer_url(base::StrCat(
      {features::kAIChatConversationShareBaseUrl.Get(), *share_id}));
  if (!shared_conversation_viewer_url.is_valid()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  // A missing deletion id doesn't invalidate the share - the link works, it
  // just can't be deleted from this client.
  const std::string* deletion_id = response.FindString("deletion_id");
  std::move(callback).Run(ConversationShareResult{
      .viewer_url = std::move(shared_conversation_viewer_url),
      .share_id = *share_id,
      .deletion_id = deletion_id ? *deletion_id : std::string()});
}

}  // namespace ai_chat
