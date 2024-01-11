/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/post_cards/post_cards.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/uphold/uphold_card.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::uphold {

PostCards::PostCards(RewardsEngineImpl& engine) : engine_(engine) {}

PostCards::~PostCards() = default;

std::string PostCards::GetUrl() const {
  return engine_->Get<EnvironmentConfig>()
      .uphold_api_url()
      .Resolve("/v0/me/cards")
      .spec();
}

std::string PostCards::GeneratePayload() const {
  base::Value::Dict payload;
  payload.Set("label", internal::uphold::kCardName);
  payload.Set("currency", "BAT");

  std::string json;
  base::JSONWriter::Write(payload, &json);
  return json;
}

mojom::Result PostCards::CheckStatusCode(int status_code) const {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    BLOG(0, "Unauthorized access");
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result PostCards::ParseBody(const std::string& body,
                                   std::string* id) const {
  DCHECK(id);

  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* id_str = dict.FindString("id");
  if (!id_str) {
    BLOG(0, "Missing id");
    return mojom::Result::FAILED;
  }

  *id = *id_str;

  return mojom::Result::OK;
}

void PostCards::Request(const std::string& token,
                        PostCardsCallback callback) const {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->content = GeneratePayload();
  request->headers = {"Authorization: Bearer " + token};
  request->content_type = "application/json; charset=utf-8";
  request->method = mojom::UrlMethod::POST;

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kNone,
      base::BindOnce(&PostCards::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void PostCards::OnRequest(PostCardsCallback callback,
                          mojom::UrlResponsePtr response) const {
  DCHECK(response);

  mojom::Result result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::OK) {
    return std::move(callback).Run(result, "");
  }

  std::string id;
  result = ParseBody(response->body, &id);
  std::move(callback).Run(result, std::move(id));
}

}  // namespace brave_rewards::internal::endpoint::uphold
