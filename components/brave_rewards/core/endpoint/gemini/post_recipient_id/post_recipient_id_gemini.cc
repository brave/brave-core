/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/post_recipient_id/post_recipient_id_gemini.h"

#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/uuid.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::gemini {

PostRecipientId::PostRecipientId(RewardsEngineImpl& engine) : engine_(engine) {}

PostRecipientId::~PostRecipientId() = default;

std::string PostRecipientId::GetUrl() {
  return engine_->Get<EnvironmentConfig>()
      .gemini_api_url()
      .Resolve("/v1/payments/recipientIds")
      .spec();
}

mojom::Result PostRecipientId::ParseBody(const std::string& body,
                                         std::string* recipient_id) {
  DCHECK(recipient_id);

  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* result = dict.FindString("result");
  if (!result || *result != "OK") {
    BLOG(0, "Failed creating recipient_id");
    return mojom::Result::FAILED;
  }

  const auto* id = dict.FindString("recipient_id");
  if (!id) {
    BLOG(0, "Response missing a recipient_id");
    return mojom::Result::FAILED;
  }

  *recipient_id = *id;
  return mojom::Result::OK;
}

std::string PostRecipientId::GeneratePayload() {
  base::Value::Dict payload;
  payload.Set("label", kRecipientLabel);

  std::string json;
  base::JSONWriter::Write(payload, &json);

  std::string base64;
  base::Base64Encode(json, &base64);
  return base64;
}

void PostRecipientId::Request(const std::string& token,
                              PostRecipientIdCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->method = mojom::UrlMethod::POST;
  request->headers = {"Authorization: Bearer " + token,
                      "X-GEMINI-PAYLOAD: " + GeneratePayload()};

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&PostRecipientId::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void PostRecipientId::OnRequest(PostRecipientIdCallback callback,
                                mojom::UrlResponsePtr response) {
  DCHECK(response);

  auto header = response->headers.find("www-authenticate");
  if (header != response->headers.end()) {
    std::string auth_header = header->second;
    if (auth_header.find("unverified_account") != std::string::npos) {
      return std::move(callback).Run(mojom::Result::NOT_FOUND, "");
    }
  }

  switch (response->status_code) {
    case net::HTTP_OK:
      break;
    case net::HTTP_NOT_FOUND:
      std::move(callback).Run(mojom::Result::NOT_FOUND, "");
      return;
    case net::HTTP_UNAUTHORIZED:
    case net::HTTP_FORBIDDEN:
      std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, "");
      return;
    default:
      std::move(callback).Run(mojom::Result::FAILED, "");
      return;
  }

  std::string recipient_id;
  mojom::Result result = ParseBody(response->body, &recipient_id);
  std::move(callback).Run(result, std::move(recipient_id));
}

}  // namespace brave_rewards::internal::endpoint::gemini
