/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/get_available/get_available.h"

#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/url_helpers.h"
#include "brave/components/brave_rewards/core/common/url_loader.h"
#include "brave/components/brave_rewards/core/promotion/promotion_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/wallet/wallet.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

GetAvailable::GetAvailable(RewardsEngineImpl& engine) : engine_(engine) {}

GetAvailable::~GetAvailable() = default;

std::string GetAvailable::GetUrl(const std::string& platform) {
  auto url = engine_->Get<EnvironmentConfig>().rewards_grant_url().Resolve(
      "/v1/promotions");

  url = URLHelpers::SetQueryParameters(
      url, {{"migrate", "true"}, {"platform", platform}});

  if (const auto wallet = engine_->wallet()->GetWallet()) {
    url = URLHelpers::SetQueryParameters(url,
                                         {{"paymentId", wallet->payment_id}});
  }

  return url.spec();
}

mojom::Result GetAvailable::CheckStatusCode(const int status_code) {
  if (status_code == net::HTTP_BAD_REQUEST) {
    BLOG(0, "Invalid paymentId or platform in request");
    return mojom::Result::FAILED;
  }

  if (status_code == net::HTTP_NOT_FOUND) {
    BLOG(0, "Unrecognized paymentId/promotion combination");
    return mojom::Result::NOT_FOUND;
  }

  if (status_code == net::HTTP_INTERNAL_SERVER_ERROR) {
    BLOG(0, "Internal server error");
    return mojom::Result::FAILED;
  }

  if (status_code != net::HTTP_OK) {
    BLOG(0, "Unexpected HTTP status: " << status_code);
    return mojom::Result::FAILED;
  }

  return mojom::Result::OK;
}

mojom::Result GetAvailable::ParseBody(
    const std::string& body,
    std::vector<mojom::PromotionPtr>* list,
    std::vector<std::string>* corrupted_promotions) {
  DCHECK(list && corrupted_promotions);

  std::optional<base::Value> value = base::JSONReader::Read(body);
  if (!value || !value->is_dict()) {
    BLOG(0, "Invalid JSON");
    return mojom::Result::FAILED;
  }

  const base::Value::Dict& dict = value->GetDict();
  const auto* promotions = dict.FindList("promotions");
  if (!promotions) {
    return mojom::Result::OK;
  }

  const auto promotion_size = promotions->size();
  for (const auto& promotion_value : *promotions) {
    mojom::PromotionPtr promotion = mojom::Promotion::New();

    const auto* item = promotion_value.GetIfDict();
    if (!item) {
      continue;
    }

    const auto* id = item->FindString("id");
    if (!id) {
      continue;
    }
    promotion->id = *id;

    const auto version = item->FindInt("version");
    if (!version) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }
    promotion->version = *version;

    const auto* type = item->FindString("type");
    if (!type) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }
    promotion->type = internal::promotion::ConvertStringToPromotionType(*type);

    const auto suggestions = item->FindInt("suggestionsPerGrant");
    if (!suggestions) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }
    promotion->suggestions = *suggestions;

    const auto* approximate_value = item->FindString("approximateValue");
    if (!approximate_value) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }

    const bool success_conversion =
        base::StringToDouble(*approximate_value, &promotion->approximate_value);
    if (!success_conversion) {
      promotion->approximate_value = 0.0;
    }

    const auto available = item->FindBool("available");
    if (!available) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }

    if (*available) {
      promotion->status = mojom::PromotionStatus::ACTIVE;
    } else {
      promotion->status = mojom::PromotionStatus::OVER;
    }

    promotion->created_at = base::Time::Now().InSecondsFSinceUnixEpoch();
    if (auto* created_at = item->FindString("createdAt")) {
      base::Time time;
      if (base::Time::FromUTCString(created_at->c_str(), &time)) {
        promotion->created_at = time.InSecondsFSinceUnixEpoch();
      }
    }

    auto* expires_at = item->FindString("expiresAt");
    if (!expires_at) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }

    base::Time expires_at_time;
    bool success =
        base::Time::FromUTCString((*expires_at).c_str(), &expires_at_time);
    if (success) {
      promotion->expires_at = expires_at_time.InSecondsFSinceUnixEpoch();
    }

    auto* claimable_until = item->FindString("claimableUntil");
    if (claimable_until) {
      base::Time claimable_until_time;
      if (base::Time::FromUTCString(claimable_until->c_str(),
                                    &claimable_until_time)) {
        promotion->claimable_until =
            claimable_until_time.InSecondsFSinceUnixEpoch();
      }
    }

    auto* public_keys = item->FindList("publicKeys");
    if (!public_keys || public_keys->empty()) {
      corrupted_promotions->push_back(promotion->id);
      continue;
    }

    std::string keys_json;
    base::JSONWriter::Write(*public_keys, &keys_json);
    promotion->public_keys = keys_json;

    auto legacy_claimed = item->FindBool("legacyClaimed");
    promotion->legacy_claimed = legacy_claimed.value_or(false);

    list->push_back(std::move(promotion));
  }

  if (promotion_size != list->size()) {
    return mojom::Result::CORRUPTED_DATA;
  }

  return mojom::Result::OK;
}

void GetAvailable::Request(const std::string& platform,
                           GetAvailableCallback callback) {
  auto url_callback = base::BindOnce(
      &GetAvailable::OnRequest, base::Unretained(this), std::move(callback));

  auto request = mojom::UrlRequest::New();
  request->url = GetUrl(platform);
  engine_->Get<URLLoader>().Load(std::move(request),
                                 URLLoader::LogLevel::kDetailed,
                                 std::move(url_callback));
}

void GetAvailable::OnRequest(GetAvailableCallback callback,
                             mojom::UrlResponsePtr response) {
  DCHECK(response);

  std::vector<mojom::PromotionPtr> list;
  std::vector<std::string> corrupted_promotions;
  mojom::Result result = CheckStatusCode(response->status_code);

  if (result != mojom::Result::OK) {
    std::move(callback).Run(result, std::move(list), corrupted_promotions);
    return;
  }

  result = ParseBody(response->body, &list, &corrupted_promotions);
  std::move(callback).Run(result, std::move(list), corrupted_promotions);
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
