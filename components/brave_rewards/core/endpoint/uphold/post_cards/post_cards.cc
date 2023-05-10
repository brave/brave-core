/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/post_cards/post_cards.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/uphold/uphold_card.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::uphold {

PostCards::PostCards(LedgerImpl& ledger) : ledger_(ledger) {}

PostCards::~PostCards() = default;

std::string PostCards::GetUrl() {
  return GetServerUrl("/v0/me/cards");
}

std::string PostCards::GeneratePayload() {
  base::Value::Dict payload;
  payload.Set("label", internal::uphold::kCardName);
  payload.Set("currency", "BAT");

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

mojom::Result PostCards::CheckStatusCode(int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Unauthorized access");
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::LEDGER_ERROR;
  }

  return mojom::Result::LEDGER_OK;
}

mojom::Result PostCards::ParseBody(const std::string& body, std::string* id) {
  DCHECK(id);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::LEDGER_ERROR;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* id_str = dict.FindString("id");
  if (!id_str) {
    BLOG(0, "Missing id");
    return mojom::Result::LEDGER_ERROR;
  }

  *id = *id_str;

  return mojom::Result::LEDGER_OK;
}

void PostCards::Request(const std::string& token, PostCardsCallback callback) {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload();
  request->headers = RequestAuthorization(token);
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;

  ledger_->LoadURL(std::move(request),
                   base::BindOnce(&PostCards::OnRequest, base::Unretained(this),
                                  std::move(callback)));
}

void PostCards::OnRequest(PostCardsCallback callback,
                          mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response, true);

  mojom::Result result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::LEDGER_OK) {
    return std::move(callback).Run(result, "");
  }

  std::string id;
  result = ParseBody(response->body, &id);
  std::move(callback).Run(result, std::move(id));
}

}  // namespace brave_rewards::internal::endpoint::uphold
