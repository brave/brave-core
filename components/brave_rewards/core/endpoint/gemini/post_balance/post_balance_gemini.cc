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
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/gemini/gemini_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

using std::placeholders::_1;

namespace brave_rewards::internal {
namespace endpoint {
namespace gemini {

PostBalance::PostBalance(RewardsEngineImpl& engine) : engine_(engine) {}

PostBalance::~PostBalance() = default;

std::string PostBalance::GetUrl() {
  return GetApiServerUrl("/v1/balances");
}

mojom::Result PostBalance::ParseBody(const std::string& body,
                                     double* available) {
  DCHECK(available);

  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_list()) {
    BLOG(0, "Invalid JSON");
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
      BLOG(0, "Missing available");
      return mojom::Result::FAILED;
    }

    const bool result =
        base::StringToDouble(std::string_view(*available_value), available);
    if (!result) {
      BLOG(0, "Invalid balance");
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
  request->headers = RequestAuthorization(token);

  engine_->Get<URLLoader>().Load(std::move(request),
                                 URLLoader::LogLevel::kDetailed,
                                 std::move(url_callback));
}

void PostBalance::OnRequest(PostBalanceCallback callback,
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

}  // namespace gemini
}  // namespace endpoint
}  // namespace brave_rewards::internal
