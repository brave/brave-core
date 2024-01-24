/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/get_cards/get_cards.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/uphold/uphold_card.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::uphold {

GetCards::GetCards(RewardsEngineImpl& engine) : engine_(engine) {}

GetCards::~GetCards() = default;

std::string GetCards::GetUrl() const {
  return engine_->Get<EnvironmentConfig>()
      .uphold_api_url()
      .Resolve("/v0/me/cards?q=currency:BAT")
      .spec();
}

mojom::Result GetCards::CheckStatusCode(int status_code) const {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    engine_->LogError(FROM_HERE) << "Unauthorized access";
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK && status_code != net::HTTP_PARTIAL_CONTENT) {
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result GetCards::ParseBody(const std::string& body,
                                  std::string* id) const {
  DCHECK(id);

  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_list()) {
    engine_->LogError(FROM_HERE) << "Invalid JSON";
    return mojom::Result::FAILED;
  }

  auto& list = value->GetList();
  for (const auto& it : list) {
    DCHECK(it.is_dict());
    const auto& dict = it.GetDict();
    const auto* label = dict.FindString("label");
    if (!label) {
      continue;
    }

    if (*label == internal::uphold::kCardName) {
      const auto* id_str = dict.FindString("id");
      if (!id_str) {
        continue;
      }

      *id = *id_str;
      return mojom::Result::OK;
    }
  }

  return mojom::Result::FAILED;
}

void GetCards::Request(const std::string& token,
                       GetCardsCallback callback) const {
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->headers = {"Authorization: Bearer " + token};

  engine_->Get<URLLoader>().Load(
      std::move(request), URLLoader::LogLevel::kDetailed,
      base::BindOnce(&GetCards::OnRequest, base::Unretained(this),
                     std::move(callback)));
}

void GetCards::OnRequest(GetCardsCallback callback,
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
