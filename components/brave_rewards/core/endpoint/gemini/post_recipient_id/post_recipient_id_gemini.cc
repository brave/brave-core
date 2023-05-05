/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/post_recipient_id/post_recipient_id_gemini.h"

#include <utility>

#include "base/base64.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/gemini/gemini_util.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_rewards::internal::endpoint::gemini {

PostRecipientId::PostRecipientId(LedgerImpl& ledger) : ledger_(ledger) {}

PostRecipientId::~PostRecipientId() = default;

std::string PostRecipientId::GetUrl() {
  return GetApiServerUrl("/v1/payments/recipientIds");
}

mojom::Result PostRecipientId::ParseBody(const std::string& body,
                                         std::string* recipient_id) {
  DCHECK(recipient_id);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* result = dict.FindString("result");
  if (!result || *result != "OK") {
    BLOG(0, "Failed creating recipient_id");
    return mojom::Result::LEDGER_ERROR;
  }

  const auto* id = dict.FindString("recipient_id");
  if (!id) {
    BLOG(0, "Response missing a recipient_id");
    return mojom::Result::LEDGER_ERROR;
  }

  *recipient_id = *id;
  return mojom::Result::LEDGER_OK;
}

std::string PostRecipientId::GeneratePayload() {
  base::Value::Dict payload;
  payload.Set("label", internal::gemini::kGeminiRecipientIDLabel);

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
  request->headers = RequestAuthorization(token);
  request->headers.push_back("X-GEMINI-PAYLOAD: " + GeneratePayload());

  ledger_->LoadURL(std::move(request),
                   base::BindOnce(&PostRecipientId::OnRequest,
                                  base::Unretained(this), std::move(callback)));
}

void PostRecipientId::OnRequest(PostRecipientIdCallback callback,
                                mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);

  auto header = response->headers.find("www-authenticate");
  if (header != response->headers.end()) {
    std::string auth_header = header->second;
    if (auth_header.find("unverified_account") != std::string::npos) {
      return std::move(callback).Run(mojom::Result::NOT_FOUND, "");
    }
  }

  mojom::Result result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::LEDGER_OK) {
    return std::move(callback).Run(result, "");
  }

  std::string recipient_id;
  result = ParseBody(response->body, &recipient_id);
  std::move(callback).Run(result, std::move(recipient_id));
}

}  // namespace brave_rewards::internal::endpoint::gemini
