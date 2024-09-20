/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/get_card/get_card.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "brave/components/brave_rewards/core/uphold/uphold_card.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoint::uphold {

GetCard::GetCard(RewardsEngine& engine) : engine_(engine) {}

GetCard::~GetCard() = default;

std::string GetCard::GetUrl(const std::string& address) {
  return engine_->Get<EnvironmentConfig>()
      .uphold_api_url()
      .Resolve(base::StrCat({"/v0/me/cards/", address}))
      .spec();
}

mojom::Result GetCard::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED ||
      status_code == net::HTTP_NOT_FOUND ||
      status_code == net::HTTP_FORBIDDEN) {
    engine_->LogError(FROM_HERE)
        << "Unauthorized access HTTP status: " << status_code;
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (!URLLoader::IsSuccessCode(status_code)) {
    engine_->LogError(FROM_HERE) << "Unexpected HTTP status: " << status_code;
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result GetCard::ParseBody(const std::string& body, double* available) {
  DCHECK(available);

  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    engine_->LogError(FROM_HERE) << "Invalid JSON";
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* available_str = dict.FindString("available");
  if (!available_str) {
    engine_->LogError(FROM_HERE) << "Missing available";
    return mojom::Result::FAILED;
  }

  const bool success = base::StringToDouble(*available_str, available);
  if (!success) {
    *available = 0.0;
  }

  return mojom::Result::OK;
}

void GetCard::Request(const std::string& address,
                      const std::string& token,
                      GetCardCallback callback) {
  auto url_callback = base::BindOnce(
      &GetCard::OnRequest, base::Unretained(this), std::move(callback));
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(address);
  request->headers = {"Authorization: Bearer " + token};

  engine_->Get<URLLoader>().Load(std::move(request),
                                 URLLoader::LogLevel::kDetailed,
                                 std::move(url_callback));
}

void GetCard::OnRequest(GetCardCallback callback,
                        mojom::UrlResponsePtr response) {
  DCHECK(response);

  mojom::Result result = CheckStatusCode(response->status_code);

  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, 0.0);
    return;
  }

  double available;
  result = ParseBody(response->body, &available);
  std::move(callback).Run(result, available);
}

}  // namespace brave_rewards::internal::endpoint::uphold
