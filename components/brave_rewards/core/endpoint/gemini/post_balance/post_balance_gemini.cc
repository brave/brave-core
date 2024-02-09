/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/post_balance/post_balance_gemini.h"

#include <optional>
#include <string_view>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;

namespace brave_rewards::internal {
namespace endpoint {
namespace gemini {

PostBalance::PostBalance(RewardsEngineImpl& engine) : engine_(engine) {}

PostBalance::~PostBalance() = default;

std::string PostBalance::GetUrl() {
  return engine_->Get<EnvironmentConfig>()
      .gemini_api_url()
      .Resolve("/v1/balances")
      .spec();
}

mojom::Result PostBalance::ParseBody(const std::string& body,
                                     double* available) {
  DCHECK(available);

  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_list()) {
    engine_->LogError(FROM_HERE) << "Invalid JSON";
    return mojom::Result::FAILED;
  }

  auto& balances = value->GetList();
  for (auto& item : balances) {
    DCHECK(item.is_dict());
    auto& balance = item.GetDict();
    const auto* currency_code = balance.FindString("currency");
    if (!currency_code || *currency_code != "BAT") {
      continue;
    }

    const auto* available_value = balance.FindString("available");
    if (!available_value) {
      engine_->LogError(FROM_HERE) << "Missing available";
      return mojom::Result::FAILED;
    }

    const bool result =
        base::StringToDouble(std::string_view(*available_value), available);
    if (!result) {
      engine_->LogError(FROM_HERE) << "Invalid balance";
      return mojom::Result::FAILED;
    }

    return mojom::Result::OK;
  }

  // If BAT is not found in the list, BAT balance for gemini is 0
  *available = 0;
  return mojom::Result::OK;
}

void PostBalance::Request(const std::string& token,
                          PostBalanceCallback callback) {
  auto url_callback = base::BindOnce(
      &PostBalance::OnRequest, base::Unretained(this), std::move(callback));
  auto request = mojom::UrlRequest::New();
  request->url = GetUrl();
  request->method = mojom::UrlMethod::POST;
  request->headers = {"Authorization: Bearer " + token};

  engine_->Get<URLLoader>().Load(std::move(request),
                                 URLLoader::LogLevel::kDetailed,
                                 std::move(url_callback));
}

void PostBalance::OnRequest(PostBalanceCallback callback,
                            mojom::UrlResponsePtr response) {
  DCHECK(response);

  switch (response->status_code) {
    case net::HTTP_OK:
      break;
    case net::HTTP_UNAUTHORIZED:
    case net::HTTP_FORBIDDEN:
      std::move(callback).Run(mojom::Result::EXPIRED_TOKEN, 0);
      return;
    default:
      std::move(callback).Run(mojom::Result::FAILED, 0);
      return;
  }

  double available;
  mojom::Result result = ParseBody(response->body, &available);
  std::move(callback).Run(result, available);
}

}  // namespace gemini
}  // namespace endpoint
}  // namespace brave_rewards::internal
