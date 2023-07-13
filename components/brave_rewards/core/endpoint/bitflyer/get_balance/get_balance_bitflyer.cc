/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/bitflyer/get_balance/get_balance_bitflyer.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace brave_rewards::internal {
namespace endpoint {
namespace bitflyer {

GetBalance::GetBalance(RewardsEngineImpl& engine) : engine_(engine) {}

GetBalance::~GetBalance() = default;

std::string GetBalance::GetUrl() {
  return GetServerUrl("/api/link/v1/account/inventory");
}

mojom::Result GetBalance::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_UNAUTHORIZED ||
      status_code == net::HTTP_NOT_FOUND ||
      status_code == net::HTTP_FORBIDDEN) {
    BLOG(0, "Invalid authorization HTTP status: " << status_code);
    return mojom::Result::EXPIRED_TOKEN;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result GetBalance::ParseBody(const std::string& body,
                                    double* available) {
  DCHECK(available);

  absl::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* inventory = dict.FindList("inventory");
  if (!inventory) {
    BLOG(0, "Missing inventory");
    return mojom::Result::FAILED;
  }

  for (const auto& item : *inventory) {
    const auto* dict_value = item.GetIfDict();
    if (!dict_value) {
      continue;
    }

    const auto* currency_code = dict_value->FindString("currency_code");
    if (!currency_code || *currency_code != "BAT") {
      continue;
    }

    const auto available_value = dict_value->FindDouble("available");
    if (!available_value) {
      BLOG(0, "Missing available");
      return mojom::Result::FAILED;
    }

    *available = available_value.value();

    return mojom::Result::OK;
  }

  BLOG(0, "Missing BAT in inventory");
  return mojom::Result::FAILED;
}

void GetBalance::Request(const std::string& token,
                         GetBalanceCallback callback) {
  auto url_callback = base::BindOnce(
      &GetBalance::OnRequest, base::Unretained(this), std::move(callback));
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->headers = RequestAuthorization(token);
  engine_->LoadURL(std::move(request), std::move(url_callback));
}

void GetBalance::OnRequest(GetBalanceCallback callback,
                           mojom::UrlResponsePtr response) {
  DCHECK(response);
  LogUrlResponse(__func__, *response);

  mojom::Result result = CheckStatusCode(response->status_code);

  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, 0.0);
    return;
  }

  double available;
  result = ParseBody(response->body, &available);
  std::move(callback).Run(result, available);
}

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace brave_rewards::internal
