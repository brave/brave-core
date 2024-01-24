/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/get_me/get_me.h"

#include <optional>
#include <utility>

#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace uphold {

GetMe::GetMe(RewardsEngineImpl& engine) : engine_(engine) {}

GetMe::~GetMe() = default;

std::string GetMe::GetUrl() {
  return engine_->Get<EnvironmentConfig>()
      .uphold_api_url()
      .Resolve("/v0/me")
      .spec();
}

mojom::Result GetMe::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED) {
    engine_->LogError(FROM_HERE) << "Unauthorized access";
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result GetMe::ParseBody(const std::string& body,
                               internal::uphold::User* user) {
  DCHECK(user);

  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    engine_->LogError(FROM_HERE) << "Invalid JSON";
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* name = dict.FindString("firstName");
  if (name) {
    user->name = *name;
  }

  if (const auto* id = dict.FindString("id")) {
    user->member_id = *id;
  }

  if (const auto* country = dict.FindString("identityCountry")) {
    user->country_id = *country;
  }

  const auto* currencies = dict.FindList("currencies");
  if (currencies) {
    const std::string currency = "BAT";
    user->bat_not_allowed = !base::Contains(*currencies, base::Value(currency));
  }

  return mojom::Result::OK;
}

void GetMe::Request(const std::string& token, GetMeCallback callback) {
  auto url_callback = base::BindOnce(&GetMe::OnRequest, base::Unretained(this),
                                     std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->headers = {"Authorization: Bearer " + token};

  engine_->Get<URLLoader>().Load(std::move(request), URLLoader::LogLevel::kNone,
                                 std::move(url_callback));
}

void GetMe::OnRequest(GetMeCallback callback, mojom::UrlResponsePtr response) {
  DCHECK(response);

  internal::uphold::User user;
  mojom::Result result = CheckStatusCode(response->status_code);
  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, user);
    return;
  }

  result = ParseBody(response->body, &user);
  std::move(callback).Run(result, user);
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace brave_rewards::internal
